/*
 * Copyright (c) 2024-2026 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-03     zcz          Initial version
 */

#include "hardware_test_thread.h"
#include <rtthread.h>
#include "qled.h"
#include "main.h"

/* 外部变量声明 */
extern rt_base_t led_orange_pin;
extern rt_base_t led_blue_pin;

/* ADC和DAC句柄 */
extern ADC_HandleTypeDef hadc2;
extern DAC_HandleTypeDef hdac1;

/* 静态线程栈 */
static char hardware_test_thread_stack[512] __attribute__((aligned(RT_ALIGN_SIZE)));

/* 线程控制块 */
static struct rt_thread hardware_test_thread;

/* ADC通道15测试函数（电流检测） */
static uint32_t adc_test_current(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    
    // 配置ADC通道15（电流检测）
    sConfig.Channel = ADC_CHANNEL_15;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    HAL_ADC_ConfigChannel(&hadc2, &sConfig);
    
    HAL_ADC_Start(&hadc2);
    HAL_ADC_PollForConversion(&hadc2, 100);
    uint32_t adc_value = HAL_ADC_GetValue(&hadc2);
    HAL_ADC_Stop(&hadc2);
    return adc_value;
}

/* ADC通道14测试函数（电压检测） */
static uint32_t adc_test_voltage(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    
    // 配置ADC通道14（电压检测）
    sConfig.Channel = ADC_CHANNEL_14;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    HAL_ADC_ConfigChannel(&hadc2, &sConfig);
    
    HAL_ADC_Start(&hadc2);
    HAL_ADC_PollForConversion(&hadc2, 100);
    uint32_t adc_value = HAL_ADC_GetValue(&hadc2);
    HAL_ADC_Stop(&hadc2);
    return adc_value;
}

/* DAC测试函数 */
static void dac_test(float voltage)
{
    // 计算对应的DAC计数值（0-4095）
    uint16_t value = (uint16_t)(voltage / 3.3f * 4095.0f);
    
    // 确保值在有效范围内
    if (value > 4095) value = 4095;
    if (value < 0) value = 0;
    
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value);
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
    
   
}

/* 硬件测试线程入口 */
static void hardware_test_thread_entry(void *parameter)
{
    uint32_t adc_current_value;
    uint32_t adc_voltage_value;
    uint16_t dac_value = 0;
    
    rt_kprintf("Hardware test thread started\n");
    rt_kprintf("Testing ADC2-IN15(Current) and ADC2-IN14(Voltage) and DAC...\n");
    
    // 设置橙灯闪烁，亮500ms，灭500ms
    qled_set_blink(led_orange_pin, 500, 500);
    
    while (1)
    {
        // ADC测试：读取电流值（通道15）
        adc_current_value = adc_test_current();
        float current_voltage = (float)adc_current_value / 4095.0f * 3.3f;
        rt_kprintf("ADC2-IN15(Current): %d (%.2fV)\n", adc_current_value, current_voltage);
        
        // ADC测试：读取电压值（通道14）
        adc_voltage_value = adc_test_voltage();
        float voltage = (float)adc_voltage_value / 4095.0f * 3.3f;
        rt_kprintf("ADC2-IN14(Voltage): %d (%.2fV)\n", adc_voltage_value, voltage);
        
        // DAC测试：输出3.3V
        dac_test(2.000f);
        
        // 线程延时1秒
        rt_thread_mdelay(5000);
    }
}

/* 初始化硬件测试线程 */
int hardware_test_thread_init(void)
{
    // 创建硬件测试线程
    rt_thread_init(&hardware_test_thread,              // 线程控制块
                   "hardware_test",                   // 线程名称
                   hardware_test_thread_entry,        // 线程入口函数
                   RT_NULL,                           // 线程参数
                   hardware_test_thread_stack,        // 线程栈
                   sizeof(hardware_test_thread_stack), // 栈大小
                   8,                                 // 优先级
                   10);                               // 时间片
    
    // 启动线程
    rt_thread_startup(&hardware_test_thread);
    
    return RT_EOK;
}
