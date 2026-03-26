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

/**
 * @brief  Board initialization function
 * @return None
 */
void rt_hw_board_init(void)
{
    /* 初始化SysTick */
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / RT_TICK_PER_SECOND);
    
    /* 设置SysTick中断优先级 */
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}
