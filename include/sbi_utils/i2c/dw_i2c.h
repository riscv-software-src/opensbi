/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 StarFive Technology Co., Ltd.
 *
 * Author: Minda Chen <minda.chen@starfivetech.com>
 */

#ifndef __DW_I2C_H__
#define __DW_I2C_H__

#include <sbi_utils/i2c/i2c.h>

int dw_i2c_init(struct i2c_adapter *, int nodeoff);

struct dw_i2c_adapter {
	unsigned long addr;
	struct i2c_adapter adapter;
};

#endif
