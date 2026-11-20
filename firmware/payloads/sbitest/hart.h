/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Harts of the S-mode test payload.
 *
 * The harts are found in the device tree. Each one is a node of a linked list
 * that is allocated with malloc(). The list is built once by the boot hart
 * and never changed afterwards, so all harts can read it without a lock.
 */

#ifndef __SBITEST_HART_H__
#define __SBITEST_HART_H__

/* Size of the stack of a secondary hart, it is allocated with malloc() */
#define HART_STACK_SIZE (128 * 1024)

/*
 * Position of the members of struct hart that start.S reads, in units of one
 * register. A parked hart has no stack and no C environment, it walks the
 * list in assembly. hart.c checks the offsets at compile time.
 */
#define HART_NEXT 0
#define HART_ID 1
#define HART_STACK 2

#ifndef __ASSEMBLER__

/* Bitness (XLEN) of the code that runs here */
#define OWN_BITNESS __riscv_xlen

struct hart {
	/* Next hart in the list, NULL at the end */
	struct hart *next;
	/* Value of the reg property of the node in the device tree */
	unsigned long hartid;
	/*
	 * Top of the stack of the hart, 0 while it has none. The boot hart
	 * writes it with assign_stack(), the parked hart polls it, see
	 * start.S.
	 */
	unsigned long stack_top;
	/* 32 or 64, 0 if the device tree does not tell */
	unsigned int bitness;
	/* The boot hart started this hart successfully */
	unsigned int started;
};

/*
 * First hart of the list of harts, NULL as long as there is no list. It is
 * defined in start.S where the parked harts read it.
 */
extern struct hart *hart_list;

/* Counters of the secondary harts that were woken up, see hart.c */
extern volatile unsigned int secondary_awake;
extern volatile unsigned int secondary_on_stack;
extern volatile unsigned int secondary_done;
extern volatile unsigned int secondary_errors;

/*
 * Build a list of the harts in the device tree. They are the children of
 * /cpus whose compatible property has the string "riscv". The reg property is
 * the hart ID and riscv,isa-base tells the bitness. Nodes without a usable
 * reg property are skipped. The result is NULL if there are no harts, or if
 * /cpus is missing or has an address size that is not supported. count is set
 * to the number of harts.
 */
struct hart *harts_parse(const void *fdt, unsigned int *count);

/* Release a list that harts_parse() returned */
void harts_free(struct hart *list);

/* Parse the device tree and make the result the list of harts of the test */
void harts_init(const void *fdt);

/* First hart of the list of the test, NULL if there is none */
struct hart *hart_first(void);

/* Number of harts in the list of the test */
unsigned int hart_count(void);

/* Find a hart in the list of the test by its ID, NULL if there is none */
struct hart *hart_find(unsigned long hartid);

/* Can the hart run this payload? Only if it has the same bitness. */
int hart_can_run(const struct hart *hart);

/*
 * Get a stack of HART_STACK_SIZE bytes for the hart from malloc() and publish
 * it in the list. Return the top of the stack, or 0 if the hart is not in the
 * list or there is no memory. It can be called from any hart at the same
 * time. If the hart has a stack already, that one is returned.
 */
unsigned long assign_stack(unsigned long hartid);

/* C entry of a secondary hart that was woken up, runs on the new stack */
__attribute__((noreturn)) void secondary_main(struct hart *hart);

#endif /* __ASSEMBLER__ */

#endif /* __SBITEST_HART_H__ */
