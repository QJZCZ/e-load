/* RT-Thread 最小化配置文件 */
#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* 基本配置 */
#define RT_THREAD_PRIORITY_MAX     32
#define RT_TICK_PER_SECOND        1000
#define RT_ALIGN_SIZE              4
#define RT_NAME_MAX                16
#define RT_CPUS_NR                 1
#define RT_BACKTRACE_LEVEL_MAX_NR  10

/* 核心功能 - 只保留线程管理和定时器 */
#define RT_USING_THREAD            1
#define RT_USING_TIMER             1
#define RT_USING_SEMAPHORE         0
#define RT_USING_MUTEX             0
#define RT_USING_HEAP              0
#define RT_USING_MODULE            0
#define RT_USING_SIGNALS           0
#define RT_USING_MESSAGEQUEUE      0
#define RT_USING_RTC               0
#define RT_USING_USER_MAIN         0

/* 内存管理 - 禁用动态内存 */
#define RT_USING_HEAP              0

/* 调试功能 - 禁用所有调试 */
#define RT_DEBUG                   0
#define RT_DEBUG_INIT              0
#define RT_USING_CONSOLE           0
#define RT_CONSOLEBUF_SIZE         128

/* 组件 - 禁用所有组件 */
#define RT_USING_FINSH             0
#define RT_USING_FILESYSTEM        0
#define RT_USING_NETWORK           0
#define RT_USING_DFS               0
#define RT_USING_LWIP              0
#define RT_USING_USBD              0
#define RT_USING_USBH              0
#define RT_USING_CAN               0
#define RT_USING_MQTT              0
#define RT_USING_MODBUS            0
#define RT_USING_OTA               0
#define RT_USING_AT                0

/* CPU架构 */
#define RT_USING_ARCH_CORTEX_M4

#endif /* RT_CONFIG_H__ */

