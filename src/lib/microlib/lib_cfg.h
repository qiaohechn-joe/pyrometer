/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：lib_cfg.h
 * 文件描述：通用库配置文件
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.0 2025/03/28 陈军  取消部分编译警告
 */

#ifndef _LIB_CFG_H_
#define _LIB_CFG_H_

#include "gd32f4xx_rcu.h"
#include "gd32f4xx_gpio.h"

/* 调试级别 */                // todo 日志模块待重构，不同模块有独立的开关与调试等级。
#define DBG_LEVEL_NONE    "0" /* 不打印 */
#define DBG_LEVEL_ERR     "1" /* 错误信息 */
#define DBG_LEVEL_UPGRADE "2" /* 升级信息 */
#define DBG_LEVEL_RECORD  "3" /* 日志信息 */
#define DBG_LEVEL_NET     "4" /* 网络信息 */
#define DBG_LEVEL_INFO    "5" /* 一般信息 */
#define DBG_LEVEL_CAMERA  "6" /* 摄像头信息 */
#define DBG_LEVEL_ALL     "7" /* 打印所有 */
#define DBG_LEVEL_CNT     8

/*  输入输出接口配置 */
#define LIB_IO_DEV        USART2 /* IO设备 USART2*/
#define LIB_IO_RATE       115200 /* IO波特率 */
#define LIB_IO_BUFF_SIZE  512    /* IO缓存大小 */

/* TXD2: PD8   RXD2: PD9 */
#define io_pin_init()                                                                                                                                \
    {                                                                                                                                                \
        rcu_periph_clock_enable(RCU_GPIOD);                                                                                                          \
        gpio_af_set(GPIOD, GPIO_AF_7, GPIO_PIN_8 | GPIO_PIN_9);                                                                                      \
        gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_8 | GPIO_PIN_9);                                                               \
        gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8 | GPIO_PIN_9);                                                   \
        rcu_periph_clock_enable(RCU_GPIOB);                                                                                                          \
        gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_14 | GPIO_PIN_15);                                                         \
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_14 | GPIO_PIN_15);                                                 \
        gpio_bit_set(GPIOB, GPIO_PIN_14);                                                                                                            \
        gpio_bit_reset(GPIOB, GPIO_PIN_15);                                                                                                          \
    }

/* 屏蔽警告屏蔽模块 */
#if defined(__CC_ARM)

// #pragma diag_suppress 1296  /* 允许使用扩展常量初始化，避免结构体偏移量计算时的警告 */
// #pragma diag_suppress 177   /* 抑制“未引用的静态函数”警告，防止编译器报错 */

#elif defined(__ICCARM__)
#elif defined(__GNUC__)
#endif

/* 定义不同编译器的ALIGN宏（用于字节对齐） */
#if defined(__ICCARM__) /* 对于IAR Embedded Workbench */
#define ALIGN(n)  _Pragma("data_alignment=" #n)
#define hq_packed __attribute__((__packed__))
#elif defined(__CC_ARM) || defined(__GNUC__) /* 对于Keil MDK-ARM、GCC */
#define ALIGN(n)  __attribute__((aligned(n)))
#define hq_packed __packed
// #define hq_packed
#elif defined(__clang__) /* 对于ARM Clang */
#define ALIGN(n)  __attribute__((aligned(n)))
#define hq_packed __attribute__((__packed__))
#else
#error "Unsupported compiler"
#endif

#endif /* _LIB_CFG_H_ */
