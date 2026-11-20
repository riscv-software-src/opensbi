/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Declarations of the DBTR test: the SBI debug trigger extension. The
 * expected results come from the SBI specification, chapter "Debug Triggers
 * Extension", and from the Sdtrig specification for the encoding of tdata1.
 */

#ifndef __SBITEST_DBTR_H__
#define __SBITEST_DBTR_H__

#include "trap.h"

/* Start and size of the first M-mode reserved region in the FDT, 0 if none */
extern unsigned long dbtr_mmode_base;
extern unsigned long dbtr_mmode_size;

/* Only this hart executes the triggers and takes the breakpoints */
extern unsigned long dbtr_boot_hartid;

/*
 * Exception hook of the DBTR test: counts the breakpoints that a trigger on
 * dbtr_target() or dbtr_target2() raises and skips the instruction.
 */
int dbtr_trap_hook(struct trap_regs *regs);

/*
 * The tests of one hart. Every hart runs them with its own triggers and its
 * own shared memory, the boot hart also checks that the triggers fire. The
 * result is the number of failed checks. It is also the task of the secondary
 * harts, see secondary_task in hart.h.
 */
unsigned int test_dbtr_hart(unsigned long hartid);

#endif /* __SBITEST_DBTR_H__ */
