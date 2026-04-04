/*
 * Copyright (c) 2024-2026 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-03     zcz          Initial version
 */

#include "button_scan_thread.h"
#include <rtthread.h>
#include "pin.h"
#include "qled.h"
#include "flexible_button.h"

/* 外部变量声明 */
extern rt_base_t led_orange_pin;
extern rt_base_t led_blue_pin;

/* 按钮定义 */
static flex_button_t button_right; // 右
static flex_button_t button_middle; // 中
static flex_button_t button_down; // 下
static flex_button_t button_left; // 左
static flex_button_t button_up; // 上

/* 静态线程栈 */
static char button_scan_thread_stack[256] __attribute__((aligned(RT_ALIGN_SIZE)));

/* 线程控制块 */
static struct rt_thread button_scan_thread;

/* 按钮回调函数 */
static void button_callback(void* arg);

/* 按钮读取函数 */
static uint8_t button_right_read(void* arg);
static uint8_t button_middle_read(void* arg);
static uint8_t button_down_read(void* arg);
static uint8_t button_left_read(void* arg);
static uint8_t button_up_read(void* arg);

/* 按钮扫描线程入口 */
static void button_scan_thread_entry(void *parameter);

/* 按钮回调函数 */
static void button_callback(void* arg)
{
    flex_button_t* btn = (flex_button_t*)arg;
    
    switch (btn->event)
    {
    case FLEX_BTN_PRESS_CLICK:
        if (btn->id == 1)
        {
            rt_kprintf("右 button clicked: Output to serial port\n");
        }
        else if (btn->id == 2)
        {
            rt_kprintf("中 button clicked: Output to serial port\n");
        }
        else if (btn->id == 3)
        {
            rt_kprintf("下 button clicked: Output to serial port\n");
        }
        else if (btn->id == 4)
        {
            rt_kprintf("左 button clicked: Output to serial port\n");
        }
        else if (btn->id == 5)
        {
            rt_kprintf("上 button clicked: Output to serial port\n");
        }
        break;
    default:
        break;
    }
}

/* 按钮读取函数 */
static uint8_t button_right_read(void* arg)
{
    /* 右: PC11 */
    rt_base_t pin = rt_pin_get("PC.11");
    return rt_pin_read(pin);
}

static uint8_t button_middle_read(void* arg)
{
    /* 中: PB4 */
    rt_base_t pin = rt_pin_get("PB.4");
    return rt_pin_read(pin);
}

static uint8_t button_down_read(void* arg)
{
    /* 下: PC12 */
    rt_base_t pin = rt_pin_get("PC.12");
    return rt_pin_read(pin);
}

static uint8_t button_left_read(void* arg)
{
    /* 左: PB3 */
    rt_base_t pin = rt_pin_get("PB.3");
    return rt_pin_read(pin);
}

static uint8_t button_up_read(void* arg)
{
    /* 上: PD2 */
    rt_base_t pin = rt_pin_get("PD.2");
    return rt_pin_read(pin);
}

/* 按钮扫描线程入口 */
static void button_scan_thread_entry(void *parameter)
{
    while (1)
    {
        flex_button_scan();
        rt_thread_mdelay(20); /* 20ms扫描一次 */
    }
}

/* 初始化按钮扫描线程 */
int button_scan_thread_init(void)
{
  // 初始化按钮1 (右: PC11)
  button_right.id = 1;
  button_right.usr_button_read = button_right_read;
  button_right.cb = button_callback;
  button_right.pressed_logic_level = 0; // 按键按下为低电平
  button_right.short_press_start_tick = FLEX_MS_TO_SCAN_CNT(500);
  button_right.long_press_start_tick = FLEX_MS_TO_SCAN_CNT(1000);
  button_right.long_hold_start_tick = FLEX_MS_TO_SCAN_CNT(2000);
  
  // 初始化按钮2 (中: PB4)
  button_middle.id = 2;
  button_middle.usr_button_read = button_middle_read;
  button_middle.cb = button_callback;
  button_middle.pressed_logic_level = 0; // 按键按下为低电平
  button_middle.short_press_start_tick = FLEX_MS_TO_SCAN_CNT(500);
  button_middle.long_press_start_tick = FLEX_MS_TO_SCAN_CNT(1000);
  button_middle.long_hold_start_tick = FLEX_MS_TO_SCAN_CNT(2000);
  
  // 初始化按钮3 (下: PC12)
  button_down.id = 3;
  button_down.usr_button_read = button_down_read;
  button_down.cb = button_callback;
  button_down.pressed_logic_level = 0; // 按键按下为低电平
  button_down.short_press_start_tick = FLEX_MS_TO_SCAN_CNT(500);
  button_down.long_press_start_tick = FLEX_MS_TO_SCAN_CNT(1000);
  button_down.long_hold_start_tick = FLEX_MS_TO_SCAN_CNT(2000);
  
  // 初始化按钮4 (左: PB3)
  button_left.id = 4;
  button_left.usr_button_read = button_left_read;
  button_left.cb = button_callback;
  button_left.pressed_logic_level = 0; // 按键按下为低电平
  button_left.short_press_start_tick = FLEX_MS_TO_SCAN_CNT(500);
  button_left.long_press_start_tick = FLEX_MS_TO_SCAN_CNT(1000);
  button_left.long_hold_start_tick = FLEX_MS_TO_SCAN_CNT(2000);
  
  // 初始化按钮5 (上: PD2)
  button_up.id = 5;
  button_up.usr_button_read = button_up_read;
  button_up.cb = button_callback;
  button_up.pressed_logic_level = 0; // 按键按下为低电平
  button_up.short_press_start_tick = FLEX_MS_TO_SCAN_CNT(500);
  button_up.long_press_start_tick = FLEX_MS_TO_SCAN_CNT(1000);
  button_up.long_hold_start_tick = FLEX_MS_TO_SCAN_CNT(2000);
  
  // 注册按钮
  flex_button_register(&button_right);
  flex_button_register(&button_middle);
  flex_button_register(&button_down);
  flex_button_register(&button_left);
  flex_button_register(&button_up);
  
  // 创建按钮扫描线程
  rt_thread_init(&button_scan_thread,                // 线程控制块
                 "button_scan",                    // 线程名称
                 button_scan_thread_entry,          // 线程入口函数
                 RT_NULL,                          // 线程参数
                 button_scan_thread_stack,          // 线程栈
                 sizeof(button_scan_thread_stack),   // 栈大小
                 7,                                // 优先级
                 10);                              // 时间片
  
  // 启动按钮扫描线程
  rt_thread_startup(&button_scan_thread);
  
  return RT_EOK;
}
