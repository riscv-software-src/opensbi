/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 */

#ifndef __SBI_CPPC_H__
#define __SBI_CPPC_H__

#include <sbi/sbi_types.h>

/** CPPC device */
struct sbi_cppc_device {
	/** Name of the CPPC device */
	char name[32];

	/** probe - returns register width if implemented, 0 otherwise */
	int (*cppc_probe)(unsigned long reg);

	/** read the cppc register*/
	int (*cppc_read)(unsigned long reg, uint64_t *val);

	/** write to the cppc register*/
	int (*cppc_write)(unsigned long reg, uint64_t val);
};

int sbi_cppc_probe(unsigned long reg);
int sbi_cppc_read(unsigned long reg, uint64_t *val);
int sbi_cppc_write(unsigned long reg, uint64_t val);

const struct sbi_cppc_device *sbi_cppc_get_device(void);
void sbi_cppc_set_device(const struct sbi_cppc_device *dev);

#endif
