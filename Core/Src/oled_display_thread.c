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

#include "oled_display_thread.h"
#include <rtthread.h>
#include "oled.h"
#include "button_scan_thread.h"
#include "hardware_test_thread.h"
/* 外部全局变量声明 */
extern uint32_t g_adc_current_value;
extern float g_current_voltage;
extern uint32_t g_adc_voltage_value;
extern float g_voltage;
extern float g_dac_voltage;
extern uint8_t g_dac_enabled;

extern float g_power;
extern float g_resistance;

/* 外部变量声明，用于模式和目标值 */
extern TargetValues target_values;
extern Mode current_mode;


/* 静态线程栈 */
static char oled_display_thread_stack[2048] __attribute__((aligned(RT_ALIGN_SIZE)));

/* 线程控制块 */
static struct rt_thread oled_display_thread;

/* 当前页面 */
uint8_t current_page = PAGE_MODE_SELECT;


/* 页面切换函数 */
void oled_switch_page(uint8_t page)
{
    current_page = page;
}

/* 输出页面显示函数 */
static void display_output_page(void)
{
    // 清屏
    //OLED_Clear(0);
    
    // 显示标题
    OLED_ShowString(50, 0, "Output", 16);
    
    // 显示电流值
    OLED_ShowString(0, 10, "C:", 12);
    char current_str[20];
    rt_sprintf(current_str, " %.3fA", g_current_voltage);
    OLED_ShowString(40, 10, (uint8_t*)current_str, 12);
 
    // 显示电压值
    OLED_ShowString(0, 11, "V:", 12);
    char voltage_str[20];
    rt_sprintf(voltage_str, " %.3fV", g_voltage);
    OLED_ShowString(40, 11, (uint8_t*)voltage_str, 12);
   
    
    // 显示功率值
    OLED_ShowString(0, 12, "P:", 12);
    char power_str[20];
    rt_sprintf(power_str, " %.3fW", g_power);
    OLED_ShowString(40, 12, (uint8_t*)power_str, 12);
     
    // 显示电阻值
    OLED_ShowString(0, 13, "R:", 12);
    char resistance_str[20];
    rt_sprintf(resistance_str, " %.3fom", g_resistance);
    OLED_ShowString(40, 13, (uint8_t*)resistance_str, 12);

    // 显示目标电流值
    OLED_ShowString(0, 14, "Target:", 12);
    char target_str[20];
    if(current_mode == MODE_CONST_CURRENT){
        rt_sprintf(target_str, "%.2fA [%s]", target_values.g_target_current, g_dac_enabled ? "ON" : "OFF");
    }
    else if(current_mode == MODE_CONST_POWER){
        rt_sprintf(target_str, "%.2fW [%s]", target_values.g_target_power, g_dac_enabled ? "ON" : "OFF");
    }
    else if(current_mode == MODE_CONST_RESISTANCE){
        rt_sprintf(target_str, "%.2fom [%s]", target_values.g_target_resistance, g_dac_enabled ? "ON" : "OFF");
    }
    OLED_ShowString(20, 15, target_str, 12); 
}

/* 设置页面显示函数 */
static void display_set_page(void)
{
    
    if(current_mode == MODE_CONST_CURRENT){
       // 显示标题
    OLED_ShowString(20, 0, "Set Current ", 16);
    
    // 显示当前电流值
    //OLED_ShowString(0, 10, "Current:", 12);
    char current_str[20];
    rt_sprintf(current_str, " %.3fA", target_values.g_target_current);
    OLED_ShowString(22, 12, (uint8_t*)current_str, 16);
    }
    else if(current_mode == MODE_CONST_POWER){
      // 显示标题
    OLED_ShowString(20, 0, "Set Power ", 16);
    
    // 显示当前功率值
    //OLED_ShowString(0, 10, "Power:", 12);
    char power_str[20];
    rt_sprintf(power_str, " %.3fW", target_values.g_target_power);
    OLED_ShowString(22, 12, (uint8_t*)power_str, 16);
    }
    else if(current_mode == MODE_CONST_RESISTANCE){
      // 显示标题
    OLED_ShowString(20, 0, "Set Res ", 16);
    
    // 显示当前电阻值
    //OLED_ShowString(0, 10, "Resistance:", 12);
    char resistance_str[20];
    rt_sprintf(resistance_str, " %.3fom", target_values.g_target_resistance);
    OLED_ShowString(22, 12, resistance_str, 16);
    }
    
}

/* 模式选择页面显示函数 */
static void display_mode_select_page(void)
{
    // 清屏
   // OLED_Clear(0);
    
    // 显示标题
    OLED_ShowString(50, 0, "Mode", 16);
    
    // 显示模式选项，当前选中的模式高亮显示
    if (current_mode == MODE_CONST_CURRENT) {
        OLED_ShowString(0, 10, "> 1.ConstCurrent", 16);
        OLED_ShowString(0, 13, " 2.ConstRes", 12);
        OLED_ShowString(0, 15, " 3.ConstPower", 12);
    } else if (current_mode == MODE_CONST_RESISTANCE) {
        OLED_ShowString(0, 10, " 1.ConstCurrent", 12);
        OLED_ShowString(0, 12, "> 2.ConstRes", 16);
        OLED_ShowString(0, 15, " 3.ConstPower", 12);
    } else if (current_mode == MODE_CONST_POWER) {
        OLED_ShowString(0, 10, " 1.ConstCurrent", 12);
        OLED_ShowString(0, 12, " 2.ConstRes", 12);
        OLED_ShowString(0, 14, "> 3.ConstPower", 16);
    }
  
}

/* OLED显示线程入口 */
static void oled_display_thread_entry(void *parameter)
{
  rt_kprintf("OLED display thread started\n");
  
  // 初始化 OLED
  rt_kprintf("Initializing OLED...\n");
  OLED_Init();
  rt_kprintf("OLED initialized\n");
  OLED_Clear(0);
  while (1)
  {
    // 根据当前页面显示不同内容
    switch (current_page)
    {
    case PAGE_OUTPUT:
        display_output_page();
        break;
    case PAGE_SET:
        display_set_page();
        break;
    case PAGE_MODE_SELECT:
        display_mode_select_page();
        break;
    default:
        display_mode_select_page();
        break;
    }
    
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
