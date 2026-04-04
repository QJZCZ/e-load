/*
 * Copyright (c) 2006-2024 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-01     zcz          Initial version
 */

#include <rthw.h>
#include <rtthread.h>
#include "main.h"

/* 外部函数声明 */
extern void SystemClock_Config(void);
extern void MX_GPIO_Init(void);
extern void MX_ADC2_Init(void);
extern void MX_DAC1_Init(void);
extern void MX_I2C1_Init(void);
extern void MX_USART1_UART_Init(void);
extern void MX_USART3_UART_Init(void);

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

/* 外部设备驱动初始化函数声明 */
extern int rt_hw_pin_init(void);

void rt_hw_console_output(const char *str)
{
    while (*str)
    {
        HAL_UART_Transmit(&huart3, (uint8_t*)str, 1, 1000);
        str++;
    }
}

/**
 * @brief  Board initialization function
 * @return None
 */
void rt_hw_board_init(void)
{
    /* 初始化HAL库 */
    HAL_Init();
    
    /* 配置系统时钟 */
    SystemClock_Config();
    
    /* 初始化GPIO */
    MX_GPIO_Init();
    
    /* 初始化ADC */
    MX_ADC2_Init();
    
    /* 初始化DAC */
    MX_DAC1_Init();
    
    /* 初始化I2C */
    MX_I2C1_Init();
    
    /* 初始化UART1 */
    MX_USART1_UART_Init();
    
    /* 初始化UART3 */
    MX_USART3_UART_Init();
    
 
    /* 初始化PIN设备驱动 */
    rt_hw_pin_init();
    
    /* 初始化SysTick */
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / RT_TICK_PER_SECOND);
    
    /* 设置SysTick中断优先级 */
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}
