/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 YADRO
 *
 * Authors:
 *   Nikita Shubin <n.shubin@yadro.com>
 */

#ifndef __FDT_I2C_H__
#define __FDT_I2C_H__

#include <sbi_utils/fdt/fdt_driver.h>
#include <sbi_utils/i2c/i2c.h>

/** Get I2C adapter identified by nodeoff */
int fdt_i2c_adapter_get(const void *fdt, int nodeoff,
			struct i2c_adapter **out_adapter);

#endif
