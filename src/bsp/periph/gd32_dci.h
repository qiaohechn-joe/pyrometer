/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_dci.h
 * 文件描述：数字摄像头（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           v1.0 2025/3/6 张晓博 初始版本
 */

#ifndef _GD32_DCI_H_
#define _GD32_DCI_H_

#include "gd32f4xx.h"

/*
    DCI_PIXCLK=PA6,   DCI_VSYNC=PB7,   DCI_HSYNC=PA4,
    DCI_D0=PC6,       DCI_D1=PC7,      DCI_D2=PE0,       DCI_D3=PE1
    DCI_D4=PE4,       DCI_D5=PB6,      DCI_D6=PE5,       DCI_D7=PE6
*/

#define dci_pin_init()                                                                                                                               \
    {                                                                                                                                                \
        rcu_periph_clock_enable(RCU_GPIOA);                                                                                                          \
        rcu_periph_clock_enable(RCU_GPIOB);                                                                                                          \
        rcu_periph_clock_enable(RCU_GPIOC);                                                                                                          \
        rcu_periph_clock_enable(RCU_GPIOE);                                                                                                          \
                                                                                                                                                     \
        gpio_af_set(GPIOA, GPIO_AF_13, GPIO_PIN_4 | GPIO_PIN_6);                                                                                     \
        gpio_af_set(GPIOB, GPIO_AF_13, GPIO_PIN_6 | GPIO_PIN_7);                                                                                     \
        gpio_af_set(GPIOC, GPIO_AF_13, GPIO_PIN_6 | GPIO_PIN_7);                                                                                     \
        gpio_af_set(GPIOE, GPIO_AF_13, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6);                                              \
                                                                                                                                                     \
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4 | GPIO_PIN_6);                                                                 \
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_4 | GPIO_PIN_6);                                                     \
                                                                                                                                                     \
        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6 | GPIO_PIN_7);                                                                 \
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_6 | GPIO_PIN_7);                                                     \
                                                                                                                                                     \
        gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6 | GPIO_PIN_7);                                                                 \
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_6 | GPIO_PIN_7);                                                     \
                                                                                                                                                     \
        gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6);                          \
        gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6);              \
    }

void gd32_dci_init(void (*rx_cb)(void *arg));
void gd32_dci_start(void);
void gd32_dci_stop(void);
void gd32_dci_dma_init(uint32_t periph_addr, uint32_t mem_addr0, uint32_t mem_addr1, uint32_t num, void (*dma_callback)(void *arg));
void gd32_dci_dma_start(void);

#endif
