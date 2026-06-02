/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_eth.h
 * 文件描述：网口驱动（基于GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#ifndef _GD32_ETH_H_
#define _GD32_ETH_H_

#include "gd32f4xx_rcu.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_enet.h"
#include "lib_cfg.h"

/* 以太网参数 */
#define ETH_RMII_MODE    1
#define ETH_SPEED_MODE   ENET_100M_FULLDUPLEX
#define ETH_LINK_CHK_PER 500 /* 毫秒 */

/* PHY参数 */
#define PHY_AN_TIMEO     5000 /* 自动协商超时时间（毫秒） */

/* PHY（物理层）时序参数 */

/*
   ETH_REFCLK : PA1  [IN]

   ETH_TXEN   : PB11 [OUT]
   ETH_TXD1   : PB13 [OUT]
   ETH_TXD0   : PB12 [OUT]

   ETH_CRSDV  : PA7  [IN]
   ETH_RXD1   : PC5  [IN]
   ETH_RXD0   : PC4  [IN]

   ETH_MDC    : PC1  [OUT]
   ETH_MDIO   : PA2  [IN/OUT]
*/

#define eth_init_pin()                                                                                                                               \
    do {                                                                                                                                             \
        rcu_periph_clock_enable(RCU_GPIOA);                                                                                                          \
        rcu_periph_clock_enable(RCU_GPIOB);                                                                                                          \
        rcu_periph_clock_enable(RCU_GPIOC);                                                                                                          \
        gpio_af_set(GPIOA, GPIO_AF_11, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7);                                                                        \
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_1 | GPIO_PIN_7);                                                                 \
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2);                                                                            \
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7);                                        \
                                                                                                                                                     \
        gpio_af_set(GPIOB, GPIO_AF_11, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);                                                                     \
        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);                                                 \
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);                                     \
                                                                                                                                                     \
        gpio_af_set(GPIOC, GPIO_AF_11, GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5);                                                                        \
        gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5);                                                    \
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5);                                        \
    } while (0)

int      gd32_eth_init(enet_mediamode_enum mode, void (*rx_cb)(void *arg), void (*link_cb)(void *arg));
uint8_t *gd32_eth_get_mac(void);
int      gd32_eth_setup_mode(void);
int      gd32_eth_link_up(void);

#endif /* _GD32_ETH_ */
