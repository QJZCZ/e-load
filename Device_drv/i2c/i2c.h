/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-08-21     RiceChen     the first version
 * 2026-03-26     zcz         create i2c.h
 */

#ifndef I2C_H__
#define I2C_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C总线配置结构体
 */
typedef struct rt_i2c_config
{
    const char *name;  /* 总线名称 */
    rt_uint8_t scl;    /* SCL引脚 */
    rt_uint8_t sda;    /* SDA引脚 */
} rt_i2c_config_t;

/**
 * @brief I2C消息结构体
 */
typedef struct rt_i2c_msg
{
    rt_uint16_t addr;     /* 设备地址 */
    rt_uint16_t flags;    /* 标志位 */
    rt_uint16_t len;      /* 数据长度 */
    rt_uint8_t *buf;      /* 数据缓冲区 */
} rt_i2c_msg_t;

/**
 * @brief I2C总线设备结构体
 */
typedef struct rt_i2c_bus_device
{
    const char *name;           /* 总线名称 */
    rt_uint32_t retries;        /* 重试次数 */
    const struct rt_i2c_config *config;  /* 总线配置 */
}
rt_i2c_bus_device_t;

/* I2C消息标志位 */
#define RT_I2C_RD              0x0001  /* 读操作 */
#define RT_I2C_NO_START        0x0002  /* 无起始条件 */
#define RT_I2C_IGNORE_NACK     0x0004  /* 忽略NACK */
#define RT_I2C_NO_READ_ACK     0x0008  /* 无读ACK */
#define RT_I2C_ADDR_10BIT      0x0010  /* 10位地址 */

/**
 * @brief 控制I2C总线
 * @param bus I2C总线设备
 * @param cmd 命令
 * @param arg 参数
 * @return 成功返回RT_EOK，失败返回错误码
 */
rt_err_t rt_i2c_control(struct rt_i2c_bus_device *bus, rt_uint32_t cmd, rt_uint32_t arg);

/**
 * @brief 传输I2C消息
 * @param bus I2C总线设备
 * @param msgs 消息数组
 * @param num 消息数量
 * @return 成功返回传输的消息数量，失败返回错误码
 */
rt_size_t rt_i2c_transfer(struct rt_i2c_bus_device *bus, struct rt_i2c_msg msgs[], rt_uint32_t num);

/**
 * @brief 主设备发送数据
 * @param bus I2C总线设备
 * @param addr 设备地址
 * @param flags 标志位
 * @param buf 数据缓冲区
 * @param count 数据长度
 * @return 成功返回发送的数据长度，失败返回错误码
 */
rt_size_t rt_i2c_master_send(struct rt_i2c_bus_device *bus, rt_uint16_t addr, rt_uint16_t flags, const rt_uint8_t *buf, rt_uint32_t count);

/**
 * @brief 主设备接收数据
 * @param bus I2C总线设备
 * @param addr 设备地址
 * @param flags 标志位
 * @param buf 数据缓冲区
 * @param count 数据长度
 * @return 成功返回接收的数据长度，失败返回错误码
 */
rt_size_t rt_i2c_master_recv(struct rt_i2c_bus_device *bus, rt_uint16_t addr, rt_uint16_t flags, rt_uint8_t *buf, rt_uint32_t count);

/**
 * @brief 查找I2C总线设备
 * @param bus_name 总线名称
 * @return 成功返回I2C总线设备指针，失败返回RT_NULL
 */
struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *bus_name);

/**
 * @brief 初始化I2C核心
 * @return 成功返回RT_EOK
 */
int rt_i2c_core_init(void);

#ifdef __cplusplus
}
#endif

#endif /* I2C_H__ */