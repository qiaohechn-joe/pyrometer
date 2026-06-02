/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：vti7064x.h
 * 文件描述：vti7064x(psram)驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：64Mbit
 * 更新记录：
 *           V1.0 2024/11/18  张晓博 初始版本
 */

#ifndef _VTI7064X_H_
#define _VTI7064X_H_

#include "platform.h"

#define PSRAM_BASE_ADDRESS 0x60000000 /* Bank0 Region0起始地址 */

#define VTI7064_START_ADDR PSRAM_BASE_ADDRESS
#define VTI7064_TOTAL_SIZE 8 * 1024 * 1024 /* 64Mbit */

/*
   EXMC_NE0=PD7   EXMC_CLK=PD3
   EXMC_D0=PD14   EXMC_D1=PD15  EXMC_D2=PD0  EXMC_D3=PD1
*/
#define vti7064x_pin_init()                                                                                                                          \
    {                                                                                                                                                \
        rcu_periph_clock_enable(RCU_GPIOD);                                                                                                          \
        gpio_af_set(GPIOD, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_7 | GPIO_PIN_14 | GPIO_PIN_15);                               \
        gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_7 | GPIO_PIN_14 | GPIO_PIN_15);         \
        gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_MAX,                                                                               \
                                GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_7 | GPIO_PIN_14 | GPIO_PIN_15);                                      \
    }

int vti7064x_init(void);
int vti7064x_test(void);

#endif
