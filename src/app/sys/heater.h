/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：heater.h
 * 文件描述：加热器驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/8/6 乔鹤   初始版本
 */

#ifndef _HEATER_H
#define _HEATER_H

#include "platform.h"
#include "gd32_timer.h"

/* PTC_EN: PE5 */
#define PTC_EN_PORT     GPIOA
#define PTC_EN_PIN      GPIO_PIN_5
#define PTC_EN_GPIO_CLK RCU_GPIOA

#define ptc_en_on()     gpio_bit_set(PTC_EN_PORT, PTC_EN_PIN)
#define ptc_en_off()    gpio_bit_reset(PTC_EN_PORT, PTC_EN_PIN)

#define PWM_PORT        PTC_EN_PORT
#define PWM_PIN         PTC_EN_PIN
#define PWM_AF          GPIO_AF_1
#define PWM_PORT_RCU    PTC_EN_GPIO_CLK

#define ptc_pin_init()                                                                                                                               \
    do {                                                                                                                                             \
        rcu_periph_clock_enable(PWM_PORT_RCU);                                                                                                       \
        gpio_af_set(PWM_PORT, PWM_AF, PWM_PIN);                                                                                                      \
        gpio_mode_set(PWM_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, PWM_PIN);                                                                              \
        gpio_output_options_set(PWM_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, PWM_PIN);                                                                \
    } while (0) /* pwm配置 */

#define PWM_TIMER_RCU     RCU_TIMER1
#define PWM_TIMER         TIMER1
#define PWM_TIMER_IRQ_NUM TIMER1_IRQn
#define PWM_TIMER_CH      TIMER_CH_0
#define PWM_PERIOD        5000
#define PWM_DUTY_CYCLE    sys_status.pwm_ticks

#define PWM_IN_PROGRESS   1
#define PWM_COMPLETED     2
#define PWM_FULL_POWER    3
#define PWM_ZERO_POWER    4
#define PWM_ADJ           5

typedef struct {
    float   target_val;
    float   integral;
    float   current_val;
    float   pre_val;
    uint8_t pwm_state;
} pid_t;

typedef struct {
    float setpoint;             // 期望温度
    float internal_temperature; // 当前温度

    float  error;             // 误差
    double integral;          // 误差累积值
    float  derivative;        // 误差变化率
    float  previous_error;    // 上一次误差
    float  previous_integral; // 上一次积分
    float  output;            // 输出值
    float  set_value;         // 设置值
    float  ambient_temp;      // 环境温度
} HEATER_DATA;
void         heater_init(void);
void         heater_task(void *arg);
static float ambient_temp_pid_ctrl(void);
void         PTC_PWM_Set(float value);

#endif
