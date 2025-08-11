/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems, Inc.
 *
 * Author(s):
 *   Himanshu Chauhan <hchauhan@ventanamicro.com>
 */

#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_csr_detect.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_byteorder.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_dbtr.h>
#include <sbi/sbi_heap.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_asm.h>


/** Offset of pointer to HART's debug triggers info in scratch space */
static unsigned long hart_state_ptr_offset;

#define dbtr_get_hart_state_ptr(__scratch)				\
	sbi_scratch_read_type((__scratch), void *, hart_state_ptr_offset)

#define dbtr_thishart_state_ptr()				\
	dbtr_get_hart_state_ptr(sbi_scratch_thishart_ptr())

#define dbtr_set_hart_state_ptr(__scratch, __hart_state)		\
	sbi_scratch_write_type((__scratch), void *, hart_state_ptr_offset, \
			       (__hart_state))

#define INDEX_TO_TRIGGER(_index)					\
	({								\
		struct sbi_dbtr_trigger *__trg = NULL;			\
		struct sbi_dbtr_hart_triggers_state *__hs = NULL;	\
		__hs = dbtr_get_hart_state_ptr(sbi_scratch_thishart_ptr()); \
		__trg = &__hs->triggers[_index];			\
		(__trg);						\
	})

#define for_each_trig_entry(_base, _max, _etype, _entry)		\
	for (int _idx = 0; _entry = ((_etype *)_base + _idx),		\
	     _idx < _max;						\
	     _idx++, _entry = ((_etype *)_base + _idx))

#define DBTR_SHMEM_MAKE_PHYS(_p_hi, _p_lo) (_p_lo)

/* must call with hs != NULL */
static inline bool sbi_dbtr_shmem_disabled(
	struct sbi_dbtr_hart_triggers_state *hs)
{
	return (hs->shmem.phys_lo == SBI_DBTR_SHMEM_INVALID_ADDR &&
		hs->shmem.phys_hi == SBI_DBTR_SHMEM_INVALID_ADDR
		? true : false);
}

/* must call with hs != NULL */
static inline void sbi_dbtr_disable_shmem(
	struct sbi_dbtr_hart_triggers_state *hs)
{
	hs->shmem.phys_lo = SBI_DBTR_SHMEM_INVALID_ADDR;
	hs->shmem.phys_hi = SBI_DBTR_SHMEM_INVALID_ADDR;
}

/* must call with hs which is not disabled */
static inline void *hart_shmem_base(
	struct sbi_dbtr_hart_triggers_state *hs)
{
	return ((void *)(unsigned long)DBTR_SHMEM_MAKE_PHYS(
			hs->shmem.phys_hi, hs->shmem.phys_lo));
}

static void sbi_trigger_init(struct sbi_dbtr_trigger *trig,
			     unsigned long type_mask, unsigned long idx)
{
	trig->type_mask = type_mask;
	trig->state = 0;
	trig->tdata1 = 0;
	trig->tdata2 = 0;
	trig->tdata3 = 0;
	trig->index = idx;
}

static inline struct sbi_dbtr_trigger *sbi_alloc_trigger(void)
{
	int i;
	struct sbi_dbtr_trigger *f_trig = NULL;
	struct sbi_dbtr_hart_triggers_state *hart_state;

	hart_state = dbtr_thishart_state_ptr();
	if (!hart_state)
		return NULL;

	if (hart_state->available_trigs <= 0)
		return NULL;

	for (i = 0; i < hart_state->total_trigs; i++) {
		f_trig = INDEX_TO_TRIGGER(i);
		if (f_trig->state & RV_DBTR_BIT_MASK(TS, MAPPED))
			continue;
		hart_state->available_trigs--;
		break;
	}

	if (i == hart_state->total_trigs)
		return NULL;

	__set_bit(RV_DBTR_BIT(TS, MAPPED), &f_trig->state);

	return f_trig;
}

static inline void sbi_free_trigger(struct sbi_dbtr_trigger *trig)
{
	struct sbi_dbtr_hart_triggers_state *hart_state;

	if (trig == NULL)
		return;

	hart_state = dbtr_thishart_state_ptr();
	if (!hart_state)
		return;

	trig->state = 0;
	trig->tdata1 = 0;
	trig->tdata2 = 0;
	trig->tdata3 = 0;

	hart_state->available_trigs++;
}

int sbi_dbtr_init(struct sbi_scratch *scratch, bool coldboot)
{
	struct sbi_trap_info trap = {0};
	unsigned long tdata1;
	unsigned long val;
	int i;
	struct sbi_dbtr_hart_triggers_state *hart_state = NULL;

	if (!sbi_hart_has_extension(scratch, SBI_HART_EXT_SDTRIG))
		return SBI_SUCCESS;

	if (coldboot) {
		hart_state_ptr_offset = sbi_scratch_alloc_type_offset(void *);
		if (!hart_state_ptr_offset)
			return SBI_ENOMEM;
	}

	hart_state = dbtr_get_hart_state_ptr(scratch);
	if (!hart_state) {
		hart_state = sbi_zalloc(sizeof(*hart_state));
		if (!hart_state)
			return SBI_ENOMEM;
		hart_state->hartid = current_hartid();
		dbtr_set_hart_state_ptr(scratch, hart_state);
	}

	/* disable the shared memory */
	sbi_dbtr_disable_shmem(hart_state);

	/* Skip probing triggers if already probed */
	if (hart_state->probed)
		goto _probed;

	for (i = 0; i < RV_MAX_TRIGGERS; i++) {
		csr_write_allowed(CSR_TSELECT, &trap, i);
		if (trap.cause)
			break;

		val = csr_read_allowed(CSR_TSELECT, &trap);
		if (trap.cause)
			break;

		/*
		 * Read back tselect and check that it contains the
		 * written value
		 */
		if (val != i)
			break;

		val = csr_read_allowed(CSR_TINFO, &trap);
		if (trap.cause) {
			/*
			 * If reading tinfo caused an exception, the
			 * debugger must read tdata1 to discover the
			 * type.
			 */
			tdata1 = csr_read_allowed(CSR_TDATA1,
						  &trap);
			if (trap.cause)
				break;

			if (TDATA1_GET_TYPE(tdata1) == 0)
				break;

			sbi_trigger_init(INDEX_TO_TRIGGER(i),
					 BIT(TDATA1_GET_TYPE(tdata1)),
					 i);
			hart_state->total_trigs++;
		} else {
			if (val == 1)
				break;

			sbi_trigger_init(INDEX_TO_TRIGGER(i), val, i);
			hart_state->total_trigs++;
		}
	}

	hart_state->probed = 1;

 _probed:
	hart_state->available_trigs = hart_state->total_trigs;

	return SBI_SUCCESS;
}

int sbi_dbtr_get_total_triggers(void)
{
	struct sbi_dbtr_hart_triggers_state *hs;
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	/*
	 * This function may be used during ecall registration.
	 * By that time the debug trigger module might not be
	 * initialized. If the extension is not supported, report
	 * number of triggers as 0.
	 */
	if (!sbi_hart_has_extension(scratch, SBI_HART_EXT_SDTRIG))
		return 0;

	hs = dbtr_thishart_state_ptr();
	if (!hs)
		return 0;

	return hs->total_trigs;
}

int sbi_dbtr_setup_shmem(const struct sbi_domain *dom, unsigned long smode,
			 unsigned long shmem_phys_lo,
			 unsigned long shmem_phys_hi)
{
	struct sbi_dbtr_hart_triggers_state *hart_state;

	if (dom && !sbi_domain_is_assigned_hart(dom, current_hartindex())) {
		sbi_dprintf("%s: calling hart not assigned to this domain\n",
			   __func__);
		return SBI_ERR_DENIED;
	}

	hart_state = dbtr_thishart_state_ptr();
	if (!hart_state)
		return SBI_ERR_FAILED;

	/* call is to disable shared memory */
	if (shmem_phys_lo == SBI_DBTR_SHMEM_INVALID_ADDR
	    && shmem_phys_hi == SBI_DBTR_SHMEM_INVALID_ADDR) {
		sbi_dbtr_disable_shmem(hart_state);
		return SBI_SUCCESS;
	}

	/* the shared memory must be disabled on this hart */
	if (!sbi_dbtr_shmem_disabled(hart_state))
		return SBI_ERR_ALREADY_AVAILABLE;

	/* lower physical address must be XLEN/8 bytes aligned */
	if (shmem_phys_lo & SBI_DBTR_SHMEM_ALIGN_MASK)
		return SBI_ERR_INVALID_PARAM;

	/*
	* On RV32, the M-mode can only access the first 4GB of
	* the physical address space because M-mode does not have
	* MMU to access full 34-bit physical address space.
	* So fail if the upper 32 bits of the physical address
	* is non-zero on RV32.
	*
	* On RV64, kernel sets upper 64bit address part to zero.
	* So fail if the upper 64bit of the physical address
	* is non-zero on RV64.
	*/
	if (shmem_phys_hi)
		return SBI_EINVALID_ADDR;

	if (dom && !sbi_domain_check_addr(dom,
		  DBTR_SHMEM_MAKE_PHYS(shmem_phys_hi, shmem_phys_lo), smode,
		  SBI_DOMAIN_READ | SBI_DOMAIN_WRITE))
		return SBI_ERR_INVALID_ADDRESS;

	hart_state->shmem.phys_lo = shmem_phys_lo;
	hart_state->shmem.phys_hi = shmem_phys_hi;

	return SBI_SUCCESS;
}

static void dbtr_trigger_setup(struct sbi_dbtr_trigger *trig,
			       struct sbi_dbtr_data_msg *recv)
{
	unsigned long tdata1;

	if (!trig)
		return;

	trig->tdata1 = lle_to_cpu(recv->tdata1);
	trig->tdata2 = lle_to_cpu(recv->tdata2);
	trig->tdata3 = lle_to_cpu(recv->tdata3);

	tdata1 = lle_to_cpu(recv->tdata1);

	trig->state = 0;

	__set_bit(RV_DBTR_BIT(TS, MAPPED), &trig->state);

	SET_TRIG_HW_INDEX(trig->state, trig->index);

	switch (TDATA1_GET_TYPE(tdata1)) {
	case RISCV_DBTR_TRIG_MCONTROL:
		if (__test_bit(RV_DBTR_BIT(MC, U), &tdata1))
			__set_bit(RV_DBTR_BIT(TS, U), &trig->state);

		if (__test_bit(RV_DBTR_BIT(MC, S), &tdata1))
			__set_bit(RV_DBTR_BIT(TS, S), &trig->state);
		break;
	case RISCV_DBTR_TRIG_MCONTROL6:
		if (__test_bit(RV_DBTR_BIT(MC6, U), &tdata1))
			__set_bit(RV_DBTR_BIT(TS, U), &trig->state);

		if (__test_bit(RV_DBTR_BIT(MC6, S), &tdata1))
			__set_bit(RV_DBTR_BIT(TS, S), &trig->state);

		if (__test_bit(RV_DBTR_BIT(MC6, VU), &tdata1))
			__set_bit(RV_DBTR_BIT(TS, VU), &trig->state);

		if (__test_bit(RV_DBTR_BIT(MC6, VS), &tdata1))
			__set_bit(RV_DBTR_BIT(TS, VS), &trig->state);
		break;
	case RISCV_DBTR_TRIG_ICOUNT:
		if (__test_bit(RV_DBTR_BIT(ICOUNT, U), &tdata1))
			__set_bit(RV_DBTR_BIT(TS, U), &trig->state);

		if (__test_bit(RV_DBTR_BIT(ICOUNT, S), &tdata1))
			__set_bit(RV_DBTR_BIT(TS, S), &trig->state);

		if (__test_bit(RV_DBTR_BIT(ICOUNT, VU), &tdata1))
			__set_bit(RV_DBTR_BIT(TS, VU), &trig->state);

		if (__test_bit(RV_DBTR_BIT(ICOUNT, VS), &tdata1))
			__set_bit(RV_DBTR_BIT(TS, VS), &trig->state);
		break;
	default:
		sbi_dprintf("%s: Unknown type (tdata1: 0x%lx Type: %ld)\n",
			    __func__, tdata1, TDATA1_GET_TYPE(tdata1));
		break;
	}
}

static inline void update_bit(unsigned long new, int nr, volatile unsigned long *addr)
{
	if (new)
		__set_bit(nr, addr);
	else
		__clear_bit(nr, addr);
}

static void dbtr_trigger_enable(struct sbi_dbtr_trigger *trig)
{
	unsigned long state;
	unsigned long tdata1;

	if (!trig || !(trig->state & RV_DBTR_BIT_MASK(TS, MAPPED)))
		return;

	state = trig->state;
	tdata1 = trig->tdata1;

	switch (TDATA1_GET_TYPE(tdata1)) {
	case RISCV_DBTR_TRIG_MCONTROL:
		update_bit(state & RV_DBTR_BIT_MASK(TS, U),
			   RV_DBTR_BIT(MC, U), &trig->tdata1);
		update_bit(state & RV_DBTR_BIT_MASK(TS, S),
			   RV_DBTR_BIT(MC, S), &trig->tdata1);
		break;
	case RISCV_DBTR_TRIG_MCONTROL6:
		update_bit(state & RV_DBTR_BIT_MASK(TS, VU),
			   RV_DBTR_BIT(MC6, VU), &trig->tdata1);
		update_bit(state & RV_DBTR_BIT_MASK(TS, VS),
			   RV_DBTR_BIT(MC6, VS), &trig->tdata1);
		update_bit(state & RV_DBTR_BIT_MASK(TS, U),
			   RV_DBTR_BIT(MC6, U), &trig->tdata1);
		update_bit(state & RV_DBTR_BIT_MASK(TS, S),
			   RV_DBTR_BIT(MC6, S), &trig->tdata1);
		break;
	case RISCV_DBTR_TRIG_ICOUNT:
		update_bit(state & RV_DBTR_BIT_MASK(TS, VU),
			   RV_DBTR_BIT(ICOUNT, VU), &trig->tdata1);
		update_bit(state & RV_DBTR_BIT_MASK(TS, VS),
			   RV_DBTR_BIT(ICOUNT, VS), &trig->tdata1);
		update_bit(state & RV_DBTR_BIT_MASK(TS, U),
			   RV_DBTR_BIT(ICOUNT, U), &trig->tdata1);
		update_bit(state & RV_DBTR_BIT_MASK(TS, S),
			   RV_DBTR_BIT(ICOUNT, S), &trig->tdata1);
		break;
	default:
		break;
	}

	/*
	 * RISC-V Debug Support v1.0.0 section 5.5:
	 * Debugger cannot simply set a trigger by writing tdata1, then tdata2,
	 * etc. The current value of tdata2 might not be legal with the new
	 * value of tdata1. To help with this situation, it is guaranteed that
	 * writing 0 to tdata1 disables the trigger, and leaves it in a state
	 * where tdata2 and tdata3 can be written with any value that makes
	 * sense for any trigger type supported by this trigger.
	 */
	csr_write(CSR_TSELECT, trig->index);
	csr_write(CSR_TDATA1, 0x0);
	csr_write(CSR_TDATA2, trig->tdata2);
	csr_write(CSR_TDATA1, trig->tdata1);
}

static void dbtr_trigger_disable(struct sbi_dbtr_trigger *trig)
{
	unsigned long tdata1;

	if (!trig || !(trig->state & RV_DBTR_BIT_MASK(TS, MAPPED)))
		return;

	tdata1 = trig->tdata1;

	switch (TDATA1_GET_TYPE(tdata1)) {
	case RISCV_DBTR_TRIG_MCONTROL:
		__clear_bit(RV_DBTR_BIT(MC, U), &trig->tdata1);
		__clear_bit(RV_DBTR_BIT(MC, S), &trig->tdata1);
		break;
	case RISCV_DBTR_TRIG_MCONTROL6:
		__clear_bit(RV_DBTR_BIT(MC6, VU), &trig->tdata1);
		__clear_bit(RV_DBTR_BIT(MC6, VS), &trig->tdata1);
		__clear_bit(RV_DBTR_BIT(MC6, U), &trig->tdata1);
		__clear_bit(RV_DBTR_BIT(MC6, S), &trig->tdata1);
		break;
	case RISCV_DBTR_TRIG_ICOUNT:
		__clear_bit(RV_DBTR_BIT(ICOUNT, VU), &trig->tdata1);
		__clear_bit(RV_DBTR_BIT(ICOUNT, VS), &trig->tdata1);
		__clear_bit(RV_DBTR_BIT(ICOUNT, U), &trig->tdata1);
		__clear_bit(RV_DBTR_BIT(ICOUNT, S), &trig->tdata1);
		break;
	default:
		break;
	}

	csr_write(CSR_TSELECT, trig->index);
	csr_write(CSR_TDATA1, trig->tdata1);
}

static void dbtr_trigger_clear(struct sbi_dbtr_trigger *trig)
{
	if (!trig || !(trig->state & RV_DBTR_BIT_MASK(TS, MAPPED)))
		return;

	csr_write(CSR_TSELECT, trig->index);
	csr_write(CSR_TDATA1, 0x0);
	csr_write(CSR_TDATA2, 0x0);
}

static int dbtr_trigger_supported(unsigned long type)
{
	switch (type) {
	case RISCV_DBTR_TRIG_MCONTROL:
	case RISCV_DBTR_TRIG_MCONTROL6:
	case RISCV_DBTR_TRIG_ICOUNT:
		return 1;
	default:
		break;
	}

	return 0;
}

static int dbtr_trigger_valid(unsigned long type, unsigned long tdata)
{
	switch (type) {
	case RISCV_DBTR_TRIG_MCONTROL:
		if (!(tdata & RV_DBTR_BIT_MASK(MC, DMODE)) &&
		    !(tdata & RV_DBTR_BIT_MASK(MC, M)))
			return 1;
		break;
	case RISCV_DBTR_TRIG_MCONTROL6:
		if (!(tdata & RV_DBTR_BIT_MASK(MC6, DMODE)) &&
		    !(tdata & RV_DBTR_BIT_MASK(MC6, M)))
			return 1;
		break;
	case RISCV_DBTR_TRIG_ICOUNT:
		if (!(tdata & RV_DBTR_BIT_MASK(ICOUNT, DMODE)) &&
		    !(tdata & RV_DBTR_BIT_MASK(ICOUNT, M)))
			return 1;
		break;
	default:
		break;
	}

	return 0;
}

int sbi_dbtr_num_trig(unsigned long data, unsigned long *out)
{
	unsigned long type = TDATA1_GET_TYPE(data);
	u32 hartid = current_hartid();
	unsigned long total = 0;
	struct sbi_dbtr_trigger *trig;
	int i;
	struct sbi_dbtr_hart_triggers_state *hs;

	hs = dbtr_thishart_state_ptr();
	if (!hs)
		return SBI_ERR_FAILED;

	if (data == 0) {
		*out = hs->total_trigs;
		return SBI_SUCCESS;
	}

	for (i = 0; i < hs->total_trigs; i++) {
		trig = INDEX_TO_TRIGGER(i);

		if (__test_bit(type, &trig->type_mask))
			total++;
	}

	sbi_dprintf("%s: hart%d: total triggers of type %lu: %lu\n",
		    __func__, hartid, type, total);

	*out = total;
	return SBI_SUCCESS;
}

int sbi_dbtr_read_trig(unsigned long smode,
		       unsigned long trig_idx_base, unsigned long trig_count)
{
	struct sbi_dbtr_data_msg *xmit;
	struct sbi_dbtr_trigger *trig;
	union sbi_dbtr_shmem_entry *entry;
	void *shmem_base = NULL;
	struct sbi_dbtr_hart_triggers_state *hs = NULL;

	hs = dbtr_thishart_state_ptr();
	if (!hs)
		return SBI_ERR_FAILED;

	if (trig_idx_base >= hs->total_trigs ||
	    trig_idx_base + trig_count >= hs->total_trigs)
		return SBI_ERR_INVALID_PARAM;

	if (sbi_dbtr_shmem_disabled(hs))
		return SBI_ERR_NO_SHMEM;

	shmem_base = hart_shmem_base(hs);

	sbi_hart_map_saddr((unsigned long)shmem_base,
			   trig_count * sizeof(*entry));
	for_each_trig_entry(shmem_base, trig_count, typeof(*entry), entry) {
		xmit = &entry->data;
		trig = INDEX_TO_TRIGGER((_idx + trig_idx_base));
		csr_write(CSR_TSELECT, trig->index);
		trig->tdata1 = csr_read(CSR_TDATA1);
		trig->tdata2 = csr_read(CSR_TDATA2);
		trig->tdata3 = csr_read(CSR_TDATA3);
		xmit->tstate = cpu_to_lle(trig->state);
		xmit->tdata1 = cpu_to_lle(trig->tdata1);
		xmit->tdata2 = cpu_to_lle(trig->tdata2);
		xmit->tdata3 = cpu_to_lle(trig->tdata3);
	}
	sbi_hart_unmap_saddr();

	return SBI_SUCCESS;
}

int sbi_dbtr_install_trig(unsigned long smode,
			  unsigned long trig_count, unsigned long *out)
{
	void *shmem_base = NULL;
	union sbi_dbtr_shmem_entry *entry;
	struct sbi_dbtr_data_msg *recv;
	struct sbi_dbtr_id_msg *xmit;
	unsigned long ctrl;
	struct sbi_dbtr_trigger *trig;
	struct sbi_dbtr_hart_triggers_state *hs = NULL;

	hs = dbtr_thishart_state_ptr();
	if (!hs)
		return SBI_ERR_FAILED;

	if (sbi_dbtr_shmem_disabled(hs))
		return SBI_ERR_NO_SHMEM;

	shmem_base = hart_shmem_base(hs);
	sbi_hart_map_saddr((unsigned long)shmem_base,
			   trig_count * sizeof(*entry));

	/* Check requested triggers configuration */
	for_each_trig_entry(shmem_base, trig_count, typeof(*entry), entry) {
		recv = (struct sbi_dbtr_data_msg *)(&entry->data);
		ctrl = recv->tdata1;

		if (!dbtr_trigger_supported(TDATA1_GET_TYPE(ctrl))) {
			*out = _idx;
			sbi_hart_unmap_saddr();
			return SBI_ERR_FAILED;
		}

		if (!dbtr_trigger_valid(TDATA1_GET_TYPE(ctrl), ctrl)) {
			*out = _idx;
			sbi_hart_unmap_saddr();
			return SBI_ERR_FAILED;
		}
	}

	if (hs->available_trigs < trig_count) {
		*out = hs->available_trigs;
		sbi_hart_unmap_saddr();
		return SBI_ERR_FAILED;
	}

	/* Install triggers */
	for_each_trig_entry(shmem_base, trig_count, typeof(*entry), entry) {
		/*
		 * Since we have already checked if enough triggers are
		 * available, trigger allocation must succeed.
		 */
		trig = sbi_alloc_trigger();

		recv = (struct sbi_dbtr_data_msg *)(&entry->data);
		xmit = (struct sbi_dbtr_id_msg *)(&entry->id);

		dbtr_trigger_setup(trig,  recv);
		dbtr_trigger_enable(trig);
		xmit->idx = cpu_to_lle(trig->index);

	}
	sbi_hart_unmap_saddr();

	return SBI_SUCCESS;
}

int sbi_dbtr_uninstall_trig(unsigned long trig_idx_base,
			    unsigned long trig_idx_mask)
{
	unsigned long trig_mask = trig_idx_mask << trig_idx_base;
	unsigned long idx = trig_idx_base;
	struct sbi_dbtr_trigger *trig;
	struct sbi_dbtr_hart_triggers_state *hs;

	hs = dbtr_thishart_state_ptr();
	if (!hs)
		return SBI_ERR_FAILED;

	for_each_set_bit_from(idx, &trig_mask, hs->total_trigs) {
		trig = INDEX_TO_TRIGGER(idx);
		if (!(trig->state & RV_DBTR_BIT_MASK(TS, MAPPED)))
			return SBI_ERR_INVALID_PARAM;

		dbtr_trigger_clear(trig);

		sbi_free_trigger(trig);
	}

	return SBI_SUCCESS;
}

int sbi_dbtr_enable_trig(unsigned long trig_idx_base,
			 unsigned long trig_idx_mask)
{
	unsigned long trig_mask = trig_idx_mask << trig_idx_base;
	unsigned long idx = trig_idx_base;
	struct sbi_dbtr_trigger *trig;
	struct sbi_dbtr_hart_triggers_state *hs;

	hs = dbtr_thishart_state_ptr();
	if (!hs)
		return SBI_ERR_FAILED;

	for_each_set_bit_from(idx, &trig_mask, hs->total_trigs) {
		trig = INDEX_TO_TRIGGER(idx);
		sbi_dprintf("%s: enable trigger %lu\n", __func__, idx);
		dbtr_trigger_enable(trig);
	}

	return SBI_SUCCESS;
}

int sbi_dbtr_update_trig(unsigned long smode,
			 unsigned long trig_count)
{
	unsigned long trig_idx;
	struct sbi_dbtr_trigger *trig;
	union sbi_dbtr_shmem_entry *entry;
	void *shmem_base = NULL;
	struct sbi_dbtr_hart_triggers_state *hs = NULL;

	hs = dbtr_thishart_state_ptr();
	if (!hs)
		return SBI_ERR_FAILED;

	if (sbi_dbtr_shmem_disabled(hs))
		return SBI_ERR_NO_SHMEM;

	shmem_base = hart_shmem_base(hs);

	if (trig_count >= hs->total_trigs)
		return SBI_ERR_BAD_RANGE;

	for_each_trig_entry(shmem_base, trig_count, typeof(*entry), entry) {
		sbi_hart_map_saddr((unsigned long)entry, sizeof(*entry));
		trig_idx = entry->id.idx;

		if (trig_idx >= hs->total_trigs) {
			sbi_hart_unmap_saddr();
			return SBI_ERR_INVALID_PARAM;
		}

		trig = INDEX_TO_TRIGGER(trig_idx);

		if (!(trig->state & RV_DBTR_BIT_MASK(TS, MAPPED))) {
			sbi_hart_unmap_saddr();
			return SBI_ERR_FAILED;
		}

		dbtr_trigger_setup(trig, &entry->data);
		sbi_hart_unmap_saddr();
		dbtr_trigger_enable(trig);
	}

	return SBI_SUCCESS;
}

int sbi_dbtr_disable_trig(unsigned long trig_idx_base,
			  unsigned long trig_idx_mask)
{
	unsigned long trig_mask = trig_idx_mask << trig_idx_base;
	unsigned long idx = trig_idx_base;
	struct sbi_dbtr_trigger *trig;
	struct sbi_dbtr_hart_triggers_state *hs;

	hs = dbtr_thishart_state_ptr();
	if (!hs)
		return SBI_ERR_FAILED;

	for_each_set_bit_from(idx, &trig_mask, hs->total_trigs) {
		trig = INDEX_TO_TRIGGER(idx);
		dbtr_trigger_disable(trig);
	}

	return SBI_SUCCESS;
}
