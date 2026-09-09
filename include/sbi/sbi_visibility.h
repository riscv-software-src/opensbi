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

/* Thread Safety Analysis (TSA) annotations for use with clang -Wthread-safety */
#if defined(__clang__) && __clang_major__ >= 19
#define TSA_ATTR(x) __attribute__((x))
#else
#define TSA_ATTR(x)
#endif

#define CAPABILITY(x) TSA_ATTR(capability(x))
#define GUARDED_BY(x) TSA_ATTR(guarded_by(x))
#define PT_GUARDED_BY(x) TSA_ATTR(pt_guarded_by(x))
#define ACQUIRE(...) TSA_ATTR(acquire_capability(__VA_ARGS__))
#define RELEASE(...) TSA_ATTR(release_capability(__VA_ARGS__))
#define MUST_HOLD(...) TSA_ATTR(requires_capability(__VA_ARGS__))
#define MUST_NOT_HOLD(...) TSA_ATTR(locks_excluded(__VA_ARGS__))
#define TRY_ACQUIRE(b, ...) TSA_ATTR(try_acquire_capability(b, __VA_ARGS__))
#define NO_THREAD_SAFETY_ANALYSIS TSA_ATTR(no_thread_safety_analysis)

#endif
