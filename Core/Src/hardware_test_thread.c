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
#include "stm32f3xx_hal_flash.h"
#include "stm32f3xx_hal_flash_ex.h"

/* 外部变量声明 */
extern rt_base_t led_orange_pin;
extern rt_base_t led_blue_pin;

/* ADC和DAC句柄 */
extern ADC_HandleTypeDef hadc2;
extern DAC_HandleTypeDef hdac1;

/* Flash存储配置 */
#define FLASH_USER_START_ADDR   0x0800F800  // 从Flash末尾开始，预留2KB空间
#define FLASH_USER_END_ADDR     0x08010000  // Flash结束地址 (64KB芯片)



/* 全局变量，用于存储ADC值 */
uint32_t g_adc_current_value = 0;
float g_current_voltage = 0.0f;
uint32_t g_adc_voltage_value = 0;
float g_voltage = 0.0f;
float g_power = 0.0f;
float g_resistance = 0.0f;
TargetValues target_values = {0.0f, 0.0f, 0.0f};
Mode current_mode = MODE_CONST_CURRENT;




/* 全局变量，用于控制DAC输出 */
float g_dac_voltage = 0.0f;
uint8_t g_dac_enabled = 0; // 0: 关闭，1: 开启




/* PID参数 */
#define PID_KP 2.2f  // 比例系数
#define PID_KI 0.2f  // 积分系数
#define PID_KD 0.00f // 微分系数

/* PID相关变量 */
static float pid_error = 0.0f;      // 误差
static float pid_error_sum = 0.0f;   // 误差积分
static float pid_error_last = 0.0f;  // 上一次误差
static float pid_output = 0.0f;      // PID输出

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
    sConfig.SamplingTime = ADC_SAMPLETIME_61CYCLES_5;
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
    sConfig.SamplingTime = ADC_SAMPLETIME_61CYCLES_5;
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
    // 确保DAC始终开启
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
    
    // 计算对应的DAC计数值（0-4095）
    uint16_t value;
    if (g_dac_enabled)
    {
        // 当启用时，使用设定的电压值
        value = (uint16_t)(voltage / 3.312f * 4095.0f);
        // 确保值在有效范围内
        if (value > 4095) value = 4095;
        if (value < 0) value = 0;
    }
    else
    {
        // 当禁用时，输出0V
        value = 0;
    }
    
    // 设置DAC输出值
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value);
    
  
}

/* PID算法函数（带前馈控制） */
static float pid_control(float target, float actual)
{
    // 前馈控制量：根据电流和DAC输出的1:1关系计算
    float feedforward = 0.9f*target; // 因为电流和DAC输出的关系是1:1
    
    // 计算误差
    pid_error = target - actual;
    
    // 计算误差积分
    pid_error_sum += pid_error * 0.02f; // 0.02f是采样时间（20ms）
    
    // 限制误差积分范围，防止积分饱和
    if (pid_error_sum > 100.0f) pid_error_sum = 100.0f;
    if (pid_error_sum < -100.0f) pid_error_sum = -100.0f;
    
    // 计算误差微分
    float pid_error_diff = (pid_error - pid_error_last) / 0.02f;
    
    // 计算PID输出
    float pid_output = PID_KP * pid_error + PID_KI * pid_error_sum + PID_KD * pid_error_diff;
    
    // 保存上一次误差
    pid_error_last = pid_error;
    
    // 总输出 = 前馈控制量 + PID控制量
    float total_output = feedforward + pid_output;
    
    // 限制总输出范围
    if (total_output > 3.3f) total_output = 3.3f;
    if (total_output < 0.0f) total_output = 0.0f;
    
    return total_output;
}

/* 开环控制函数 */
static float open_loop_control(float target)
{
    // 直接根据目标值输出，利用电流和DAC输出的 y = 0.933947x + 72.404692
    float output =(target*1000/0.933947f-72.404692f)/1000;
    
    // 限制输出范围
    if (output > 3.3f) output = 3.3f;
    if (output < 0.0f) output = 0.0f;
    
    return output;
}

/* 硬件测试线程入口 */
static void hardware_test_thread_entry(void *parameter)
{
    uint32_t adc_current_value;
    uint32_t adc_voltage_value;
 
    // 校准 ADC
    rt_kprintf("Calibrating ADC...\n");
    if ( HAL_ADCEx_Calibration_Start(&hadc2,ADC_SINGLE_ENDED) != HAL_OK)
    {
        rt_kprintf("ADC calibration failed!\n");
    }
    else
    {
        rt_kprintf("ADC calibration successful!\n");
    }
    
    // 设置橙灯闪烁，亮500ms，灭500ms
   // qled_set_blink(led_orange_pin, 500, 500);
    
    while (1)
    {
        // ADC测试：读取电流值（通道15）
        adc_current_value = adc_test_current();
        g_adc_current_value = adc_current_value;
        g_current_voltage = (float)adc_current_value / 4095.0f * 3.312f;
        g_current_voltage=g_current_voltage/1.1f-0.013;
        g_current_voltage=g_current_voltage*1.0374f - 0.0056f;
        // ADC测试：读取电压值（通道14）
        adc_voltage_value = adc_test_voltage();
        g_adc_voltage_value = adc_voltage_value;
        g_voltage = 2.0f * (float)adc_voltage_value / 4095.0f * 3.312f-0.03;
        g_power = g_current_voltage * g_voltage;
        g_resistance = g_voltage / g_current_voltage;
        // 使用开环控制输出
        if (g_dac_enabled)
        {
            // 计算开环输出
            if (current_mode == MODE_CONST_CURRENT)
            {
                g_dac_voltage = open_loop_control(target_values.g_target_current);
            }
            else if (current_mode == MODE_CONST_POWER)
            {
                 // 对于功率模式，需要根据电压计算目标电流
                 float target_current = target_values.g_target_power / g_voltage;
                 g_dac_voltage = open_loop_control(target_current);
            }
            else if (current_mode == MODE_CONST_RESISTANCE)
            {
                // 对于电阻模式，需要根据电压计算目标电流
                float target_current = g_voltage / target_values.g_target_resistance;
                g_dac_voltage = open_loop_control(target_current);
            }
            qled_set_blink(led_orange_pin, 500, 500);
        }else
        {

          qled_set_off(led_orange_pin);
        }
        dac_test(g_dac_voltage);
     
        // 线程延时100ms，提高PID控制响应速度

        rt_thread_mdelay(20);
    }
}

/* 解锁Flash */
static void unlock_flash(void)
{
    HAL_FLASH_Unlock();
}

/* 锁定Flash */
static void lock_flash(void)
{
    HAL_FLASH_Lock();
}

/* 擦除指定扇区 */
static HAL_StatusTypeDef erase_flash_sector(void)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t error;

    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = FLASH_USER_START_ADDR;
    erase_init.NbPages = 1;
    

    return HAL_FLASHEx_Erase(&erase_init, &error);
}

/* 写入数据到Flash */
static HAL_StatusTypeDef write_flash(uint32_t address, uint32_t data)
{
    return HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data);
}

/* 从Flash读取数据 */
static uint32_t read_flash(uint32_t address)
{
    return *(uint32_t*)address;
}

/* 从Flash读取配置 */
static void flash_read_config(void)
{
    // 解锁Flash
    unlock_flash();
    
    // 读取Flash数据
    TargetValues *flash_config = (TargetValues *)FLASH_USER_START_ADDR;
    
    // 检查配置是否有效（简单校验）
    if (flash_config->g_target_current >= 0.0f && flash_config->g_target_current <= 3.0f &&
        flash_config->g_target_power >= 0.0f && flash_config->g_target_power <= 3.0f &&
        flash_config->g_target_resistance >= 0.0f && flash_config->g_target_resistance <= 30.0f)
    {
        // 配置有效，读取到全局变量
        target_values = *flash_config;
        rt_kprintf("Flash config loaded: I=%.2fA, P=%.2fW, R=%.2fΩ\n", 
                   target_values.g_target_current, 
                   target_values.g_target_power, 
                   target_values.g_target_resistance);
    }
    else
    {
        // 配置无效，使用默认值
        target_values.g_target_current = 0.5f;
        target_values.g_target_power = 0.5f;
        target_values.g_target_resistance = 10.0f;
        rt_kprintf("Flash config invalid, using default values\n");
    }
    
    // 锁定Flash
    lock_flash();
}

/* 将配置写入Flash */
void flash_write_config(void)
{
    HAL_StatusTypeDef status;
    
    // 解锁Flash
    unlock_flash();
    
    // 擦除指定扇区
    status = erase_flash_sector();
    if (status != HAL_OK)
    {
        rt_kprintf("Flash erase failed: %d\n", status);
        lock_flash();
        return;
    }
    
    // 写入配置数据
    uint32_t *data = (uint32_t *)&target_values;
    uint32_t size = sizeof(TargetValues) / sizeof(uint32_t);
    
    for (uint32_t i = 0; i < size; i++)
    {
        status = write_flash(FLASH_USER_START_ADDR + i * 4, data[i]);
        if (status != HAL_OK)
        {
            rt_kprintf("Flash write failed: %d at address 0x%08lx\n", status, FLASH_USER_START_ADDR + i * 4);
            lock_flash();
            return;
        }
    }
    
    // 锁定Flash
    lock_flash();
    
    // 验证写入
    uint32_t verify_data[sizeof(TargetValues)/4];
    for (uint32_t i = 0; i < size; i++)
    {
        verify_data[i] = read_flash(FLASH_USER_START_ADDR + i*4);
        if (verify_data[i] != data[i])
        {
            rt_kprintf("Flash write verify failed at address 0x%08lx\n", FLASH_USER_START_ADDR + i*4);
            return;
        }
    }
    
    rt_kprintf("Flash config saved: I=%.2fA, P=%.2fW, R=%.2fΩ\n", 
               target_values.g_target_current, 
               target_values.g_target_power, 
               target_values.g_target_resistance);
}

/* 初始化硬件测试线程 */
int hardware_test_thread_init(void)
{
    // 从Flash读取配置
    flash_read_config();
    
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
