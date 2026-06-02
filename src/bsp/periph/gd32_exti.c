/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_exti.c
 * 文件描述：EXTI-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/12/31 李兆越  初始版本
 */

#include "gd32_ll.h"
#include "gd32_exti.h"

/*
 * 功能描述：初始化一个外部中断引脚
 * 入参说明：
 *   RCU_GPIOX         --- 要启用的 GPIO 外设时钟
 *   GPIOX             --- 要配置的 GPIO 端口
 *   GPIO_PIN_X        --- 要配置的具体引脚
 *   EXTI_SOURCE_GPIOx  --- EXTI 线源 GPIO
 *   EXTI_SOURCE_PINx   --- EXTI 线源引脚
 * 返 回 值：无
 */
void exti_init_pin(rcu_periph_enum RCU_GPIOX, uint32_t GPIOX, uint32_t GPIO_PIN_X, uint8_t EXTI_SOURCE_GPIOx, uint8_t EXTI_SOURCE_PINx) {
    rcu_periph_clock_enable(RCU_GPIOX);                                    /* 启用指定的 GPIO 外设时钟 */
    gpio_mode_set(GPIOX, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_X); /* 配置引脚为输入模式，无上拉下拉 */
    syscfg_exti_line_config(EXTI_SOURCE_GPIOx, EXTI_SOURCE_PINx);          /* 连接指定的 EXTI 线到 GPIO 端口 */
}

/*
 * 功能描述：初始化外部中断的中断处理
 * 入参说明：
 *   EXTI_X           --- 要初始化的 EXTI 线
 *   EXTI_TRIG_X      --- EXTI 触发类型（上升沿、下降沿等）
 *   xxx_IRQn         --- 中断号，指定的中断向量
 *   prio             --- 中断优先级，数值越小优先级越高
 *   xxx_handler      --- 指向处理函数的指针
 * 返 回 值：无
 */
void exti_init_irq(exti_line_enum EXTI_X, exti_trig_type_enum EXTI_TRIG_X, IRQn_Type xxx_IRQn, int prio, isr_handler_t xxx_handler) {
    exti_init(EXTI_X, EXTI_INTERRUPT, EXTI_TRIG_X); /* 初始化 EXTI，设置为上升沿触发中断 */
    exti_interrupt_flag_clear(EXTI_X);              /* 清除指定 EXTI 线的中断标志 */
    gd32_setup_isr(xxx_IRQn, xxx_handler, 0);       /* 注册中断服务程序（ISR） */
    gd32_setup_irq(xxx_IRQn, prio);                 /* 配置中断优先级 */
}
