/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：tiag_cfg.h
 * 文件描述：跨阻放大器增益档位配置代码（基于ADG633）
 * 作    者：和其光电嵌软团队
 * 备    注：
 *           WT：宽带跨阻（Wideband Transimpedance）
 *           NT：窄带跨阻（Narrowband Transimpedance）
 *           WAS:宽带模拟开关（Wideband Analog Switch）
 *           NAS:窄带模拟开关（Narrowband Analog Switch）
 * 更新记录：
 *           V1.0 2025/03/19 陈军  初始版本
 */

#ifndef _TIAG_CFG_H_
#define _TIAG_CFG_H_

#include "gd32_ll.h"

#define AS_PIN_HIGH        1
#define AS_PIN_LOW         0

#define as_set(pin, state) (state ? gpio_bit_set(GPIOE, pin) : gpio_bit_reset(GPIOE, pin))

/*
   宽带模拟开关引脚映射:
   WAS_A0  :  GPIOE(11)
   WAS_A1  :  GPIOE(12)
   WAS_A2  :  GPIOE(13)
*/
#define was_pin_init()                                                                                                                               \
    {                                                                                                                                                \
        rcu_periph_clock_enable(RCU_GPIOE);                                                                                                          \
        gpio_mode_set(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);                                             \
        gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);                                   \
    }

#define was_a0(state) as_set(GPIO_PIN_11, state)
#define was_a1(state) as_set(GPIO_PIN_12, state)
#define was_a2(state) as_set(GPIO_PIN_13, state)
#define was_all_off()                                                                                                                                \
    {                                                                                                                                                \
        as_set(GPIO_PIN_11, AS_PIN_HIGH);                                                                                                            \
        as_set(GPIO_PIN_12, AS_PIN_HIGH);                                                                                                            \
        as_set(GPIO_PIN_13, AS_PIN_HIGH);                                                                                                            \
    }

/* 宽带跨阻：
 * gain_wb_high()：  H档，小跨阻，高温段
 * gain_wb_middle()：M档，中跨阻，中温段
 * gain_wb_low()：   L档，大跨阻，低温段
 */
#define gain_wb_high()                                                                                                                               \
    {                                                                                                                                                \
        /*was_all_off();*/                                                                                                                           \
        was_a0(AS_PIN_LOW);                                                                                                                          \
        was_a1(AS_PIN_HIGH);                                                                                                                         \
        was_a2(AS_PIN_HIGH);                                                                                                                         \
    }
#define gain_wb_middle()                                                                                                                             \
    {                                                                                                                                                \
        /*was_all_off();*/                                                                                                                           \
        was_a0(AS_PIN_HIGH);                                                                                                                         \
        was_a1(AS_PIN_LOW);                                                                                                                          \
        was_a2(AS_PIN_HIGH);                                                                                                                         \
    }
#define gain_wb_low()                                                                                                                                \
    {                                                                                                                                                \
        /*was_all_off();*/                                                                                                                           \
        was_a0(AS_PIN_HIGH);                                                                                                                         \
        was_a1(AS_PIN_HIGH);                                                                                                                         \
        was_a2(AS_PIN_LOW);                                                                                                                          \
    }

/*
   窄带模拟开关引脚映射:
   NAS_A0  :  GPIOE(8)
   NAS_A1  :  GPIOE(9)
   NAS_A2  :  GPIOE(10)
*/
#define nas_pin_init()                                                                                                                               \
    {                                                                                                                                                \
        rcu_periph_clock_enable(RCU_GPIOE);                                                                                                          \
        gpio_mode_set(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10);                                               \
        gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10);                                     \
    }
#define nas_a0(state) as_set(GPIO_PIN_8, state)
#define nas_a1(state) as_set(GPIO_PIN_9, state)
#define nas_a2(state) as_set(GPIO_PIN_10, state)
#define nas_all_off()                                                                                                                                \
    {                                                                                                                                                \
        as_set(GPIO_PIN_8, AS_PIN_HIGH);                                                                                                             \
        as_set(GPIO_PIN_9, AS_PIN_HIGH);                                                                                                             \
        as_set(GPIO_PIN_10, AS_PIN_HIGH);                                                                                                            \
    }

/* 窄带跨阻：
 * gain_nb_high()：  H档，小跨阻，高温段
 * gain_nb_middle()：M档，中跨阻，中温段
 * gain_nb_low()：   L档，大跨阻，低温段
 */
#define gain_nb_high()                                                                                                                               \
    {                                                                                                                                                \
        /*nas_all_off();*/                                                                                                                           \
        nas_a0(AS_PIN_HIGH);                                                                                                                         \
        nas_a1(AS_PIN_HIGH);                                                                                                                         \
        nas_a2(AS_PIN_LOW);                                                                                                                          \
    }
#define gain_nb_middle()                                                                                                                             \
    {                                                                                                                                                \
        /*nas_all_off();*/                                                                                                                           \
        nas_a0(AS_PIN_HIGH);                                                                                                                         \
        nas_a1(AS_PIN_LOW);                                                                                                                          \
        nas_a2(AS_PIN_HIGH);                                                                                                                         \
    }
#define gain_nb_low()                                                                                                                                \
    {                                                                                                                                                \
        /*nas_all_off();*/                                                                                                                           \
        nas_a0(AS_PIN_LOW);                                                                                                                          \
        nas_a1(AS_PIN_HIGH);                                                                                                                         \
        nas_a2(AS_PIN_HIGH);                                                                                                                         \
    }

#endif /* _TIAG_CFG_H_ */
