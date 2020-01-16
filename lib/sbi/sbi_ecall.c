/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_tlb.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_unpriv.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_version.h>
#include <sbi/riscv_asm.h>

#define SBI_ECALL_VERSION_MAJOR 0
#define SBI_ECALL_VERSION_MINOR 2
#define SBI_OPENSBI_IMPID 1

u16 sbi_ecall_version_major(void)
{
	return SBI_ECALL_VERSION_MAJOR;
}

u16 sbi_ecall_version_minor(void)
{
	return SBI_ECALL_VERSION_MINOR;
}

static int sbi_load_hart_mask_unpriv(struct sbi_scratch *scratch, ulong *pmask,
				     ulong *hmask, struct sbi_trap_info *uptrap)
{
	ulong mask = 0;

	if (pmask) {
		mask = sbi_load_ulong(pmask, scratch, uptrap);
		if (uptrap->cause)
			return SBI_ETRAP;
	} else {
		mask = sbi_hart_available_mask();
	}
	*hmask = mask;
	return 0;
}

static int sbi_ecall_base_probe(struct sbi_scratch *scratch,
				unsigned long extid,
				unsigned long *out_val)
{
	struct sbi_ecall_extension *ext;

	ext = sbi_ecall_find_extension(extid);
	if (!ext) {
		*out_val = 0;
		return 0;
	}

	if (ext->probe)
		return ext->probe(scratch, extid, out_val);

	*out_val = 1;
	return 0;
}

static int sbi_ecall_base_handler(struct sbi_scratch *scratch,
				  unsigned long extid, unsigned long funcid,
				  unsigned long *args, unsigned long *out_val,
				  struct sbi_trap_info *out_trap)
{
	int ret = 0;

	switch (funcid) {
	case SBI_EXT_BASE_GET_SPEC_VERSION:
		*out_val = (SBI_ECALL_VERSION_MAJOR <<
			   SBI_SPEC_VERSION_MAJOR_OFFSET) &
			   (SBI_SPEC_VERSION_MAJOR_MASK <<
			    SBI_SPEC_VERSION_MAJOR_OFFSET);
		*out_val = *out_val | SBI_ECALL_VERSION_MINOR;
		break;
	case SBI_EXT_BASE_GET_IMP_ID:
		*out_val = SBI_OPENSBI_IMPID;
		break;
	case SBI_EXT_BASE_GET_IMP_VERSION:
		*out_val = OPENSBI_VERSION;
		break;
	case SBI_EXT_BASE_GET_MVENDORID:
		*out_val = csr_read(CSR_MVENDORID);
		break;
	case SBI_EXT_BASE_GET_MARCHID:
		*out_val = csr_read(CSR_MARCHID);
		break;
	case SBI_EXT_BASE_GET_MIMPID:
		*out_val = csr_read(CSR_MIMPID);
		break;
	case SBI_EXT_BASE_PROBE_EXT:
		ret = sbi_ecall_base_probe(scratch, args[0], out_val);
		break;
	default:
		ret = SBI_ENOTSUPP;
	}

	return ret;
}

static struct sbi_ecall_extension ecall_base = {
	.extid_start = SBI_EXT_BASE,
	.extid_end = SBI_EXT_BASE,
	.handle = sbi_ecall_base_handler,
};

static int sbi_ecall_time_handler(struct sbi_scratch *scratch,
				  unsigned long extid, unsigned long funcid,
				  unsigned long *args, unsigned long *out_val,
				  struct sbi_trap_info *out_trap)
{
	int ret = 0;

	if (funcid == SBI_EXT_TIME_SET_TIMER) {
#if __riscv_xlen == 32
		sbi_timer_event_start(scratch,
				      (((u64)args[1] << 32) | (u64)args[0]));
#else
		sbi_timer_event_start(scratch, (u64)args[0]);
#endif
	} else
		ret = SBI_ENOTSUPP;

	return ret;
}

static struct sbi_ecall_extension ecall_time = {
	.extid_start = SBI_EXT_TIME,
	.extid_end = SBI_EXT_TIME,
	.handle = sbi_ecall_time_handler,
};

static int sbi_ecall_ipi_handler(struct sbi_scratch *scratch,
				 unsigned long extid, unsigned long funcid,
				 unsigned long *args, unsigned long *out_val,
				 struct sbi_trap_info *out_trap)
{
	int ret = 0;

	if (funcid == SBI_EXT_IPI_SEND_IPI)
		ret = sbi_ipi_send_smode(scratch, args[0], args[1]);
	else
		ret = SBI_ENOTSUPP;

	return ret;
}

static struct sbi_ecall_extension ecall_ipi = {
	.extid_start = SBI_EXT_IPI,
	.extid_end = SBI_EXT_IPI,
	.handle = sbi_ecall_ipi_handler,
};

static int sbi_ecall_rfence_handler(struct sbi_scratch *scratch,
				    unsigned long extid, unsigned long funcid,
				    unsigned long *args, unsigned long *out_val,
				    struct sbi_trap_info *out_trap)
{
	int ret = 0;
	struct sbi_tlb_info tlb_info;
	u32 source_hart = sbi_current_hartid();

	if (funcid >= SBI_EXT_RFENCE_REMOTE_HFENCE_GVMA &&
	    funcid <= SBI_EXT_RFENCE_REMOTE_HFENCE_VVMA_ASID)
		if (!misa_extension('H'))
			return SBI_ENOTSUPP;

	switch (funcid) {
	case SBI_EXT_RFENCE_REMOTE_FENCE_I:
		tlb_info.start  = 0;
		tlb_info.size  = 0;
		tlb_info.type  = SBI_ITLB_FLUSH;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_tlb_request(scratch, args[0], args[1], &tlb_info);
		break;
	case SBI_EXT_RFENCE_REMOTE_HFENCE_GVMA:
		tlb_info.start = (unsigned long)args[2];
		tlb_info.size  = (unsigned long)args[3];
		tlb_info.type  = SBI_TLB_FLUSH_GVMA;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_tlb_request(scratch, args[0], args[1], &tlb_info);
		break;
	case SBI_EXT_RFENCE_REMOTE_HFENCE_GVMA_VMID:
		tlb_info.start = (unsigned long)args[2];
		tlb_info.size  = (unsigned long)args[3];
		tlb_info.asid  = (unsigned long)args[4];
		tlb_info.type  = SBI_TLB_FLUSH_GVMA_VMID;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_tlb_request(scratch, args[0], args[1], &tlb_info);
		break;
	case SBI_EXT_RFENCE_REMOTE_HFENCE_VVMA:
		tlb_info.start = (unsigned long)args[2];
		tlb_info.size  = (unsigned long)args[3];
		tlb_info.type  = SBI_TLB_FLUSH_VVMA;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_tlb_request(scratch, args[0], args[1], &tlb_info);
		break;
	case SBI_EXT_RFENCE_REMOTE_HFENCE_VVMA_ASID:
		tlb_info.start = (unsigned long)args[2];
		tlb_info.size  = (unsigned long)args[3];
		tlb_info.asid  = (unsigned long)args[4];
		tlb_info.type  = SBI_TLB_FLUSH_VVMA_ASID;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_tlb_request(scratch, args[0], args[1], &tlb_info);
		break;
	case SBI_EXT_RFENCE_REMOTE_SFENCE_VMA:
		tlb_info.start = (unsigned long)args[2];
		tlb_info.size  = (unsigned long)args[3];
		tlb_info.type  = SBI_TLB_FLUSH_VMA;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_tlb_request(scratch, args[0], args[1], &tlb_info);
		break;
	case SBI_EXT_RFENCE_REMOTE_SFENCE_VMA_ASID:
		tlb_info.start = (unsigned long)args[2];
		tlb_info.size  = (unsigned long)args[3];
		tlb_info.asid  = (unsigned long)args[4];
		tlb_info.type  = SBI_TLB_FLUSH_VMA_ASID;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_tlb_request(scratch, args[0], args[1], &tlb_info);
		break;

	default:
		ret = SBI_ENOTSUPP;
	};

	return ret;
}

static struct sbi_ecall_extension ecall_rfence = {
	.extid_start = SBI_EXT_RFENCE,
	.extid_end = SBI_EXT_RFENCE,
	.handle = sbi_ecall_rfence_handler,
};

static int sbi_ecall_0_1_handler(struct sbi_scratch *scratch,
				 unsigned long extid, unsigned long funcid,
				 unsigned long *args, unsigned long *out_val,
				 struct sbi_trap_info *out_trap)
{
	int ret = 0;
	struct sbi_tlb_info tlb_info;
	u32 source_hart = sbi_current_hartid();
	ulong hmask = 0;

	switch (extid) {
	case SBI_EXT_0_1_SET_TIMER:
#if __riscv_xlen == 32
		sbi_timer_event_start(scratch,
				      (((u64)args[1] << 32) | (u64)args[0]));
#else
		sbi_timer_event_start(scratch, (u64)args[0]);
#endif
		break;
	case SBI_EXT_0_1_CONSOLE_PUTCHAR:
		sbi_putc(args[0]);
		break;
	case SBI_EXT_0_1_CONSOLE_GETCHAR:
		ret = sbi_getc();
		break;
	case SBI_EXT_0_1_CLEAR_IPI:
		sbi_ipi_clear_smode(scratch);
		break;
	case SBI_EXT_0_1_SEND_IPI:
		ret = sbi_load_hart_mask_unpriv(scratch, (ulong *)args[0],
						&hmask, out_trap);
		if (ret != SBI_ETRAP)
			ret = sbi_ipi_send_smode(scratch, hmask, 0);
		break;
	case SBI_EXT_0_1_REMOTE_FENCE_I:
		tlb_info.start  = 0;
		tlb_info.size  = 0;
		tlb_info.type  = SBI_ITLB_FLUSH;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_load_hart_mask_unpriv(scratch, (ulong *)args[0],
						&hmask, out_trap);
		if (ret != SBI_ETRAP)
			ret = sbi_tlb_request(scratch, hmask, 0, &tlb_info);
		break;
	case SBI_EXT_0_1_REMOTE_SFENCE_VMA:
		tlb_info.start = (unsigned long)args[1];
		tlb_info.size  = (unsigned long)args[2];
		tlb_info.type  = SBI_TLB_FLUSH_VMA;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_load_hart_mask_unpriv(scratch, (ulong *)args[0],
						&hmask, out_trap);
		if (ret != SBI_ETRAP)
			ret = sbi_tlb_request(scratch, hmask, 0, &tlb_info);
		break;
	case SBI_EXT_0_1_REMOTE_SFENCE_VMA_ASID:
		tlb_info.start = (unsigned long)args[1];
		tlb_info.size  = (unsigned long)args[2];
		tlb_info.asid  = (unsigned long)args[3];
		tlb_info.type  = SBI_TLB_FLUSH_VMA_ASID;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_load_hart_mask_unpriv(scratch, (ulong *)args[0],
						&hmask, out_trap);
		if (ret != SBI_ETRAP)
			ret = sbi_tlb_request(scratch, hmask, 0, &tlb_info);
		break;
	case SBI_EXT_0_1_SHUTDOWN:
		sbi_system_shutdown(scratch, 0);
		break;
	default:
		ret = SBI_ENOTSUPP;
	};

	return ret;
}

static struct sbi_ecall_extension ecall_0_1 = {
	.extid_start = SBI_EXT_0_1_SET_TIMER,
	.extid_end = SBI_EXT_0_1_SHUTDOWN,
	.handle = sbi_ecall_0_1_handler,
};

static int sbi_ecall_vendor_probe(struct sbi_scratch *scratch,
				  unsigned long extid,
				  unsigned long *out_val)
{
	*out_val = sbi_platform_vendor_ext_check(sbi_platform_ptr(scratch),
						 extid);
	return 0;
}

static int sbi_ecall_vendor_handler(struct sbi_scratch *scratch,
				    unsigned long extid, unsigned long funcid,
				    unsigned long *args, unsigned long *out_val,
				    struct sbi_trap_info *out_trap)
{
	return sbi_platform_vendor_ext_provider(sbi_platform_ptr(scratch),
						extid, funcid, args,
						out_val, out_trap);
}

static struct sbi_ecall_extension ecall_vendor = {
	.extid_start = SBI_EXT_VENDOR_START,
	.extid_end = SBI_EXT_VENDOR_END,
	.probe = sbi_ecall_vendor_probe,
	.handle = sbi_ecall_vendor_handler,
};

static SBI_LIST_HEAD(ecall_exts_list);

struct sbi_ecall_extension *sbi_ecall_find_extension(unsigned long extid)
{
	struct sbi_ecall_extension *t, *ret = NULL;

	sbi_list_for_each_entry(t, &ecall_exts_list, head) {
		if (t->extid_start <= extid && extid <= t->extid_end) {
			ret = t;
			break;
		}
	}

	return ret;
}

int sbi_ecall_register_extension(struct sbi_ecall_extension *ext)
{
	if (!ext || (ext->extid_end < ext->extid_start) || !ext->handle)
		return SBI_EINVAL;
	if (sbi_ecall_find_extension(ext->extid_start) ||
	    sbi_ecall_find_extension(ext->extid_end))
		return SBI_EINVAL;

	SBI_INIT_LIST_HEAD(&ext->head);
	sbi_list_add_tail(&ext->head, &ecall_exts_list);

	return 0;
}

void sbi_ecall_unregister_extension(struct sbi_ecall_extension *ext)
{
	bool found = FALSE;
	struct sbi_ecall_extension *t;

	if (!ext)
		return;

	sbi_list_for_each_entry(t, &ecall_exts_list, head) {
		if (t == ext) {
			found = TRUE;
			break;
		}
	}

	if (found)
		sbi_list_del_init(&ext->head);
}

int sbi_ecall_handler(u32 hartid, ulong mcause, struct sbi_trap_regs *regs,
		      struct sbi_scratch *scratch)
{
	int ret = 0;
	struct sbi_ecall_extension *ext;
	unsigned long extension_id = regs->a7;
	unsigned long func_id = regs->a6;
	struct sbi_trap_info trap = {0};
	unsigned long out_val;
	bool is_0_1_spec = 0;
	unsigned long args[6];

	args[0] = regs->a0;
	args[1] = regs->a1;
	args[2] = regs->a2;
	args[3] = regs->a3;
	args[4] = regs->a4;
	args[5] = regs->a5;

	ext = sbi_ecall_find_extension(extension_id);
	if (ext && ext->handle) {
		ret = ext->handle(scratch, extension_id, func_id,
				  args, &out_val, &trap);
		if (extension_id >= SBI_EXT_0_1_SET_TIMER &&
		    extension_id <= SBI_EXT_0_1_SHUTDOWN)
			is_0_1_spec = 1;
	} else {
		ret = SBI_ENOTSUPP;
	}

	if (ret == SBI_ETRAP) {
		trap.epc = regs->mepc;
		sbi_trap_redirect(regs, &trap, scratch);
	} else {
		/* This function should return non-zero value only in case of
		 * fatal error. However, there is no good way to distinguish
		 * between a fatal and non-fatal errors yet. That's why we treat
		 * every return value except ETRAP as non-fatal and just return
		 * accordingly for now. Once fatal errors are defined, that
		 * case should be handled differently.
		 */
		regs->mepc += 4;
		regs->a0 = ret;
		if (!is_0_1_spec)
			regs->a1 = out_val;
	}

	return 0;
}

int sbi_ecall_init(void)
{
	int ret;

	/* The order of below registrations is performance optimized */
	ret = sbi_ecall_register_extension(&ecall_time);
	if (ret)
		return ret;
	ret = sbi_ecall_register_extension(&ecall_rfence);
	if (ret)
		return ret;
	ret = sbi_ecall_register_extension(&ecall_ipi);
	if (ret)
		return ret;
	ret = sbi_ecall_register_extension(&ecall_base);
	if (ret)
		return ret;
	ret = sbi_ecall_register_extension(&ecall_0_1);
	if (ret)
		return ret;
	ret = sbi_ecall_register_extension(&ecall_vendor);
	if (ret)
		return ret;

	return 0;
}
