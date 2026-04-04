/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-03     zcz          create drv_i2c.h
 */

#ifndef DRV_I2C_H__
#define DRV_I2C_H__

#include "i2c.h"

#ifdef RT_USING_I2C1
#define RT_I2C1_CONFIG \
{ \
    .name = "i2c1", \
    .scl = 15, /* PA15: I2C1_SCL */ \
    .sda = 23,  /* PB7: I2C1_SDA */ \
}
#endif

#ifdef RT_USING_I2C2
#define RT_I2C2_CONFIG \
{ \
    .name = "i2c2", \
    .scl = 10, /* PB10: I2C2_SCL */ \
    .sda = 11, /* PB11: I2C2_SDA */ \
}
#endif

#endif /* DRV_I2C_H__ */
