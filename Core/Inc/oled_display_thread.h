/*
 * Copyright (c) 2024-2026 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-04     zcz          Initial version
 * 2026-04-16     zcz          Add page mode
 */

#ifndef __OLED_DISPLAY_THREAD_H__
#define __OLED_DISPLAY_THREAD_H__

#include <rtthread.h>

/* 页面定义 */
#define PAGE_OUTPUT      0  // 输出页面
#define PAGE_SET         1  // 设置页面
#define PAGE_MODE_SELECT 2  // 模式选择页面


/* 外部变量声明 */
extern uint8_t current_page;


/* 函数声明 */
int oled_display_thread_init(void);
void oled_switch_page(uint8_t page);

#endif /* __OLED_DISPLAY_THREAD_H__ */
