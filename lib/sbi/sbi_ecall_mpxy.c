/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_mpxy.h>

static int sbi_ecall_mpxy_handler(unsigned long extid, unsigned long funcid,
				  struct sbi_trap_regs *regs,
				  struct sbi_ecall_return *out)
{
	int ret = 0;

	switch (funcid) {
	case SBI_EXT_MPXY_GET_SHMEM_SIZE:
		out->value = sbi_mpxy_get_shmem_size();
		break;
	case SBI_EXT_MPXY_SET_SHMEM:
		ret = sbi_mpxy_set_shmem(regs->a0, regs->a1, regs->a2);
		break;
	case SBI_EXT_MPXY_GET_CHANNEL_IDS:
		ret = sbi_mpxy_get_channel_ids(regs->a0);
		break;
	case SBI_EXT_MPXY_READ_ATTRS:
		ret = sbi_mpxy_read_attrs(regs->a0, regs->a1, regs->a2);
		break;
	case SBI_EXT_MPXY_WRITE_ATTRS:
		ret = sbi_mpxy_write_attrs(regs->a0, regs->a1, regs->a2);
		break;
	case SBI_EXT_MPXY_SEND_MSG_WITH_RESP:
		ret = sbi_mpxy_send_message(regs->a0, regs->a1,
					    regs->a2, &out->value);
		break;
	case SBI_EXT_MPXY_SEND_MSG_WITHOUT_RESP:
		ret = sbi_mpxy_send_message(regs->a0, regs->a1, regs->a2,
					    NULL);
		break;
	case SBI_EXT_MPXY_GET_NOTIFICATION_EVENTS:
		ret = sbi_mpxy_get_notification_events(regs->a0, &out->value);
		break;
	default:
		ret = SBI_ENOTSUPP;
	}

	return ret;
}

struct sbi_ecall_extension ecall_mpxy;

static int sbi_ecall_mpxy_register_extensions(void)
{
	if (!sbi_mpxy_channel_available())
		return 0;

	return sbi_ecall_register_extension(&ecall_mpxy);
}

struct sbi_ecall_extension ecall_mpxy = {
	.name			= "mpxy",
	.extid_start		= SBI_EXT_MPXY,
	.extid_end		= SBI_EXT_MPXY,
	.register_extensions	= sbi_ecall_mpxy_register_extensions,
	.handle			= sbi_ecall_mpxy_handler,
};
