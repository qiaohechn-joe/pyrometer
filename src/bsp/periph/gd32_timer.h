/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_timer.h
 * 文件描述：TIMER-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/12/05 陈军  增加超时定时器功能
 *           V1.2 2025/02/17 李超  超时定时器时基修正
 */

#ifndef _GD32_TIMER_H_
#define _GD32_TIMER_H_

#include "gd32f4xx.h"
#include "gd32_ll.h"

void     gd32_timer_init(uint32_t dev, int prescale, int period);
void     gd32_timer_shutdown(uint32_t dev);
uint32_t gd32_timer_clk_freq(uint32_t dev);
void     timer_timeo_init(uint32_t timer, uint32_t timeo, isr_handler_t handler);
void     pwm_timer_init(rcu_periph_enum timer_rcu, uint32_t timer, uint16_t period, uint16_t channel, uint32_t pulse);
void     timer_set_dutyCycle(uint32_t timer_periph, uint16_t channel, int duty);

#endif /* _GD32_TIMER_H_ */
