/*
 * Copyright (c) 2024-2026 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-03     zcz          Initial version
 */

#ifndef __HARDWARE_TEST_THREAD_H__
#define __HARDWARE_TEST_THREAD_H__

#include <rtthread.h>

typedef struct {
   float g_target_current; // 目标电流值（A）
   float g_target_power; // 目标功率值（W）
   float g_target_resistance; // 目标电阻值（Ω）
} TargetValues;
extern TargetValues target_values;

 typedef enum {
    MODE_CONST_CURRENT = 0,
    MODE_CONST_RESISTANCE = 1,
    MODE_CONST_POWER = 2,
 } Mode;
extern Mode current_mode;



/* 函数声明 */
int hardware_test_thread_init(void);
void flash_write_config(void);

#endif /* __HARDWARE_TEST_THREAD_H__ */
