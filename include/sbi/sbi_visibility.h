/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 SiFive
 */

#ifndef __SBI_VISIBILITY_H__
#define __SBI_VISIBILITY_H__

#ifndef __DTS__
/*
 * Declare all global objects with hidden visibility so access is PC-relative
 * instead of going through the GOT.
 */
#pragma GCC visibility push(hidden)
#endif

#endif
