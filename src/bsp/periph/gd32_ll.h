/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_ll.h
 * 文件描述：LL-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/06/06 陈军  向量表偏移设置函数nvic_vector_table_set()移除
 */

#ifndef _GD32_LL_H_
#define _GD32_LL_H_

#include "gd32f4xx.h"

/* 延迟定时器滴答数 */
#define LL_DLY_TIMER_TICKS 500

/* UID */
#define GD32_UID_BASE      (uint32_t)0x1FFF7A10

typedef void (*isr_handler_t)(void);

/* 初始化 */
void gd32_ll_init(void);

/* IRQ */
void gd32_setup_irq(IRQn_Type nr, int prio);
void gd32_setup_isr(IRQn_Type nr, isr_handler_t handler, int order);
#define gd32_enable_irq(nr)  NVIC_EnableIRQ(nr)
#define gd32_disable_irq(nr) NVIC_DisableIRQ(nr)

/* 延时 */
void gd32_delay_us(uint32_t us);
#define gd32_delay_ms(ms) gd32_delay_us(ms * 1000)

#endif /* _GD32_LL_H_ */
