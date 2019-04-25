/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 HardenedLinux.
 *
 * Authors:
 *   Xiang Wang<wxjstz@126.com>
 *
 */
#ifndef __SBI_FDT_H__
#define __SBI_FDT_H__

/* update next_addr by fdt /chosen:opensbi,next_addr
 * /{
 *     ...
 *     chosen {
 *        opensbi,next_addr = xxxx
 *     }
 *     ...
 *  } */
int sbi_fdt_update_next_addr(struct sbi_scratch *scratch, bool coolboot);

#endif
