/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Tests of the SBI debug trigger extension (DBTR) of one hart.
 *
 * The expected results are those of the SBI specification, chapter "Debug
 * Triggers Extension". Where the specification allows more than one result,
 * the check accepts all of them. Where its text is evidently wrong, the
 * comment says what is checked instead:
 *
 * - Read, install and update fail with BAD_RANGE if the count exceeds
 *   trig_max. The text says ">= trig_max", which would make it impossible to
 *   read all triggers of a hart that has exactly trig_max triggers.
 *
 * The state of the extension is per hart: every hart has its own triggers
 * and its own shared memory. All harts run the same cases at the same time.
 * Only the boot hart executes the code that the triggers are set on, to see
 * that they fire.
 */

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_string.h>
#include "dbtr.h"
#include "ecall.h"
#include "hart.h"
#include "heap.h"
#include "sbitest.h"
#include "trap.h"

/* tdata1 of the Sdtrig specification */
#define XLEN		__riscv_xlen
#define TD1_TYPE_SHIFT	(XLEN - 4)
#define TD1_DMODE	(1UL << (XLEN - 5))
#define TD1_TYPE(val)	((val) >> TD1_TYPE_SHIFT)
#define TD1_WITH_TYPE(t) ((unsigned long)(t) << TD1_TYPE_SHIFT)

/* Trigger types */
#define TYPE_MCONTROL	2UL
#define TYPE_MCONTROL6	6UL
/* A type that Sdtrig reserves, no trigger can implement it */
#define TYPE_RESERVED	9UL

/* Bits that mcontrol and mcontrol6 have in common */
#define MC_EXEC		(1UL << 2)
#define MC_U		(1UL << 3)
#define MC_S		(1UL << 4)
#define MC_M		(1UL << 6)
#define MC_CHAIN	(1UL << 11)

/* trig_state */
#define STATE_MAPPED	(1UL << 0)
#define STATE_U		(1UL << 1)
#define STATE_S		(1UL << 2)
#define STATE_VU	(1UL << 3)
#define STATE_VS	(1UL << 4)
#define STATE_HAVE_HW	(1UL << 5)
#define STATE_RESERVED	(3UL << 6)

/* Four XLEN bit words per trigger, XLEN / 2 bytes */
#define ENTRY_WORDS	4
#define ENTRY_BYTES	(ENTRY_WORDS * sizeof(unsigned long))

struct ctx {
	unsigned long hartid;
	unsigned long trig_max;
	unsigned long *shmem;
	/* Type of the triggers used for the cases, 0 if there are none */
	unsigned long type;
	unsigned long other_type;
	unsigned long matching;
	unsigned int failed;
	int boot;
};

/* Number of breakpoints that were raised on a target */
static volatile unsigned long target_hits;

/*
 * Code that a trigger is set on. The first instruction is skipped by the trap
 * hook, the second one returns.
 */
asm(".text\n"
    ".balign 4\n"
    ".globl dbtr_target\n"
    "dbtr_target:\n"
    "\tnop\n"
    "\tret\n"
    ".balign 4\n"
    ".globl dbtr_target2\n"
    "dbtr_target2:\n"
    "\tnop\n"
    "\tret\n");
void dbtr_target(void);
void dbtr_target2(void);

int dbtr_trap_hook(struct trap_regs *regs)
{
	if (regs->scause != CAUSE_BREAKPOINT)
		return 0;
	if (regs->sepc != (unsigned long)dbtr_target &&
	    regs->sepc != (unsigned long)dbtr_target2)
		return 0;

	__atomic_fetch_add(&target_hits, 1, __ATOMIC_RELEASE);
	trap_skip_insn(regs);

	return 1;
}

/* Run fn and tell how often a trigger fired on it. Boot hart only. */
static unsigned long fires(void (*volatile fn)(void))
{
	unsigned long before = __atomic_load_n(&target_hits, __ATOMIC_ACQUIRE);

	fn();

	return __atomic_load_n(&target_hits, __ATOMIC_ACQUIRE) - before;
}

static void report(struct ctx *c, const char *name, int ok, long err, long val)
{
	if (ok) {
		/* Do not let all harts print the same lines */
		if (c->boot)
			sbi_printf("PASS %s\n", name);
		return;
	}

	c->failed++;
	sbi_printf("FAIL hart %lu: %s (error %ld, value %ld)\n", c->hartid, name,
		   err, val);
}

/* Boolean condition */
#define EXPECT(c, name, cond) report(c, name, !!(cond), 0, 0)

/* The error code of a call is err */
#define EXPECT_ERR(c, name, ret, err) \
	report(c, name, (ret).error == (err), (ret).error, (ret).value)

/* The call succeeded and returned value 0 */
#define EXPECT_OK0(c, name, ret) \
	report(c, name, !(ret).error && !(ret).value, (ret).error, (ret).value)

/* The error code is one of two that the specification allows */
#define EXPECT_ERR2(c, name, ret, e1, e2)                                  \
	report(c, name, (ret).error == (e1) || (ret).error == (e2),        \
	       (ret).error, (ret).value)

/* Entry i of the shared memory */
static unsigned long *entry(struct ctx *c, unsigned long i)
{
	return c->shmem + i * ENTRY_WORDS;
}

static void set_entry(struct ctx *c, unsigned long i, unsigned long idx,
		      unsigned long td1, unsigned long td2, unsigned long td3)
{
	unsigned long *e = entry(c, i);

	e[0] = idx;
	e[1] = td1;
	e[2] = td2;
	e[3] = td3;
}

/* Fill the shared memory with a pattern that is not a valid state */
static void poison(struct ctx *c)
{
	unsigned long i;

	for (i = 0; i < c->trig_max * ENTRY_WORDS; i++)
		c->shmem[i] = (unsigned long)0xa5a5a5a5a5a5a5a5ULL;
}

/* A configuration that fires on execution of addr in S-mode */
static unsigned long good_td1(unsigned long type)
{
	return TD1_WITH_TYPE(type) | MC_EXEC | MC_S;
}

/* Read all triggers, return the number of mapped ones or -1 */
static long read_all(struct ctx *c)
{
	unsigned long i;
	long mapped = 0;
	struct sbiret ret;

	poison(c);
	ret = sbi_debug_read_triggers(0, c->trig_max);
	if (ret.error)
		return -1;

	for (i = 0; i < c->trig_max; i++)
		if (entry(c, i)[0] & STATE_MAPPED)
			mapped++;

	return mapped;
}

/* Uninstall every trigger that is mapped, return the number or -1 */
static long cleanup(struct ctx *c)
{
	unsigned long i, mapped[32];
	unsigned long n = 0;

	if (read_all(c) < 0)
		return -1;

	for (i = 0; i < c->trig_max && n < 32; i++)
		if (entry(c, i)[0] & STATE_MAPPED)
			mapped[n++] = i;

	for (i = 0; i < n; i++)
		if (sbi_debug_uninstall_triggers(mapped[i], 1).error)
			return -1;

	return n;
}

/* Install one trigger, entry 0 of the shared memory has its logical index */
static struct sbiret install_one(struct ctx *c, unsigned long td1,
				 unsigned long td2, unsigned long td3)
{
	set_entry(c, 0, ~0UL, td1, td2, td3);

	return sbi_debug_install_triggers(1);
}

/* The number of triggers and the number that fit a configuration */
static void test_num_triggers(struct ctx *c)
{
	struct sbiret ret = sbi_debug_num_triggers(0);
	/* The mcontrol6 type is preferred, it is the newer one */
	static const unsigned long types[] = { TYPE_MCONTROL6, TYPE_MCONTROL };
	unsigned int i;
	unsigned long t;

	EXPECT(c, "num_triggers(0) succeeds", !ret.error);
	EXPECT(c, "num_triggers(0) returns trig_max > 0", ret.value > 0);
	c->trig_max = ret.value;

	for (i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
		t = types[i];
		ret = sbi_debug_num_triggers(good_td1(t));
		EXPECT(c, "num_triggers(type) succeeds", !ret.error);
		EXPECT(c, "num_triggers(type) <= trig_max",
		       (unsigned long)ret.value <= c->trig_max);
		if (!c->type && !ret.error && ret.value > 0) {
			c->type = t;
			c->matching = ret.value;
		} else if (!c->other_type && !ret.error && ret.value > 0) {
			c->other_type = t;
		}
	}

	ret = sbi_debug_num_triggers(good_td1(TYPE_RESERVED));
	EXPECT(c, "num_triggers(reserved type) succeeds", !ret.error);
	EXPECT(c, "num_triggers(reserved type) returns 0", ret.value == 0);

	if (c->boot)
		sbi_printf("INFO trig_max %lu, type %lu, matching %lu\n",
			   c->trig_max, c->type, c->matching);
}

/* The shared memory is disabled at the beginning, then it is set */
static void test_shmem_disabled(struct ctx *c)
{
	struct sbiret ret;

	/* The memory is not set: all calls that need it say so */
	ret = sbi_debug_read_triggers(0, 1);
	EXPECT_ERR(c, "read without shmem is NO_SHMEM", ret, SBI_ERR_NO_SHMEM);
	ret = sbi_debug_install_triggers(1);
	EXPECT_ERR(c, "install without shmem is NO_SHMEM", ret,
		   SBI_ERR_NO_SHMEM);
	ret = sbi_debug_update_triggers(1);
	EXPECT_ERR(c, "update without shmem is NO_SHMEM", ret,
		   SBI_ERR_NO_SHMEM);
}

/*
 * A set_shmem call that has to fail with e1 or e2. If the firmware accepts
 * it anyway, the shared memory is disabled again at once: the next call that
 * uses it would make M-mode write to the bad address.
 */
static void bad_shmem(struct ctx *c, const char *name, unsigned long lo,
		      unsigned long hi, unsigned long flags, long e1, long e2)
{
	struct sbiret ret = sbi_debug_set_shmem(lo, hi, flags);

	report(c, name, ret.error == e1 || ret.error == e2, ret.error,
	       ret.value);
	if (!ret.error)
		sbi_debug_set_shmem(~0UL, ~0UL, 0);
}

static void test_shmem_setup(struct ctx *c)
{
	unsigned long addr = (unsigned long)c->shmem;
	unsigned long align = sizeof(unsigned long);
	struct sbiret ret;

	/* flags is reserved and must be zero */
	bad_shmem(c, "set_shmem flags=1 is INVALID_PARAM", addr, 0, 1,
		  SBI_ERR_INVALID_PARAM, SBI_ERR_INVALID_PARAM);
	bad_shmem(c, "set_shmem flags=~0 is INVALID_PARAM", addr, 0, ~0UL,
		  SBI_ERR_INVALID_PARAM, SBI_ERR_INVALID_PARAM);
	bad_shmem(c, "disable with flags=1 is INVALID_PARAM", ~0UL, ~0UL, 1,
		  SBI_ERR_INVALID_PARAM, SBI_ERR_INVALID_PARAM);

	/* The address has to be aligned to XLEN / 8 bytes */
	bad_shmem(c, "set_shmem unaligned by 1 is INVALID_PARAM", addr + 1, 0,
		  0, SBI_ERR_INVALID_PARAM, SBI_ERR_INVALID_PARAM);
	bad_shmem(c, "set_shmem half aligned is INVALID_PARAM",
		  addr + align / 2, 0, 0, SBI_ERR_INVALID_PARAM,
		  SBI_ERR_INVALID_PARAM);

	/* Addresses that S-mode cannot use, the note allows FAILED too */
	bad_shmem(c, "set_shmem hi != 0 is not accepted", addr, 1, 0,
		  SBI_ERR_INVALID_ADDRESS, SBI_ERR_FAILED);
	bad_shmem(c, "set_shmem at the top of memory is not accepted",
		  ~0UL & ~(align - 1), 0, 0, SBI_ERR_INVALID_ADDRESS,
		  SBI_ERR_FAILED);
	/*
	 * Ranges that touch M-mode memory. All addresses are aligned, so the
	 * alignment check does not hide the range check. The shared memory
	 * is size bytes long, word is the smallest step that keeps alignment.
	 */
	if (dbtr_mmode_size) {
		unsigned long size = c->trig_max * ENTRY_BYTES;
		unsigned long end = dbtr_mmode_base + dbtr_mmode_size;

		bad_shmem(c, "set_shmem starting in M-mode memory fails",
			  dbtr_mmode_base, 0, 0, SBI_ERR_INVALID_ADDRESS,
			  SBI_ERR_FAILED);
		bad_shmem(c, "set_shmem starting in its last word fails",
			  end - align, 0, 0, SBI_ERR_INVALID_ADDRESS,
			  SBI_ERR_FAILED);
		/* Starts outside, the last word of the range is inside */
		bad_shmem(c, "set_shmem ending in M-mode memory fails",
			  dbtr_mmode_base - size + align, 0, 0,
			  SBI_ERR_INVALID_ADDRESS, SBI_ERR_FAILED);
	}

	/* None of the rejected calls enabled the shared memory */
	ret = sbi_debug_read_triggers(0, 1);
	EXPECT_ERR(c, "rejected addresses leave shmem disabled", ret,
		   SBI_ERR_NO_SHMEM);

	/* Disabling what is disabled is fine */
	ret = sbi_debug_set_shmem(~0UL, ~0UL, 0);
	EXPECT_OK0(c, "disable while disabled succeeds", ret);

	ret = sbi_debug_set_shmem(addr, 0, 0);
	EXPECT_ERR(c, "set_shmem succeeds", ret, SBI_SUCCESS);
}

/* Reading triggers: ranges and the state of the unused triggers */
static void test_read(struct ctx *c)
{
	unsigned long i, max = c->trig_max;
	struct sbiret ret;
	int clean = 1;

	poison(c);
	ret = sbi_debug_read_triggers(0, max);
	EXPECT_ERR(c, "read of all triggers succeeds", ret, SBI_SUCCESS);
	for (i = 0; i < max; i++)
		if (entry(c, i)[0] & (STATE_MAPPED | STATE_RESERVED))
			clean = 0;
	EXPECT(c, "unused triggers are not mapped, reserved bits 0", clean);

	ret = sbi_debug_read_triggers(max - 1, 1);
	EXPECT_ERR(c, "read of the last trigger succeeds", ret, SBI_SUCCESS);
	ret = sbi_debug_read_triggers(max, 1);
	EXPECT_ERR(c, "read at trig_max is BAD_RANGE", ret, SBI_ERR_BAD_RANGE);
	ret = sbi_debug_read_triggers(0, max + 1);
	EXPECT_ERR(c, "read of trig_max + 1 is BAD_RANGE", ret,
		   SBI_ERR_BAD_RANGE);
	ret = sbi_debug_read_triggers(max - 1, 2);
	EXPECT_ERR(c, "read over the end is BAD_RANGE", ret,
		   SBI_ERR_BAD_RANGE);
	ret = sbi_debug_read_triggers(1, ~0UL);
	EXPECT_ERR(c, "read with huge count is BAD_RANGE", ret,
		   SBI_ERR_BAD_RANGE);
	ret = sbi_debug_read_triggers(~0UL, 1);
	EXPECT_ERR(c, "read with huge base is BAD_RANGE", ret,
		   SBI_ERR_BAD_RANGE);
}

/* Nothing is installed: the failed call must not leave a trigger behind */
static void expect_none(struct ctx *c, const char *name)
{
	report(c, name, read_all(c) == 0, 0, 0);
}

/* A failed install of entry 0: error, index 0 in value, nothing installed */
static void expect_install_fails(struct ctx *c, const char *name,
				 unsigned long td1, unsigned long td2,
				 unsigned long td3, long e1, long e2)
{
	struct sbiret ret = install_one(c, td1, td2, td3);

	report(c, name, (ret.error == e1 || ret.error == e2) && ret.value == 0,
	       ret.error, ret.value);
	/* Whatever the result, do not leave a trigger for the next case */
	if (cleanup(c) > 0)
		report(c, "a failed install leaves no trigger", 0, 0, 0);
}

static void test_install(struct ctx *c)
{
	unsigned long addr = (unsigned long)dbtr_target;
	unsigned long td1 = good_td1(c->type);
	struct sbiret ret;
	unsigned long *e;

	ret = sbi_debug_install_triggers(0);
	EXPECT_OK0(c, "install of 0 triggers succeeds", ret);
	ret = sbi_debug_install_triggers(c->trig_max + 1);
	EXPECT_ERR(c, "install of trig_max + 1 is BAD_RANGE", ret,
		   SBI_ERR_BAD_RANGE);
	ret = sbi_debug_install_triggers(~0UL);
	EXPECT_ERR(c, "install of ~0 is BAD_RANGE", ret, SBI_ERR_BAD_RANGE);
	expect_none(c, "BAD_RANGE installs nothing");

	/* The M bit and the DMODE bit must be zero */
	expect_install_fails(c, "install with M bit is INVALID_PARAM",
			     td1 | MC_M, addr, 0, SBI_ERR_INVALID_PARAM,
			     SBI_ERR_INVALID_PARAM);
	expect_install_fails(c, "install with DMODE is INVALID_PARAM",
			     td1 | TD1_DMODE, addr, 0, SBI_ERR_INVALID_PARAM,
			     SBI_ERR_INVALID_PARAM);
	/* Types that no trigger has: none, reserved, and a disabled one */
	expect_install_fails(c, "install of type 0 fails", 0, addr, 0,
			     SBI_ERR_INVALID_PARAM, SBI_ERR_FAILED);
	expect_install_fails(c, "install of reserved type fails",
			     good_td1(TYPE_RESERVED), addr, 0,
			     SBI_ERR_INVALID_PARAM, SBI_ERR_FAILED);
	/*
	 * A configuration that ends with a chain is incomplete. The specification
	 * forbids it but names no error, so only report what happens.
	 */
	ret = install_one(c, td1 | MC_CHAIN, addr, 0);
	if (c->boot)
		sbi_printf("INFO install with chain in the last entry: error %ld\n",
			   ret.error);
	cleanup(c);

	/* A good one: the index is written back, the state is as saved */
	ret = install_one(c, td1, addr, 0);
	EXPECT_OK0(c, "install of a good trigger succeeds", ret);
	e = entry(c, 0);
	EXPECT(c, "index written back is below trig_max", e[0] < c->trig_max);

	poison(c);
	ret = sbi_debug_read_triggers(0, c->trig_max);
	EXPECT_ERR(c, "read after install succeeds", ret, SBI_SUCCESS);
	EXPECT(c, "read: one trigger is mapped", read_all(c) == 1);

	cleanup(c);
	expect_none(c, "cleanup removes the trigger");
}

/* Update entry 0 of the shared memory, which has the logical index idx */
static struct sbiret update_one(struct ctx *c, unsigned long idx,
				unsigned long td1, unsigned long td2,
				unsigned long td3)
{
	set_entry(c, 0, idx, td1, td2, td3);

	return sbi_debug_update_triggers(1);
}

/* tdata1 of trigger idx as reported by the read call */
static unsigned long read_td1(struct ctx *c, unsigned long idx)
{
	poison(c);
	if (sbi_debug_read_triggers(idx, 1).error)
		return ~0UL;

	return entry(c, 0)[1];
}

static void test_update(struct ctx *c)
{
	unsigned long addr = (unsigned long)dbtr_target;
	unsigned long addr2 = (unsigned long)dbtr_target2;
	unsigned long td1 = good_td1(c->type);
	unsigned long idx;
	struct sbiret ret;

	ret = sbi_debug_update_triggers(0);
	EXPECT_OK0(c, "update of 0 triggers succeeds", ret);
	ret = sbi_debug_update_triggers(c->trig_max + 1);
	EXPECT_ERR(c, "update of trig_max + 1 is BAD_RANGE", ret,
		   SBI_ERR_BAD_RANGE);

	/* A trigger that is not installed */
	ret = update_one(c, 0, td1, addr, 0);
	EXPECT_ERR(c, "update of unmapped trigger is FAILED", ret,
		   SBI_ERR_FAILED);
	EXPECT(c, "failing index of update is 0", ret.value == 0);
	ret = update_one(c, c->trig_max, td1, addr, 0);
	EXPECT_ERR(c, "update of idx = trig_max is INVALID_PARAM", ret,
		   SBI_ERR_INVALID_PARAM);
	ret = update_one(c, ~0UL, td1, addr, 0);
	EXPECT_ERR(c, "update of idx = ~0 is INVALID_PARAM", ret,
		   SBI_ERR_INVALID_PARAM);

	ret = install_one(c, td1, addr, 0);
	EXPECT_OK0(c, "install for the update cases", ret);
	idx = entry(c, 0)[0];
	EXPECT(c, "installed trigger index is valid", idx < c->trig_max);
	if (ret.error || idx >= c->trig_max) {
		cleanup(c);
		return;
	}

	/* Update with another address: success */
	ret = update_one(c, idx, td1, addr2, 0);
	EXPECT_OK0(c, "update with a good configuration succeeds", ret);
	EXPECT(c, "update keeps tdata1 as given", read_td1(c, idx) == td1);

	/* The configuration is checked like in install, M and DMODE are 0 */
	ret = update_one(c, idx, td1 | MC_M, addr2, 0);
	EXPECT_ERR(c, "update with M bit is INVALID_PARAM", ret,
		   SBI_ERR_INVALID_PARAM);
	EXPECT(c, "update with M bit reports index 0", ret.value == 0);
	EXPECT(c, "rejected update leaves tdata1 alone",
	       read_td1(c, idx) == td1);
	ret = update_one(c, idx, td1 | TD1_DMODE, addr2, 0);
	EXPECT_ERR(c, "update with DMODE is INVALID_PARAM", ret,
		   SBI_ERR_INVALID_PARAM);
	EXPECT(c, "rejected DMODE update leaves tdata1 alone",
	       read_td1(c, idx) == td1);

	/*
	 * The type and the chain bit must match the installed trigger. The
	 * specification says the update fails but names no error code for
	 * it, so any error is accepted.
	 */
	ret = update_one(c, idx, good_td1(TYPE_RESERVED), addr2, 0);
	EXPECT(c, "update to a reserved type is not accepted", ret.error != 0);
	ret = update_one(c, idx, 0, addr2, 0);
	EXPECT(c, "update to type 0 is not accepted", ret.error != 0);
	if (c->other_type) {
		ret = update_one(c, idx, good_td1(c->other_type), addr2, 0);
		EXPECT(c, "update to another type is not accepted", ret.error != 0);
	}
	EXPECT(c, "rejected type changes leave tdata1 alone",
	       read_td1(c, idx) == td1);

	/* The chain bit must match the installed trigger */
	ret = update_one(c, idx, td1 | MC_CHAIN, addr2, 0);
	EXPECT(c, "update with another chain bit is not accepted", ret.error != 0);
	EXPECT(c, "rejected chain update leaves tdata1 alone",
	       read_td1(c, idx) == td1);

	cleanup(c);
	ret = update_one(c, idx, td1, addr, 0);
	EXPECT_ERR(c, "update after uninstall is FAILED", ret, SBI_ERR_FAILED);
}

/* Uninstall, enable and disable: index checks, same rules for all three */
static void test_index_sets(struct ctx *c)
{
	struct sbiret (*const fn[3])(unsigned long, unsigned long) = {
		sbi_debug_uninstall_triggers, sbi_debug_enable_triggers,
		sbi_debug_disable_triggers,
	};
	static const char *const name[3] = { "uninstall", "enable", "disable" };
	char msg[64];
	struct sbiret ret;
	unsigned int i;

	for (i = 0; i < 3; i++) {
		/* Nothing is installed, so no index is mapped */
		ret = fn[i](0, 1);
		sbi_snprintf(msg, sizeof(msg), "%s of unmapped trigger", name[i]);
		EXPECT_ERR(c, msg, ret, SBI_ERR_INVALID_PARAM);
		ret = fn[i](c->trig_max, 1);
		sbi_snprintf(msg, sizeof(msg), "%s of idx = trig_max", name[i]);
		EXPECT_ERR(c, msg, ret, SBI_ERR_INVALID_PARAM);
		ret = fn[i](~0UL, 1);
		sbi_snprintf(msg, sizeof(msg), "%s of base = ~0", name[i]);
		EXPECT_ERR(c, msg, ret, SBI_ERR_INVALID_PARAM);
		/* A bit of the mask beyond trig_max is an index >= trig_max */
		ret = fn[i](0, 1UL << (sizeof(unsigned long) * 8 - 1));
		sbi_snprintf(msg, sizeof(msg), "%s of the top bit", name[i]);
		EXPECT_ERR(c, msg, ret, SBI_ERR_INVALID_PARAM);
		/* An empty set has no invalid member */
		ret = fn[i](0, 0);
		sbi_snprintf(msg, sizeof(msg), "%s of an empty set", name[i]);
		EXPECT_OK0(c, msg, ret);
	}
}

/* The triggers fire, are disabled, enabled and removed. Boot hart only. */
static void test_fire(struct ctx *c)
{
	unsigned long td1 = good_td1(c->type);
	unsigned long state, idx;
	struct sbiret ret;

	ret = install_one(c, td1, (unsigned long)dbtr_target, 0);
	EXPECT_OK0(c, "install for the fire cases", ret);
	idx = entry(c, 0)[0];
	if (ret.error || idx >= c->trig_max) {
		cleanup(c);
		return;
	}

	EXPECT(c, "the trigger fires on its address", fires(dbtr_target) == 1);
	EXPECT(c, "the trigger does not fire elsewhere",
	       fires(dbtr_target2) == 0);

	poison(c);
	sbi_debug_read_triggers(idx, 1);
	state = entry(c, 0)[0];
	EXPECT(c, "state: mapped", state & STATE_MAPPED);
	EXPECT(c, "state: s saved, u not",
	       (state & STATE_S) && !(state & STATE_U));
	EXPECT(c, "state: vs and vu clear", !(state & (STATE_VS | STATE_VU)));
	EXPECT(c, "state: reserved bits 0", !(state & STATE_RESERVED));
	EXPECT(c, "state: tdata1 type as installed",
	       (entry(c, 0)[1] >> TD1_TYPE_SHIFT) == c->type);
	EXPECT(c, "state: tdata2 is the address",
	       entry(c, 0)[2] == (unsigned long)dbtr_target);
	EXPECT(c, "state: M and DMODE are 0",
	       !(entry(c, 0)[1] & (MC_M | TD1_DMODE)));

	ret = sbi_debug_disable_triggers(idx, 1);
	EXPECT_OK0(c, "disable succeeds", ret);
	EXPECT(c, "a disabled trigger does not fire", fires(dbtr_target) == 0);
	ret = sbi_debug_disable_triggers(idx, 1);
	EXPECT_OK0(c, "disable of a disabled trigger succeeds", ret);

	ret = sbi_debug_enable_triggers(idx, 1);
	EXPECT_OK0(c, "enable succeeds", ret);
	EXPECT(c, "an enabled trigger fires again", fires(dbtr_target) == 1);
	ret = sbi_debug_enable_triggers(idx, 1);
	EXPECT_OK0(c, "enable of an enabled trigger succeeds", ret);

	/* The update moves the trigger to the other address */
	set_entry(c, 0, idx, td1, (unsigned long)dbtr_target2, 0);
	ret = sbi_debug_update_triggers(1);
	EXPECT_OK0(c, "update to the other address succeeds", ret);
	EXPECT(c, "updated trigger fires on the new address",
	       fires(dbtr_target2) == 1);
	EXPECT(c, "updated trigger does not fire on the old one",
	       fires(dbtr_target) == 0);

	ret = sbi_debug_uninstall_triggers(idx, 1);
	EXPECT_OK0(c, "uninstall succeeds", ret);
	EXPECT(c, "an uninstalled trigger does not fire",
	       fires(dbtr_target2) == 0);
	EXPECT(c, "nothing is mapped after uninstall", read_all(c) == 0);
	ret = sbi_debug_uninstall_triggers(idx, 1);
	EXPECT_ERR(c, "second uninstall is INVALID_PARAM", ret,
		   SBI_ERR_INVALID_PARAM);
	ret = sbi_debug_enable_triggers(idx, 1);
	EXPECT_ERR(c, "enable after uninstall is INVALID_PARAM", ret,
		   SBI_ERR_INVALID_PARAM);
}

/* Batches: the failing index is reported, all triggers can be used */
static void test_batch(struct ctx *c)
{
	unsigned long td1 = good_td1(c->type);
	unsigned long i, n = c->matching;
	struct sbiret ret;

	/* A bad entry in the middle: its array index is the value */
	if (n >= 3) {
		set_entry(c, 0, ~0UL, td1, (unsigned long)dbtr_target, 0);
		set_entry(c, 1, ~0UL, td1 | MC_M, (unsigned long)dbtr_target, 0);
		set_entry(c, 2, ~0UL, td1, (unsigned long)dbtr_target2, 0);
		ret = sbi_debug_install_triggers(3);
		EXPECT_ERR(c, "batch with M bit in entry 1 is INVALID_PARAM", ret,
			   SBI_ERR_INVALID_PARAM);
		EXPECT(c, "batch failure reports array index 1", ret.value == 1);
		/*
		 * The specification does not say whether entry 0 stays
		 * installed. Report it, then clean up.
		 */
		if (c->boot)
			sbi_printf("INFO entries before a bad one stay installed: %ld\n",
				   read_all(c));
		cleanup(c);
		expect_none(c, "nothing left after the failed batch");
	}

	/* Install all triggers that can take the configuration */
	for (i = 0; i < n; i++)
		set_entry(c, i, ~0UL, td1, (unsigned long)dbtr_target, 0);
	ret = sbi_debug_install_triggers(n);
	EXPECT_OK0(c, "install of all matching triggers succeeds", ret);
	EXPECT(c, "all of them are mapped", read_all(c) == (long)n);

	/* The indexes written back are different */
	poison(c);
	for (i = 0; i < n; i++)
		set_entry(c, i, ~0UL, td1, (unsigned long)dbtr_target, 0);
	cleanup(c);
	EXPECT(c, "cleanup uninstalls all", read_all(c) == 0);

	/* One more than there are: the last one cannot get a trigger */
	if (n < c->trig_max) {
		for (i = 0; i <= n; i++)
			set_entry(c, i, ~0UL, td1, (unsigned long)dbtr_target, 0);
		ret = sbi_debug_install_triggers(n + 1);
		EXPECT_ERR2(c, "install of one trigger too many fails", ret,
			    SBI_ERR_FAILED, SBI_ERR_NOT_SUPPORTED);
		EXPECT(c, "failing index is the one too many",
		       (unsigned long)ret.value == n);
		cleanup(c);
	}

	/* After a cleanup the triggers are free again */
	ret = install_one(c, td1, (unsigned long)dbtr_target, 0);
	EXPECT_OK0(c, "triggers can be installed again", ret);
	cleanup(c);
}

/* Disabling the shared memory takes the memory away again */
static void test_shmem_off(struct ctx *c)
{
	struct sbiret ret;

	ret = sbi_debug_set_shmem(~0UL, ~0UL, 0);
	EXPECT_OK0(c, "set_shmem(~0, ~0) succeeds", ret);
	ret = sbi_debug_read_triggers(0, 1);
	EXPECT_ERR(c, "read after disable is NO_SHMEM", ret, SBI_ERR_NO_SHMEM);
	ret = sbi_debug_install_triggers(1);
	EXPECT_ERR(c, "install after disable is NO_SHMEM", ret,
		   SBI_ERR_NO_SHMEM);
	ret = sbi_debug_update_triggers(1);
	EXPECT_ERR(c, "update after disable is NO_SHMEM", ret,
		   SBI_ERR_NO_SHMEM);

	/* One of the two halves all-ones is not a disable */
	ret = sbi_debug_set_shmem(~0UL, 0, 0);
	EXPECT(c, "set_shmem(~0, 0) is not a disable", ret.error != 0);
	ret = sbi_debug_read_triggers(0, 1);
	EXPECT_ERR(c, "... and leaves shmem disabled", ret, SBI_ERR_NO_SHMEM);
}

unsigned int test_dbtr_hart(unsigned long hartid)
{
	struct ctx c = { .hartid = hartid, .boot = hartid == dbtr_boot_hartid };
	const struct hart *hart = hart_find(hartid);
	unsigned int errors;

	/* A hart without Sdtrig in the device tree has no triggers to test */
	if (!hart || !hart->sdtrig) {
		sbi_printf("Hart %lu: no Sdtrig in the device tree, skipped\n",
			   hartid);
		return 0;
	}

	test_num_triggers(&c);
	if (!c.trig_max || !c.type) {
		sbi_printf("FAIL hart %lu: no usable trigger\n", hartid);
		return c.failed + 1;
	}

	/* The shared memory is trig_max * XLEN / 2 bytes */
	c.shmem = malloc(c.trig_max * ENTRY_BYTES);
	if (!c.shmem) {
		sbi_printf("FAIL hart %lu: out of memory\n", hartid);
		return c.failed + 1;
	}

	test_shmem_disabled(&c);
	test_shmem_setup(&c);
	test_read(&c);
	test_install(&c);
	test_update(&c);
	test_index_sets(&c);
	if (c.boot)
		test_fire(&c);
	test_batch(&c);
	test_shmem_off(&c);

	/* Leave nothing behind */
	sbi_debug_set_shmem(~0UL, ~0UL, 0);
	errors = c.failed;
	free(c.shmem);

	sbi_printf("Hart %lu: %u checks failed\n", hartid, errors);

	return errors;
}
