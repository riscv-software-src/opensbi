/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_ECALL_H__
#define __SBI_ECALL_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_list.h>

#define SBI_ECALL_VERSION_MAJOR		2
#define SBI_ECALL_VERSION_MINOR		0
#define SBI_OPENSBI_IMPID		1

struct sbi_trap_regs;
struct sbi_trap_context;

struct sbi_ecall_return {
	/* Return flag to skip register update */
	bool skip_regs_update;
	/* Return value */
	unsigned long value;
};

struct sbi_ecall_extension {
	/* head is used by the extension list */
	struct sbi_dlist head;
	/*
	 * extid_start and extid_end specify the range for this extension. As
	 * the initial range may be wider than the valid runtime range, the
	 * register_extensions callback is responsible for narrowing the range
	 * before other callbacks may be invoked.
	 */
	unsigned long extid_start;
	unsigned long extid_end;
	/*
	 * register_extensions
	 *
	 * Calls sbi_ecall_register_extension() one or more times to register
	 * extension ID range(s) which should be handled by this extension.
	 * More than one sbi_ecall_extension struct and
	 * sbi_ecall_register_extension() call is necessary when the supported
	 * extension ID ranges have gaps. Additionally, extension availability
	 * must be checked before registering, which means, when this callback
	 * returns, only valid extension IDs from the initial range, which are
	 * also available, have been registered.
	 */
	int (* register_extensions)(void);
	/*
	 * probe
	 *
	 * Implements the Base extension's probe function for the extension. As
	 * the register_extensions callback ensures that no other extension
	 * callbacks will be invoked when the extension is not available, then
	 * probe can never fail. However, an extension may choose to set
	 * out_val to a nonzero value other than one. In those cases, it should
	 * implement this callback.
	 */
	int (* probe)(unsigned long extid, unsigned long *out_val);
	/*
	 * handle
	 *
	 * This is the extension handler. register_extensions ensures it is
	 * never invoked with an invalid or unavailable extension ID.
	 */
	int (* handle)(unsigned long extid, unsigned long funcid,
		       struct sbi_trap_regs *regs,
		       struct sbi_ecall_return *out);
};

u16 sbi_ecall_version_major(void);

u16 sbi_ecall_version_minor(void);

unsigned long sbi_ecall_get_impid(void);

void sbi_ecall_set_impid(unsigned long impid);

struct sbi_ecall_extension *sbi_ecall_find_extension(unsigned long extid);

int sbi_ecall_register_extension(struct sbi_ecall_extension *ext);

void sbi_ecall_unregister_extension(struct sbi_ecall_extension *ext);

int sbi_ecall_handler(struct sbi_trap_context *tcntx);

int sbi_ecall_init(void);

#endif
