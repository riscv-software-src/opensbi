/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Rivos Inc.
 *
 * Authors:
 *   Clément Léger <cleger@rivosinc.com>
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_bitmap.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_types.h>

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>

/** Offset of pointer to FWFT HART state in scratch space */
static unsigned long fwft_ptr_offset;

#define fwft_get_hart_state_ptr(__scratch)				\
	sbi_scratch_read_type((__scratch), void *, fwft_ptr_offset)

#define fwft_thishart_state_ptr()					\
	fwft_get_hart_state_ptr(sbi_scratch_thishart_ptr())

#define fwft_set_hart_state_ptr(__scratch, __phs)			\
	sbi_scratch_write_type((__scratch), void *, fwft_ptr_offset, (__phs))

#define MIS_DELEG (1UL << CAUSE_MISALIGNED_LOAD | 1UL << CAUSE_MISALIGNED_STORE)

struct fwft_config;

struct fwft_feature {
	enum sbi_fwft_feature_t id;
	int (*supported)(struct fwft_config *conf);
	int (*set)(struct fwft_config *conf, unsigned long value);
	int (*get)(struct fwft_config *conf, unsigned long *value);
};

struct fwft_config {
	const struct fwft_feature *feature;
	unsigned long flags;
};

struct fwft_hart_state {
	unsigned int config_count;
	struct fwft_config configs[];
};

static const unsigned long fwft_defined_features[] = {
	SBI_FWFT_MISALIGNED_EXC_DELEG,
	SBI_FWFT_LANDING_PAD,
	SBI_FWFT_SHADOW_STACK,
	SBI_FWFT_DOUBLE_TRAP,
	SBI_FWFT_PTE_AD_HW_UPDATING,
};

static bool fwft_is_defined_feature(enum sbi_fwft_feature_t feature)
{
	int i;

	for (i = 0; i < array_size(fwft_defined_features); i++) {
		if (fwft_defined_features[i] == feature)
			return true;
	}

	return false;
}

static int fwft_misaligned_delegation_supported(struct fwft_config *conf)
{
	if (!misa_extension('S'))
		return SBI_ENOTSUPP;

	return SBI_OK;
}

static int fwft_set_misaligned_delegation(struct fwft_config *conf,
					 unsigned long value)
{
	if (value == 1)
		csr_set(CSR_MEDELEG, MIS_DELEG);
	else if (value == 0)
		csr_clear(CSR_MEDELEG, MIS_DELEG);
	else
		return SBI_EINVAL;

	return SBI_OK;
}

static int fwft_get_misaligned_delegation(struct fwft_config *conf,
					 unsigned long *value)
{
	*value = (csr_read(CSR_MEDELEG) & MIS_DELEG) != 0;

	return SBI_OK;
}

static int fwft_adue_supported(struct fwft_config *conf)
{
	if (!sbi_hart_has_extension(sbi_scratch_thishart_ptr(),
				    SBI_HART_EXT_SVADU))
		return SBI_ENOTSUPP;

	return SBI_OK;
}

static int fwft_set_adue(struct fwft_config *conf, unsigned long value)
{
	if (value == 1)
#if __riscv_xlen == 32
		csr_set(CSR_MENVCFGH, ENVCFG_ADUE >> 32);
#else
		csr_set(CSR_MENVCFG, ENVCFG_ADUE);
#endif
	else if (value == 0)
#if __riscv_xlen == 32
		csr_clear(CSR_MENVCFGH, ENVCFG_ADUE >> 32);
#else
		csr_clear(CSR_MENVCFG, ENVCFG_ADUE);
#endif
	else
		return SBI_EINVAL;

	return SBI_OK;
}

static int fwft_get_adue(struct fwft_config *conf, unsigned long *value)
{
	unsigned long cfg;

#if __riscv_xlen == 32
	cfg = csr_read(CSR_MENVCFGH) & (ENVCFG_ADUE >> 32);
#else
	cfg = csr_read(CSR_MENVCFG) & ENVCFG_ADUE;
#endif
	*value = cfg != 0;

	return SBI_OK;
}

static struct fwft_config* get_feature_config(enum sbi_fwft_feature_t feature)
{
	int i;
	struct fwft_hart_state *fhs = fwft_thishart_state_ptr();

	if (feature & SBI_FWFT_GLOBAL_FEATURE_BIT)
		return NULL;

	for (i = 0; i < fhs->config_count; i++){
		if (feature == fhs->configs[i].feature->id)
			return &fhs->configs[i];
	}

	return NULL;
}

static int fwft_get_feature(enum sbi_fwft_feature_t feature,
			    struct fwft_config **conf)
{
	int ret;
	struct fwft_config *tconf;

	tconf = get_feature_config(feature);
	if (!tconf) {
		if (fwft_is_defined_feature(feature))
			return SBI_ENOTSUPP;

		return SBI_EDENIED;
	}

	if (tconf->feature->supported) {
		ret = tconf->feature->supported(tconf);
		if (ret)
			return ret;
	}
	*conf = tconf;

	return SBI_SUCCESS;
}

int sbi_fwft_set(enum sbi_fwft_feature_t feature, unsigned long value,
		 unsigned long flags)
{
	int ret;
	struct fwft_config *conf;

	ret = fwft_get_feature(feature, &conf);
	if (ret)
		return ret;

	if ((flags & ~SBI_FWFT_SET_FLAG_LOCK) != 0)
		return SBI_EINVAL;

	if (conf->flags & SBI_FWFT_SET_FLAG_LOCK)
		return SBI_EDENIED;

	ret = conf->feature->set(conf, value);
	if (ret)
		return ret;

	conf->flags = flags;

	return SBI_OK;
}

int sbi_fwft_get(enum sbi_fwft_feature_t feature, unsigned long *out_val)
{
	int ret;
	struct fwft_config *conf;

	ret = fwft_get_feature(feature, &conf);
	if (ret)
		return ret;

	return conf->feature->get(conf, out_val);
}

static const struct fwft_feature features[] =
{
	{
		.id = SBI_FWFT_MISALIGNED_EXC_DELEG,
		.supported = fwft_misaligned_delegation_supported,
		.set = fwft_set_misaligned_delegation,
		.get = fwft_get_misaligned_delegation,
	},
	{
		.id = SBI_FWFT_PTE_AD_HW_UPDATING,
		.supported = fwft_adue_supported,
		.set = fwft_set_adue,
		.get = fwft_get_adue,
	},
};

int sbi_fwft_init(struct sbi_scratch *scratch, bool cold_boot)
{
	int i;
	struct fwft_hart_state *fhs;

	if (cold_boot) {
		fwft_ptr_offset = sbi_scratch_alloc_type_offset(void *);
		if (!fwft_ptr_offset)
			return SBI_ENOMEM;
	}

	fhs = fwft_get_hart_state_ptr(scratch);
	if (!fhs) {
		fhs = sbi_zalloc(sizeof(*fhs) + array_size(features) * sizeof(struct fwft_config));
		if (!fhs)
			return SBI_ENOMEM;

		fhs->config_count = array_size(features);
		for (i = 0; i < array_size(features); i++)
			fhs->configs[i].feature = &features[i];

		fwft_set_hart_state_ptr(scratch, fhs);
	}

	return 0;
}
