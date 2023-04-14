/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 StarFive
 *
 * Authors:
 *   Wei Liang Lim <weiliang.lim@starfivetech.com>
 *   Minda Chen <minda.chen@starfivetech.com>
 */

#include <libfdt.h>
#include <platform_override.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_timer.h>
#include <sbi/riscv_io.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/reset/fdt_reset.h>
#include <sbi_utils/i2c/fdt_i2c.h>

struct pmic {
	struct i2c_adapter *adapter;
	u32 dev_addr;
	const char *compatible;
};

struct jh7110 {
	u64 pmu_reg_base;
	u64 clk_reg_base;
	u32 i2c_index;
};

static struct pmic pmic_inst;
static struct jh7110 jh7110_inst;
static u32 selected_hartid = -1;

/* PMU register define */
#define HW_EVENT_TURN_ON_MASK		0x04
#define HW_EVENT_TURN_OFF_MASK		0x08
#define SW_TURN_ON_POWER_MODE		0x0C
#define SW_TURN_OFF_POWER_MODE		0x10
#define SW_ENCOURAGE			0x44
#define PMU_INT_MASK			0x48
#define PCH_BYPASS			0x4C
#define PCH_PSTATE			0x50
#define PCH_TIMEOUT			0x54
#define LP_TIMEOUT			0x58
#define HW_TURN_ON_MODE			0x5C
#define CURR_POWER_MODE			0x80
#define PMU_EVENT_STATUS		0x88
#define PMU_INT_STATUS			0x8C

/* sw encourage cfg */
#define SW_MODE_ENCOURAGE_EN_LO		0x05
#define SW_MODE_ENCOURAGE_EN_HI		0x50
#define SW_MODE_ENCOURAGE_DIS_LO	0x0A
#define SW_MODE_ENCOURAGE_DIS_HI	0xA0
#define SW_MODE_ENCOURAGE_ON		0xFF

#define DEVICE_PD_MASK			0xfc
#define SYSTOP_CPU_PD_MASK		0x3

#define TIMEOUT_COUNT			100000
#define AXP15060_POWER_REG		0x32
#define AXP15060_POWER_OFF_BIT		BIT(7)
#define AXP15060_RESET_BIT		BIT(6)

#define I2C_APB_CLK_OFFSET		0x228
#define I2C_APB_CLK_ENABLE_BIT		BIT(31)

static int pm_system_reset_check(u32 type, u32 reason)
{
	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
		return 1;
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
		return 255;
	default:
		break;
	}

	return 0;
}

static int wait_pmu_pd_state(u32 mask)
{
	int count = 0;
	unsigned long addr = jh7110_inst.pmu_reg_base;
	u32 val;

	do {
		val = readl((void *)(addr + CURR_POWER_MODE));
		if (val == mask)
			return 0;

		sbi_timer_udelay(2);
		count += 1;
		if (count == TIMEOUT_COUNT)
			return SBI_ETIMEDOUT;
	} while (1);
}

static int shutdown_device_power_domain(void)
{
	unsigned long addr = jh7110_inst.pmu_reg_base;
	u32 curr_mode;
	int ret = 0;

	curr_mode = readl((void *)(addr + CURR_POWER_MODE));
	curr_mode &= DEVICE_PD_MASK;

	if (curr_mode) {
		writel(curr_mode, (void *)(addr + SW_TURN_OFF_POWER_MODE));
		writel(SW_MODE_ENCOURAGE_ON, (void *)(addr + SW_ENCOURAGE));
		writel(SW_MODE_ENCOURAGE_DIS_LO, (void *)(addr + SW_ENCOURAGE));
		writel(SW_MODE_ENCOURAGE_DIS_HI, (void *)(addr + SW_ENCOURAGE));
		ret = wait_pmu_pd_state(SYSTOP_CPU_PD_MASK);
		if (ret)
			sbi_printf("%s shutdown device power %x error\n",
				   __func__, curr_mode);
	}
	return ret;
}

static void pmic_ops(struct pmic *pmic, int type)
{
	int ret = 0;
	u8 val;

	ret = shutdown_device_power_domain();

	if (ret)
		return;

	if (!sbi_strcmp("stf,axp15060-regulator", pmic->compatible)) {
		ret = i2c_adapter_reg_read(pmic->adapter, pmic->dev_addr,
					   AXP15060_POWER_REG, &val);

		if (ret) {
			sbi_printf("%s: cannot read pmic power register\n",
				   __func__);
			return;
		}

		val |= AXP15060_POWER_OFF_BIT;
		if (type == SBI_SRST_RESET_TYPE_SHUTDOWN)
			val |= AXP15060_POWER_OFF_BIT;
		else
			val |= AXP15060_RESET_BIT;

		ret = i2c_adapter_reg_write(pmic->adapter, pmic->dev_addr,
					    AXP15060_POWER_REG, val);
		if (ret)
			sbi_printf("%s: cannot write pmic power register\n",
				   __func__);
	}
}

static void pmic_i2c_clk_enable(void)
{
	unsigned long clock_base;
	unsigned int val;

	clock_base = jh7110_inst.clk_reg_base +
		I2C_APB_CLK_OFFSET +
		(jh7110_inst.i2c_index << 2);

	val = readl((void *)clock_base);

	if (!val)
		writel(I2C_APB_CLK_ENABLE_BIT, (void *)clock_base);
}

static void pm_system_reset(u32 type, u32 reason)
{
	if (pmic_inst.adapter) {
		switch (type) {
		case SBI_SRST_RESET_TYPE_SHUTDOWN:
		case SBI_SRST_RESET_TYPE_COLD_REBOOT:
			/* i2c clk may be disabled by kernel driver */
			pmic_i2c_clk_enable();
			pmic_ops(&pmic_inst, type);
			break;
		default:
			break;
		}
	}

	sbi_hart_hang();
}

static struct sbi_system_reset_device pm_reset = {
	.name = "pm-reset",
	.system_reset_check = pm_system_reset_check,
	.system_reset = pm_system_reset
};

static int pm_reset_init(void *fdt, int nodeoff,
			 const struct fdt_match *match)
{
	int rc;
	int i2c_bus;
	struct i2c_adapter *adapter;
	u64 addr;

	rc = fdt_get_node_addr_size(fdt, nodeoff, 0, &addr, NULL);
	if (rc)
		return rc;

	pmic_inst.dev_addr = addr;
	pmic_inst.compatible = match->compatible;

	i2c_bus = fdt_parent_offset(fdt, nodeoff);
	if (i2c_bus < 0)
		return i2c_bus;

	/* i2c adapter get */
	rc = fdt_i2c_adapter_get(fdt, i2c_bus, &adapter);
	if (rc)
		return rc;

	pmic_inst.adapter = adapter;

	sbi_system_reset_add_device(&pm_reset);

	return 0;
}

static const struct fdt_match pm_reset_match[] = {
	{ .compatible = "stf,axp15060-regulator", .data = (void *)true },
	{ },
};

static struct fdt_reset fdt_reset_pmic = {
	.match_table = pm_reset_match,
	.init = pm_reset_init,
};

static int starfive_jh7110_inst_init(void *fdt)
{
	int noff, rc = 0;
	const char *name;
	u64 addr;

	noff = fdt_node_offset_by_compatible(fdt, -1, "starfive,jh7110-pmu");
	if (-1 < noff) {
		rc = fdt_get_node_addr_size(fdt, noff, 0, &addr, NULL);
		if (rc)
			goto err;
		jh7110_inst.pmu_reg_base = addr;
	}

	noff = fdt_node_offset_by_compatible(fdt, -1, "starfive,jh7110-clkgen");
	if (-1 < noff) {
		rc = fdt_get_node_addr_size(fdt, noff, 0, &addr, NULL);
		if (rc)
			goto err;
		jh7110_inst.clk_reg_base = addr;
	}

	if (pmic_inst.adapter) {
		name = fdt_get_name(fdt, pmic_inst.adapter->id, NULL);
		if (!sbi_strncmp(name, "i2c", 3))
			jh7110_inst.i2c_index = name[3] - '0';
		else
			rc = SBI_EINVAL;
	}
err:
	return rc;
}

static int starfive_jh7110_final_init(bool cold_boot,
				      const struct fdt_match *match)
{
	void *fdt = fdt_get_address();

	if (cold_boot) {
		fdt_reset_driver_init(fdt, &fdt_reset_pmic);
		return starfive_jh7110_inst_init(fdt);
	}

	return 0;
}

static bool starfive_jh7110_cold_boot_allowed(u32 hartid,
				   const struct fdt_match *match)
{
	if (selected_hartid != -1)
		return (selected_hartid == hartid);

	return true;
}

static void starfive_jh7110_fw_init(void *fdt, const struct fdt_match *match)
{
	const fdt32_t *val;
	int len, coff;

	coff = fdt_path_offset(fdt, "/chosen");
	if (-1 < coff) {
		val = fdt_getprop(fdt, coff, "starfive,boot-hart-id", &len);
		if (val && len >= sizeof(fdt32_t))
			selected_hartid = (u32) fdt32_to_cpu(*val);
	}
}

static const struct fdt_match starfive_jh7110_match[] = {
	{ .compatible = "starfive,jh7110" },
	{ },
};

const struct platform_override starfive_jh7110 = {
	.match_table = starfive_jh7110_match,
	.cold_boot_allowed = starfive_jh7110_cold_boot_allowed,
	.fw_init = starfive_jh7110_fw_init,
	.final_init = starfive_jh7110_final_init,
};
