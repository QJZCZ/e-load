/*
 * Copyright (c) 2024-2026 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-04     zcz          Initial version
 */

#include "oled_display_thread.h"
#include <rtthread.h>
#include "oled.h"

/* 静态线程栈 */
static char oled_display_thread_stack[2048] __attribute__((aligned(RT_ALIGN_SIZE)));

/* 线程控制块 */
static struct rt_thread oled_display_thread;
//x:0~127
//y:0~63
/* OLED显示线程入口 */
static void oled_display_thread_entry(void *parameter)
{
  rt_kprintf("OLED display thread started\n");
  
  // 初始化 OLED
  rt_kprintf("Initializing OLED...\n");
  OLED_Init();
  rt_kprintf("OLED initialized\n");
  
  // 清屏
  OLED_Clear(0);
  
  // 显示欢迎信息
  OLED_ShowString(50, 50, "RT-Thread", 12);
 
  while (1)
  {
    // 线程延时
    rt_thread_mdelay(1000);
  }
}

/* 初始化OLED显示线程 */
int oled_display_thread_init(void)
{
  // 创建OLED显示线程
  rt_thread_init(&oled_display_thread,              // 线程控制块
                 "oled_display",                    // 线程名称
                 oled_display_thread_entry,          // 线程入口函数
                 RT_NULL,                          // 线程参数
                 oled_display_thread_stack,          // 线程栈
                 sizeof(oled_display_thread_stack),   // 栈大小
                 8,                                // 优先级
                 10);                              // 时间片
  
  // 启动线程
  rt_thread_startup(&oled_display_thread);
  
  return RT_EOK;
}
