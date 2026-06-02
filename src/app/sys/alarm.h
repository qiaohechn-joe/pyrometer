/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：alarm.h
 * 文件描述：继电器报警驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/10/31 乔鹤   初始版本
 *           V1.1 2025/03/19 李兆越 适配新框架
 *           V1.2 2025/04/18 李兆越 继电器报警采用软定时统一管理
 *           V1.3 2025/04/28 李兆越 继电器报警死区优化
 */

#ifndef _ALARM_H
#define _ALARM_H

#include "platform.h"

#define RELAY_GPIO_PORT          GPIOE
#define RELAY_GPIO_PIN           GPIO_PIN_3

/* 宏定义报警模式 */
#define ALARM_RELAY_OFF          0 /* 关闭报警继电器 */
#define ALARM_RELAY_ON           1 /* 打开报警继电器 */
#define ALARM_NO_TARGET_INTERNAL 2 /* 常开，目标和内部 */
#define ALARM_NC_TARGET_INTERNAL 3 /* 常闭，目标和内部 */
#define ALARM_NO_INTERNAL        4 /* 常开，内部 */
#define ALARM_NC_INTERNAL        5 /* 常闭，内部 */
#define ALARM_NO_TARGET          6 /* 常开，目标 */
#define ALARM_NC_TARGET          7 /* 常闭，目标 */
#define ALARM_NO_DECAY           8 /* 常开，衰减报警 */
#define ALARM_NC_DECAY           9 /* 常闭，衰减报警 */

/*
   宽带模拟开关引脚映射:
   WAS_A0  :  GPIOE(11)
   WAS_A1  :  GPIOE(12)
   WAS_A2  :  GPIOE(13)
*/
#define alarm_pin_init()                                                                                                                             \
    {                                                                                                                                                \
        rcu_periph_clock_enable(RELAY_GPIO_PORT);                                                                                                    \
        gpio_mode_set(RELAY_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RELAY_GPIO_PIN);                                                            \
        gpio_output_options_set(RELAY_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RELAY_GPIO_PIN);                                                  \
    }

#pragma anon_unions

typedef union {
    struct {
        bool     at24cxx : 1;
        bool     nsa2862x : 1;
        bool     tm1639 : 1;
        bool     ltc2606 : 1;
        bool     ov5640 : 1;
        uint16_t rev : 12;
    };
    uint16_t states;
} BSP_STATUS; // 板载硬件工作状态，0表示工作正常，1表示工作异常

void alarm_init(void);
void alarm_task(void *arg);

#endif
