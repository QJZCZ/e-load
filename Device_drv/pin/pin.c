/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-08-21     RiceChen     the first version
 * 2026-03-26     zcz         add comments
 */

#include <board.h>
#include "pin.h"

#define RT_USING_PIN//调试

#ifdef RT_USING_PIN

/**
 * @brief 引脚编码宏
 * @param port 端口号 (0-15)
 * @param no 引脚号 (0-15)
 * @return 编码后的引脚值
 */
#define PIN_NUM(port, no) (((((port) & 0xFu) << 4) | ((no) & 0xFu)))

/**
 * @brief 从引脚编码中获取端口号
 * @param pin 编码后的引脚值
 * @return 端口号 (0-15)
 */
#define PIN_PORT(pin) ((uint8_t)(((pin) >> 4) & 0xFu))

/**
 * @brief 从引脚编码中获取引脚号
 * @param pin 编码后的引脚值
 * @return 引脚号 (0-15)
 */
#define PIN_NO(pin) ((uint8_t)((pin) & 0xFu))

/**
 * @brief 根据引脚编码获取GPIO端口指针
 * @param pin 编码后的引脚值
 * @return GPIO端口指针
 */
#define PIN_STPORT(pin) ((GPIO_TypeDef *)(GPIOA_BASE + (0x400u * PIN_PORT(pin))))

/**
 * @brief 根据引脚编码获取GPIO引脚位
 * @param pin 编码后的引脚值
 * @return GPIO引脚位 (如GPIO_PIN_0)
 */
#define PIN_STPIN(pin) ((uint16_t)(1u << PIN_NO(pin)))

/**
 * @brief STM32端口最大数量
 */
#define __STM32_PORT_MAX 8u

/**
 * @brief 支持的端口最大数量
 */
#define PIN_STPORT_MAX __STM32_PORT_MAX

/**
 * @brief 引脚中断映射结构
 */
struct pin_irq_map
{
    uint16_t pinbit;  /* 引脚位 */
    IRQn_Type irqno;  /* 中断号 */
};

/**
 * @brief 引脚中断映射表
 * 用于将GPIO引脚映射到对应的外部中断
 */
static const struct pin_irq_map pin_irq_map[] =
{
    {GPIO_PIN_0, EXTI0_IRQn},
    {GPIO_PIN_1, EXTI1_IRQn},
    {GPIO_PIN_2, EXTI2_IRQn},
    {GPIO_PIN_3, EXTI3_IRQn},
    {GPIO_PIN_4, EXTI4_IRQn},
    {GPIO_PIN_5, EXTI9_5_IRQn},
    {GPIO_PIN_6, EXTI9_5_IRQn},
    {GPIO_PIN_7, EXTI9_5_IRQn},
    {GPIO_PIN_8, EXTI9_5_IRQn},
    {GPIO_PIN_9, EXTI9_5_IRQn},
    {GPIO_PIN_10, EXTI15_10_IRQn},
    {GPIO_PIN_11, EXTI15_10_IRQn},
    {GPIO_PIN_12, EXTI15_10_IRQn},
    {GPIO_PIN_13, EXTI15_10_IRQn},
    {GPIO_PIN_14, EXTI15_10_IRQn},
    {GPIO_PIN_15, EXTI15_10_IRQn},
};

/**
 * @brief 引脚中断处理函数表
 */
static struct rt_pin_irq_hdr pin_irq_hdr_tab[] =
{
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
};

/**
 * @brief 引脚中断使能掩码
 * 用于跟踪哪些引脚的中断已启用
 */
static uint32_t pin_irq_enable_mask = 0;

/**
 * @brief 计算数组元素数量
 * @param items 数组
 * @return 数组元素数量
 */
#define ITEM_NUM(items) sizeof(items) / sizeof(items[0])

/**
 * @brief 通过引脚名称获取引脚编码
 * @param name 引脚名称，如"PA.0"
 * @return 成功返回引脚编码，失败返回错误码
 */
rt_base_t rt_pin_get(const char *name)
{
    rt_base_t pin = 0;
    int hw_port_num, hw_pin_num = 0;
    int i, name_len;

    name_len = rt_strlen(name);

    /* 检查引脚名称长度是否合法 */
    if ((name_len < 4) || (name_len >= 6))
    {
        return -RT_EINVAL;
    }
    /* 检查引脚名称格式是否合法 */
    if ((name[0] != 'P') || (name[2] != '.'))
    {
        return -RT_EINVAL;
    }

    /* 解析端口号 */
    if ((name[1] >= 'A') && (name[1] <= 'Z'))
    {
        hw_port_num = (int)(name[1] - 'A');
    }
    else
    {
        return -RT_EINVAL;
    }

    /* 解析引脚号 */
    for (i = 3; i < name_len; i++)
    {
        hw_pin_num *= 10;
        hw_pin_num += name[i] - '0';
    }

    /* 生成引脚编码 */
    pin = PIN_NUM(hw_port_num, hw_pin_num);

    return pin;
}

/**
 * @brief 写入引脚值
 * @param pin 引脚编码
 * @param value 引脚值 (PIN_LOW 或 PIN_HIGH)
 */
void rt_pin_write(rt_base_t pin, rt_base_t value)
{
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;

    /* 检查端口号是否合法 */
    if (PIN_PORT(pin) < PIN_STPORT_MAX)
    {
        /* 获取GPIO端口和引脚 */
        gpio_port = PIN_STPORT(pin);
        gpio_pin = PIN_STPIN(pin);

        /* 写入引脚值 */
        HAL_GPIO_WritePin(gpio_port, gpio_pin, (GPIO_PinState)value);
    }
}

/**
 * @brief 读取引脚值
 * @param pin 引脚编码
 * @return 引脚值 (PIN_LOW 或 PIN_HIGH)
 */
int rt_pin_read(rt_base_t pin)
{
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;
    int value = PIN_LOW;

    /* 检查端口号是否合法 */
    if (PIN_PORT(pin) < PIN_STPORT_MAX)
    {
        /* 获取GPIO端口和引脚 */
        gpio_port = PIN_STPORT(pin);
        gpio_pin = PIN_STPIN(pin);
        /* 读取引脚值 */
        value = HAL_GPIO_ReadPin(gpio_port, gpio_pin);
    }

    return value;
}

/**
 * @brief 设置引脚模式
 * @param pin 引脚编码
 * @param mode 引脚模式 (如PIN_MODE_OUTPUT, PIN_MODE_INPUT等)
 */
void rt_pin_mode(rt_base_t pin, rt_base_t mode)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /* 检查端口号是否合法 */
    if (PIN_PORT(pin) >= PIN_STPORT_MAX)
    {
        return;
    }

    /* 配置GPIO初始化结构体 */
    GPIO_InitStruct.Pin = PIN_STPIN(pin);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    /* 根据模式设置GPIO参数 */
    if (mode == PIN_MODE_OUTPUT)
    {
        /* 输出模式：推挽输出，无上下拉 */
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    }
    else if (mode == PIN_MODE_INPUT)
    {
        /* 输入模式：无上下拉 */
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    }
    else if (mode == PIN_MODE_INPUT_PULLUP)
    {
        /* 输入模式：上拉 */
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
    }
    else if (mode == PIN_MODE_INPUT_PULLDOWN)
    {
        /* 输入模式：下拉 */
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    }
    else if (mode == PIN_MODE_OUTPUT_OD)
    {
        /* 输出模式：开漏输出，无上下拉 */
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    }

    /* 初始化GPIO */
    HAL_GPIO_Init(PIN_STPORT(pin), &GPIO_InitStruct);
}

/**
 * @brief 将位值转换为位号
 * @param bit 位值
 * @return 位号 (0-31)，失败返回-1
 */
rt_inline rt_int32_t bit2bitno(rt_uint32_t bit)
{
    int i;
    for (i = 0; i < 32; i++)
    {
        if ((0x01 << i) == bit)
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 根据引脚位获取中断映射
 * @param pinbit 引脚位
 * @return 中断映射指针，失败返回RT_NULL
 */
rt_inline const struct pin_irq_map *get_pin_irq_map(uint32_t pinbit)
{
    rt_int32_t mapindex = bit2bitno(pinbit);
    if (mapindex < 0 || mapindex >= ITEM_NUM(pin_irq_map))
    {
        return RT_NULL;
    }
    return &pin_irq_map[mapindex];
};

/**
 * @brief 附加引脚中断处理函数
 * @param pin 引脚编码
 * @param mode 中断模式 (如PIN_IRQ_MODE_RISING等)
 * @param hdr 中断处理函数
 * @param args 中断处理函数参数
 * @return 成功返回RT_EOK，失败返回错误码
 */
rt_err_t rt_pin_attach_irq(rt_int32_t pin, rt_uint32_t mode, void (*hdr)(void *args), void *args)
{
    rt_base_t level;
    rt_int32_t irqindex = -1;

    /* 检查端口号是否合法 */
    if (PIN_PORT(pin) >= PIN_STPORT_MAX)
    {
        return -RT_ENOSYS;
    }

    /* 获取中断索引 */
    irqindex = bit2bitno(PIN_STPIN(pin));
    if (irqindex < 0 || irqindex >= ITEM_NUM(pin_irq_map))
    {
        return RT_ENOSYS;
    }

    /* 禁用中断 */
    level = rt_hw_interrupt_disable();
    /* 检查是否已经附加了相同的中断处理函数 */
    if (pin_irq_hdr_tab[irqindex].pin == pin &&
        pin_irq_hdr_tab[irqindex].hdr == hdr &&
        pin_irq_hdr_tab[irqindex].mode == mode &&
        pin_irq_hdr_tab[irqindex].args == args)
    {
        rt_hw_interrupt_enable(level);
        return RT_EOK;
    }
    /* 检查引脚是否已经附加了中断处理函数 */
    if (pin_irq_hdr_tab[irqindex].pin != -1)
    {
        rt_hw_interrupt_enable(level);
        return RT_EBUSY;
    }
    /* 附加中断处理函数 */
    pin_irq_hdr_tab[irqindex].pin = pin;
    pin_irq_hdr_tab[irqindex].hdr = hdr;
    pin_irq_hdr_tab[irqindex].mode = mode;
    pin_irq_hdr_tab[irqindex].args = args;
    /* 启用中断 */
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

/**
 * @brief 分离引脚中断处理函数
 * @param pin 引脚编码
 * @return 成功返回RT_EOK，失败返回错误码
 */
rt_err_t rt_pin_detach_irq(rt_int32_t pin)
{
    rt_base_t level;
    rt_int32_t irqindex = -1;

    /* 检查端口号是否合法 */
    if (PIN_PORT(pin) >= PIN_STPORT_MAX)
    {
        return -RT_ENOSYS;
    }

    /* 获取中断索引 */
    irqindex = bit2bitno(PIN_STPIN(pin));
    if (irqindex < 0 || irqindex >= ITEM_NUM(pin_irq_map))
    {
        return RT_ENOSYS;
    }

    /* 禁用中断 */
    level = rt_hw_interrupt_disable();
    /* 检查引脚是否已经附加了中断处理函数 */
    if (pin_irq_hdr_tab[irqindex].pin == -1)
    {
        rt_hw_interrupt_enable(level);
        return RT_EOK;
    }
    /* 分离中断处理函数 */
    pin_irq_hdr_tab[irqindex].pin = -1;
    pin_irq_hdr_tab[irqindex].hdr = RT_NULL;
    pin_irq_hdr_tab[irqindex].mode = 0;
    pin_irq_hdr_tab[irqindex].args = RT_NULL;
    /* 启用中断 */
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

/**
 * @brief 启用或禁用引脚中断
 * @param pin 引脚编码
 * @param enabled 启用或禁用 (PIN_IRQ_ENABLE 或 PIN_IRQ_DISABLE)
 * @return 成功返回RT_EOK，失败返回错误码
 */
rt_err_t rt_pin_irq_enable(rt_base_t pin, rt_uint32_t enabled)
{
    const struct pin_irq_map *irqmap;
    rt_base_t level;
    rt_int32_t irqindex = -1;
    GPIO_InitTypeDef GPIO_InitStruct;

    /* 检查端口号是否合法 */
    if (PIN_PORT(pin) >= PIN_STPORT_MAX)
    {
        return -RT_ENOSYS;
    }

    if (enabled == PIN_IRQ_ENABLE)
    {
        /* 获取中断索引 */
        irqindex = bit2bitno(PIN_STPIN(pin));
        if (irqindex < 0 || irqindex >= ITEM_NUM(pin_irq_map))
        {
            return RT_ENOSYS;
        }

        /* 禁用中断 */
        level = rt_hw_interrupt_disable();

        /* 检查引脚是否已经附加了中断处理函数 */
        if (pin_irq_hdr_tab[irqindex].pin == -1)
        {
            rt_hw_interrupt_enable(level);
            return RT_ENOSYS;
        }

        /* 获取中断映射 */
        irqmap = &pin_irq_map[irqindex];

        /* 配置GPIO初始化结构体 */
        GPIO_InitStruct.Pin = PIN_STPIN(pin);
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        /* 根据中断模式设置GPIO参数 */
        switch (pin_irq_hdr_tab[irqindex].mode)
        {
        case PIN_IRQ_MODE_RISING:
            GPIO_InitStruct.Pull = GPIO_PULLDOWN;
            GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
            break;
        case PIN_IRQ_MODE_FALLING:
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
            break;
        case PIN_IRQ_MODE_RISING_FALLING:
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
            break;
        }
        /* 初始化GPIO */
        HAL_GPIO_Init(PIN_STPORT(pin), &GPIO_InitStruct);

        /* 配置NVIC */
        HAL_NVIC_SetPriority(irqmap->irqno, 5, 0);
        HAL_NVIC_EnableIRQ(irqmap->irqno);
        /* 更新中断使能掩码 */
        pin_irq_enable_mask |= irqmap->pinbit;

        /* 启用中断 */
        rt_hw_interrupt_enable(level);
    }
    else if (enabled == PIN_IRQ_DISABLE)
    {
        /* 获取中断映射 */
        irqmap = get_pin_irq_map(PIN_STPIN(pin));
        if (irqmap == RT_NULL)
        {
            return RT_ENOSYS;
        }

        /* 禁用中断 */
        level = rt_hw_interrupt_disable();

        /* 反初始化GPIO */
        HAL_GPIO_DeInit(PIN_STPORT(pin), PIN_STPIN(pin));

        /* 更新中断使能掩码 */
        pin_irq_enable_mask &= ~irqmap->pinbit;
        /* 检查是否需要禁用NVIC中断 */
        if ((irqmap->pinbit >= GPIO_PIN_5) && (irqmap->pinbit <= GPIO_PIN_9))
        {
            if (!(pin_irq_enable_mask & (GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9)))
            {
                HAL_NVIC_DisableIRQ(irqmap->irqno);
            }
        }
        else if ((irqmap->pinbit >= GPIO_PIN_10) && (irqmap->pinbit <= GPIO_PIN_15))
        {
            if (!(pin_irq_enable_mask & (GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15)))
            {
                HAL_NVIC_DisableIRQ(irqmap->irqno);
            }
        }
        else
        {
            HAL_NVIC_DisableIRQ(irqmap->irqno);
        }
        /* 启用中断 */
        rt_hw_interrupt_enable(level);
    }
    else
    {
        return -RT_ENOSYS;
    }

    return RT_EOK;
}

/**
 * @brief 引脚中断处理函数
 * @param irqno 中断号
 */
rt_inline void pin_irq_hdr(int irqno)
{
    if (pin_irq_hdr_tab[irqno].hdr)
    {
        pin_irq_hdr_tab[irqno].hdr(pin_irq_hdr_tab[irqno].args);
    }
}

/**
 * @brief HAL GPIO外部中断回调函数
 * @param GPIO_Pin GPIO引脚
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    pin_irq_hdr(bit2bitno(GPIO_Pin));
}

/**
 * @brief EXTI0中断处理函数
 */
void EXTI0_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
    rt_interrupt_leave();
}

/**
 * @brief EXTI1中断处理函数
 */
void EXTI1_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
    rt_interrupt_leave();
}

/**
 * @brief EXTI2中断处理函数
 */
void EXTI2_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
    rt_interrupt_leave();
}

/**
 * @brief EXTI3中断处理函数
 */
void EXTI3_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
    rt_interrupt_leave();
}

/**
 * @brief EXTI4中断处理函数
 */
void EXTI4_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
    rt_interrupt_leave();
}

/**
 * @brief EXTI9_5中断处理函数
 */
void EXTI9_5_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
    rt_interrupt_leave();
}

/**
 * @brief EXTI15_10中断处理函数
 */
void EXTI15_10_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
    rt_interrupt_leave();
}

/**
 * @brief 初始化硬件引脚
 * @return 成功返回RT_EOK
 */
int rt_hw_pin_init(void)
{
#if defined(__HAL_RCC_GPIOA_CLK_ENABLE)
    __HAL_RCC_GPIOA_CLK_ENABLE();
#endif

#if defined(__HAL_RCC_GPIOB_CLK_ENABLE)
    __HAL_RCC_GPIOB_CLK_ENABLE();
#endif

#if defined(__HAL_RCC_GPIOC_CLK_ENABLE)
    __HAL_RCC_GPIOC_CLK_ENABLE();
#endif

#if defined(__HAL_RCC_GPIOE_CLK_ENABLE)
    __HAL_RCC_GPIOE_CLK_ENABLE();
#endif

    return RT_EOK;
}

#endif /* RT_USING_PIN */
