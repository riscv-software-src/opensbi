/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Simple simply-linked list library.
 *
 * Copyright (c) 2025 Rivos Inc.
 *
 * Authors:
 *   Clément Léger <cleger@rivosinc.com>
 */

#ifndef __SBI_SLIST_H__
#define __SBI_SLIST_H__

#include <sbi/sbi_types.h>

#define SBI_SLIST_HEAD_INIT(_ptr)	(_ptr)
#define SBI_SLIST_HEAD(_lname, _stype) struct _stype *_lname
#define SBI_SLIST_NODE(_stype) SBI_SLIST_HEAD(next, _stype)
#define SBI_SLIST_NODE_INIT(_ptr) .next = _ptr

#define SBI_INIT_SLIST_HEAD(_head) (_head) = NULL

#define SBI_SLIST_ADD(_ptr, _head) \
do { \
	(_ptr)->next = _head; \
	(_head) = _ptr; \
} while (0)

#define SBI_SLIST_FOR_EACH_ENTRY(_ptr, _head) \
	for (_ptr = _head; _ptr; _ptr = _ptr->next)

#endif
