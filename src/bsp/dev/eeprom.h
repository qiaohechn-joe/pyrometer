/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：eeprom.h
 * 文件描述：I2C接口EEPROM驱动
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军    初始版本
 *           V1.1 2024/06/05 陈军    移除EEPROM型号选择到dev_cfg文件
 *           V1.2 2024/06/12 张晓博  增加对AT24C02的支持
 *           V1.3 2025/03/10 陈军    统一为EEPROM，增加写保护（WP）逻辑
 */

#ifndef _EEPROM_H_
#define _EEPROM_H_

#include "gd32_ll.h"
#include "soft_i2c.h"

#define EEPROM_USED_WP  1 /* 1：使用写保护引脚，0：未使用 */

/* EEPROM类型 */
#define AT24C64         0
#define AT24C512        1
#define AT24C1024       2
#define AT24CM01        3
#define AT24C_CHIP_TYPE AT24CM01

#if AT24C_CHIP_TYPE == AT24CM01
#define AT24C_ADDR_LEN        2
#define AT24C_PAGE_SIZE       256
#define AT24C_CHIP_SIZE_SHIFT 17
#elif AT24C_CHIP_TYPE == AT24C64
#define AT24C_ADDR_LEN        2
#define AT24C_PAGE_SIZE       32
#define AT24C_CHIP_SIZE_SHIFT 13
#elif AT24C_CHIP_TYPE == AT24C512
#define AT24C_ADDR_LEN        2
#define AT24C_PAGE_SIZE       128
#define AT24C_CHIP_SIZE_SHIFT 16
#elif AT24C_CHIP_TYPE == AT24C1024
#define AT24C_ADDR_LEN        2
#define AT24C_PAGE_SIZE       256
#define AT24C_CHIP_SIZE_SHIFT 17
#endif

/* 写页延时（ms） */
#define AT24C_WR_DELAY 6 /* 必须大于5 */

#define eeprom_wp_init()                                                                                                                             \
    {                                                                                                                                                \
        rcu_periph_clock_enable(RCU_GPIOA);                                                                                                          \
        gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_9);                                                                          \
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_9);                                                                 \
    }

#define eeprom_wp_lock()   gpio_bit_set(GPIOA, GPIO_PIN_9)
#define eeprom_wp_unlock() gpio_bit_reset(GPIOA, GPIO_PIN_9)

int at24c_read(int bus, uint32_t dev_addr, uint32_t mem_addr, void *buff, int len);
int at24c_write(int bus, uint32_t dev_addr, uint32_t mem_addr, const void *buff, int len);

#endif /* _EEPROM_H_ */
