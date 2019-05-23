/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_ERROR_H__
#define __SBI_ERROR_H__

/* clang-format off */

#define SBI_OK		0
#define SBI_EUNKNOWN	-1
#define SBI_EFAIL	-2
#define SBI_EINVAL	-3
#define SBI_ENOENT	-4
#define SBI_ENOTSUPP	-5
#define SBI_ENODEV	-6
#define SBI_ENOSYS	-7
#define SBI_ETIMEDOUT	-8
#define SBI_EIO		-9
#define SBI_EILL	-10
#define SBI_ENOSPC	-11
#define SBI_ENOMEM	-12
#define SBI_ETRAP	-13

/* clang-format on */

#endif
