/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-08-21     RiceChen     the first version
 * 2026-03-26     zcz         create i2c.c
 */

#include <board.h>
#include "i2c.h"
#include "drv_i2c.h"
#include "pin.h"

#ifdef RT_USING_I2C

/**
 * @brief I2C总线索引枚举
 */
enum
{
#ifdef RT_USING_I2C1
    I2C1_INDEX,
#endif
#ifdef RT_USING_I2C2
    I2C2_INDEX,
#endif
};

/**
 * @brief I2C总线配置
 */
static struct rt_i2c_config i2c_config[] =
{
#ifdef RT_USING_I2C1
     RT_I2C1_CONFIG,
#endif
#ifdef RT_USING_I2C2
     RT_I2C2_CONFIG
#endif
};

/**
 * @brief I2C总线设备数组
 */
static struct rt_i2c_bus_device i2c_bus[sizeof(i2c_config) / sizeof(i2c_config[0])] = {0};

/**
 * @brief 配置I2C总线
 * @param bus I2C总线设备
 */
static void rt_i2c_configure(struct rt_i2c_bus_device *bus)
{
    rt_uint8_t scl_pin = bus->config->scl;
    rt_uint8_t sda_pin = bus->config->sda;

    rt_pin_mode(scl_pin, PIN_MODE_OUTPUT_OD);
    rt_pin_mode(sda_pin, PIN_MODE_OUTPUT_OD);

    rt_pin_write(scl_pin, PIN_HIGH);
    rt_pin_write(sda_pin, PIN_HIGH);
}

/**
 * @brief 设置SDA引脚状态
 * @param bus I2C总线设备
 * @param state 状态 (0: 低电平, 1: 高电平)
 */
static void rt_i2c_set_sda(struct rt_i2c_bus_device *bus, rt_uint32_t state)
{
    rt_uint8_t sda_pin = bus->config->sda;

    if (state)
    {
        rt_pin_write(sda_pin, PIN_HIGH);
    }
    else
    {
        rt_pin_write(sda_pin, PIN_LOW);
    }
}

/**
 * @brief 设置SCL引脚状态
 * @param bus I2C总线设备
 * @param state 状态 (0: 低电平, 1: 高电平)
 */
static void rt_i2c_set_scl(struct rt_i2c_bus_device *bus, rt_uint32_t state)
{
    rt_uint8_t scl_pin = bus->config->scl;

    if (state)
    {
        rt_pin_write(scl_pin, PIN_HIGH);
    }
    else
    {
        rt_pin_write(scl_pin, PIN_LOW);
    }
}

/**
 * @brief 获取SDA引脚状态
 * @param bus I2C总线设备
 * @return 状态 (0: 低电平, 1: 高电平)
 */
static rt_uint32_t rt_i2c_get_sda(struct rt_i2c_bus_device *bus)
{
    rt_uint8_t sda_pin = bus->config->sda;

    return rt_pin_read(sda_pin);
}

/**
 * @brief 获取SCL引脚状态
 * @param bus I2C总线设备
 * @return 状态 (0: 低电平, 1: 高电平)
 */
static rt_uint32_t rt_i2c_get_scl(struct rt_i2c_bus_device *bus)
{
    rt_uint8_t scl_pin = bus->config->scl;

    return rt_pin_read(scl_pin);
}

/**
 * @brief 微秒延迟
 * @param us 延迟时间 (微秒)
 */
static void rt_i2c_udelay(rt_uint32_t us)
{
    rt_hw_us_delay(us);
}

/* 引脚操作宏定义 */
#define SET_SDA(bus, val)   rt_i2c_set_sda(bus, val)
#define SET_SCL(bus, val)   rt_i2c_set_scl(bus, val)
#define GET_SDA(bus)        rt_i2c_get_sda(bus)
#define GET_SCL(bus)        rt_i2c_get_scl(bus)

#define SDA_L(bus)          SET_SDA(bus, 0)
#define SDA_H(bus)          SET_SDA(bus, 1)
#define SCL_L(bus)          SET_SCL(bus, 0)

/**
 * @brief 设置SCL为高电平并等待
 * @param bus I2C总线设备
 * @return 成功返回RT_EOK，失败返回错误码
 */
static rt_err_t SCL_H(struct rt_i2c_bus_device *bus)
{
    rt_tick_t start;

    SET_SCL(bus, 1);

    if(rt_i2c_get_scl(bus))
    {
        goto done;
    }

    start = rt_tick_get();
    while (!GET_SCL(bus))
    {
        if ((rt_tick_get() - start) > 100)
            return -RT_ETIMEOUT;
        rt_thread_delay(100);
    }
done:
    rt_i2c_udelay(1);

    return RT_EOK;
}

/**
 * @brief 发送I2C起始信号
 * @param bus I2C总线设备
 */
static void rt_i2c_start(struct rt_i2c_bus_device *bus)
{
    SDA_L(bus);
    rt_i2c_udelay(1);
    SCL_L(bus);
}

/**
 * @brief 发送I2C重启信号
 * @param bus I2C总线设备
 */
static void rt_i2c_restart(struct rt_i2c_bus_device *bus)
{
    SDA_H(bus);
    SCL_H(bus);
    rt_i2c_udelay(1);

    SDA_L(bus);
    rt_i2c_udelay(1);
    SCL_L(bus);
}

/**
 * @brief 发送I2C停止信号
 * @param bus I2C总线设备
 */
static void rt_i2c_stop(struct rt_i2c_bus_device *bus)
{
    SDA_L(bus);
    rt_i2c_udelay(1);
    SCL_H(bus);
    rt_i2c_udelay(1);
    SDA_H(bus);
    rt_i2c_udelay(1);
}

/**
 * @brief 等待ACK信号
 * @param bus I2C总线设备
 * @return 成功返回RT_EOK，失败返回错误码
 */
rt_inline rt_bool_t rt_i2c_waitack(struct rt_i2c_bus_device *bus)
{
    rt_bool_t ack;

    SDA_H(bus);
    rt_i2c_udelay(1);

    if (SCL_H(bus) < 0)
    {
        return -RT_ETIMEOUT;
    }
    ack = !GET_SDA(bus);
    SCL_L(bus);

    return ack;
}

/**
 * @brief 写入一个字节
 * @param bus I2C总线设备
 * @param data 要写入的数据
 * @return 成功返回1，失败返回错误码
 */
static rt_int32_t rt_i2c_writeb(struct rt_i2c_bus_device *bus, rt_uint8_t data)
{
    rt_int32_t i;
    rt_uint8_t bit;

    for (i = 7; i >= 0; i--)
    {
        SCL_L(bus);
        bit = (data >> i) & 1;
        SET_SDA(bus, bit);
        rt_i2c_udelay(1);
        if (SCL_H(bus) < 0)
        {
            return -RT_ETIMEOUT;
        }
    }
    SCL_L(bus);
    rt_i2c_udelay(1);

    return rt_i2c_waitack(bus);
}

/**
 * @brief 读取一个字节
 * @param bus I2C总线设备
 * @return 成功返回读取的数据，失败返回错误码
 */
static rt_int32_t rt_i2c_readb(struct rt_i2c_bus_device *bus)
{
    rt_uint8_t i;
    rt_uint8_t data = 0;

    SDA_H(bus);
    rt_i2c_udelay(1);
    for (i = 0; i < 8; i++)
    {
        data <<= 1;
        if (SCL_H(bus) < 0)
        {
            return -RT_ETIMEOUT;
        }
        if (GET_SDA(bus))
            data |= 1;
        SCL_L(bus);
        rt_i2c_udelay(1);
    }

    return data;
}

/**
 * @brief 发送字节数据
 * @param bus I2C总线设备
 * @param msg I2C消息
 * @return 成功返回发送的字节数，失败返回错误码
 */
static rt_size_t rt_i2c_send_bytes(struct rt_i2c_bus_device *bus, struct rt_i2c_msg *msg)
{
    rt_int32_t ret;
    rt_size_t bytes = 0;
    const rt_uint8_t *ptr = msg->buf;
    rt_int32_t count = msg->len;
    rt_uint16_t ignore_nack = msg->flags & RT_I2C_IGNORE_NACK;

    while (count > 0)
    {
        ret = rt_i2c_writeb(bus, *ptr);

        if ((ret > 0) || (ignore_nack && (ret == 0)))
        {
            count --;
            ptr ++;
            bytes ++;
        }
        else if (ret == 0)
        {
            return 0;
        }
        else
        {
            return ret;
        }
    }

    return bytes;
}

/**
 * @brief 发送ACK或NACK信号
 * @param bus I2C总线设备
 * @param ack ACK标志 (1: ACK, 0: NACK)
 * @return 成功返回RT_EOK，失败返回错误码
 */
static rt_err_t rt_i2c_send_ack_or_nack(struct rt_i2c_bus_device *bus, int ack)
{
    if (ack)
        SET_SDA(bus, 0);
    rt_i2c_udelay(1);
    if (SCL_H(bus) < 0)
    {
        return -RT_ETIMEOUT;
    }
    SCL_L(bus);

    return RT_EOK;
}

/**
 * @brief 接收字节数据
 * @param bus I2C总线设备
 * @param msg I2C消息
 * @return 成功返回接收的字节数，失败返回错误码
 */
static rt_size_t rt_i2c_recv_bytes(struct rt_i2c_bus_device *bus, struct rt_i2c_msg *msg)
{
    rt_int32_t val;
    rt_int32_t bytes = 0;   /* actual bytes */
    rt_uint8_t *ptr = msg->buf;
    rt_int32_t count = msg->len;
    const rt_uint32_t flags = msg->flags;

    while (count > 0)
    {
        val = rt_i2c_readb(bus);
        if (val >= 0)
        {
            *ptr = val;
            bytes ++;
        }
        else
        {
            break;
        }

        ptr ++;
        count --;
        if (!(flags & RT_I2C_NO_READ_ACK))
        {
            val = rt_i2c_send_ack_or_nack(bus, count);
            if (val < 0)
                return val;
        }
    }

    return bytes;
}

/**
 * @brief 发送设备地址
 * @param bus I2C总线设备
 * @param addr 设备地址
 * @param retries 重试次数
 * @return 成功返回1，失败返回0或错误码
 */
static rt_int32_t rt_i2c_send_address(struct rt_i2c_bus_device *bus, rt_uint8_t addr, rt_int32_t retries)
{
    rt_int32_t i;
    rt_err_t ret = 0;

    for (i = 0; i <= retries; i++)
    {
        ret = rt_i2c_writeb(bus, addr);
        if (ret == 1 || i == retries)
            break;
        rt_i2c_stop(bus);
        rt_i2c_udelay(1);
        rt_i2c_start(bus);
    }

    return ret;
}

/**
 * @brief 发送位地址
 * @param bus I2C总线设备
 * @param msg I2C消息
 * @return 成功返回RT_EOK，失败返回错误码
 */
static rt_err_t rt_i2c_bit_send_address(struct rt_i2c_bus_device *bus, struct rt_i2c_msg *msg)
{
    rt_uint16_t flags = msg->flags;
    rt_uint16_t ignore_nack = msg->flags & RT_I2C_IGNORE_NACK;

    rt_uint8_t addr1, addr2;
    rt_int32_t retries;
    rt_err_t ret;

    retries = ignore_nack ? 0 : bus->retries;

    if (flags & RT_I2C_ADDR_10BIT)
    {
        addr1 = 0xf0 | ((msg->addr >> 7) & 0x06);
        addr2 = msg->addr & 0xff;

        ret = rt_i2c_send_address(bus, addr1, retries);
        if ((ret != 1) && !ignore_nack)
        {
            return -RT_EIO;
        }

        ret = rt_i2c_writeb(bus, addr2);
        if ((ret != 1) && !ignore_nack)
        {
            return -RT_EIO;
        }
        if (flags & RT_I2C_RD)
        {
            rt_i2c_restart(bus);
            addr1 |= 0x01;
            ret = rt_i2c_send_address(bus, addr1, retries);
            if ((ret != 1) && !ignore_nack)
            {
                return -RT_EIO;
            }
        }
    }
    else
    {
        addr1 = msg->addr << 1;
        if (flags & RT_I2C_RD)
            addr1 |= 1;
        ret = rt_i2c_send_address(bus, addr1, retries);
        if ((ret != 1) && !ignore_nack)
            return -RT_EIO;
    }

    return RT_EOK;
}

/**
 * @brief 控制I2C总线
 * @param bus I2C总线设备
 * @param cmd 命令
 * @param arg 参数
 * @return 成功返回RT_EOK，失败返回错误码
 */
rt_err_t rt_i2c_control(struct rt_i2c_bus_device *bus, rt_uint32_t cmd, rt_uint32_t arg)
{
    return RT_EOK;
}

/**
 * @brief 传输I2C消息
 * @param bus I2C总线设备
 * @param msgs 消息数组
 * @param num 消息数量
 * @return 成功返回传输的消息数量，失败返回错误码
 */
rt_size_t rt_i2c_transfer(struct rt_i2c_bus_device *bus, struct rt_i2c_msg msgs[], rt_uint32_t num)
{
    struct rt_i2c_msg *msg;
    rt_int32_t i, ret;
    rt_uint16_t ignore_nack;

    rt_i2c_start(bus);
    for (i = 0; i < num; i++)
    {
        msg = &msgs[i];
        ignore_nack = msg->flags & RT_I2C_IGNORE_NACK;
        if (!(msg->flags & RT_I2C_NO_START))
        {
            if (i)
            {
                rt_i2c_restart(bus);
            }
            ret = rt_i2c_bit_send_address(bus, msg);
            if ((ret != RT_EOK) && !ignore_nack)
            {
                goto out;
            }
        }
        if (msg->flags & RT_I2C_RD)
        {
            ret = rt_i2c_recv_bytes(bus, msg);
            if (ret >= 1)
                ;
            if (ret < msg->len)
            {
                if (ret >= 0)
                    ret = -RT_EIO;
                goto out;
            }
        }
        else
        {
            ret = rt_i2c_send_bytes(bus, msg);
            if (ret >= 1)
                ;
            if (ret < msg->len)
            {
                if (ret >= 0)
                    ret = -RT_ERROR;
                goto out;
            }
        }
    }
    ret = i;

out:
    rt_i2c_stop(bus);

    return ret;
}

/**
 * @brief 主设备发送数据
 * @param bus I2C总线设备
 * @param addr 设备地址
 * @param flags 标志位
 * @param buf 数据缓冲区
 * @param count 数据长度
 * @return 成功返回发送的数据长度，失败返回错误码
 */
rt_size_t rt_i2c_master_send(struct rt_i2c_bus_device *bus, rt_uint16_t addr, rt_uint16_t flags, const rt_uint8_t *buf, rt_uint32_t count)
{
    rt_err_t ret;
    struct rt_i2c_msg msg;

    msg.addr  = addr;
    msg.flags = flags;
    msg.len   = count;
    msg.buf   = (rt_uint8_t *)buf;

    ret = rt_i2c_transfer(bus, &msg, 1);

    return (ret > 0) ? count : ret;
}

/**
 * @brief 主设备接收数据
 * @param bus I2C总线设备
 * @param addr 设备地址
 * @param flags 标志位
 * @param buf 数据缓冲区
 * @param count 数据长度
 * @return 成功返回接收的数据长度，失败返回错误码
 */
rt_size_t rt_i2c_master_recv(struct rt_i2c_bus_device *bus, rt_uint16_t addr, rt_uint16_t flags, rt_uint8_t *buf, rt_uint32_t count)
{
    rt_err_t ret;
    struct rt_i2c_msg msg;
    RT_ASSERT(bus != RT_NULL);

    msg.addr   = addr;
    msg.flags  = flags | RT_I2C_RD;
    msg.len    = count;
    msg.buf    = buf;

    ret = rt_i2c_transfer(bus, &msg, 1);

    return (ret > 0) ? count : ret;
}

/**
 * @brief 查找I2C总线设备
 * @param bus_name 总线名称
 * @return 成功返回I2C总线设备指针，失败返回RT_NULL
 */
struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *bus_name)
{
    rt_size_t bus_num = sizeof(i2c_bus) / sizeof(i2c_bus[0]);

    for(int i = 0; i < bus_num; i++)
    {
        if(rt_strncmp(i2c_bus[i].config->name, bus_name, RT_NAME_MAX) == 0)
        {
            return &i2c_bus[i];
        }
    }
    return RT_NULL;
}

/**
 * @brief 初始化I2C核心
 * @return 成功返回RT_EOK
 */
int rt_i2c_core_init(void)
{
    rt_size_t bus_num = sizeof(i2c_bus) / sizeof(i2c_bus[0]);

    for(int i = 0; i < bus_num; i++)
    {
        i2c_bus[i].config = &i2c_config[i];
        i2c_bus[i].retries = 3; /* 默认重试3次 */
        rt_i2c_configure(&i2c_bus[i]);
    }

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_i2c_core_init);

#endif /* RT_USING_I2C */
