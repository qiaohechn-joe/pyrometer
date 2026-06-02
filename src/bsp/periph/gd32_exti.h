/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_exti.h
 * 文件描述：EXTI-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/12/31 李兆越  初始版本
 */

#ifndef _GD32_EXTI_H_
#define _GD32_EXTI_H_

#include "gd32f4xx.h"
#include "gd32_ll.h"

/* 中断管脚配置 */
void exti_init_pin(rcu_periph_enum RCU_GPIOX, uint32_t GPIOX, uint32_t GPIO_PIN_X, uint8_t EXTI_SOURCE_GPIOx, uint8_t EXTI_SOURCE_PINx);

void exti_init_irq(exti_line_enum EXTI_X, exti_trig_type_enum EXTI_TRIG_X, IRQn_Type xxx_IRQn, int prio, isr_handler_t xxx_handler);

#endif /* _GD32_EXTI_H_ */
