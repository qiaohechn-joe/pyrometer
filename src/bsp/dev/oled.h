/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：oled.h
 * 文件描述：oled驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/11/15  乔鹤   初始版本
 *           V1.1 2025/03/25  李兆越 适配新框架
 */

#ifndef __OLED_H
#define __OLED_H

#include "platform.h"

#define OLED_SCL_CLR() gpio_bit_reset(GPIOB, GPIO_PIN_10) // SCL
#define OLED_SCL_SET() gpio_bit_set(GPIOB, GPIO_PIN_10)

#define OLED_SDA_CLR() gpio_bit_reset(GPIOB, GPIO_PIN_3) // DIN
#define OLED_SDA_SET() gpio_bit_set(GPIOB, GPIO_PIN_3)

#define OLED_I2C_BUS   I2C_BUS_OLED  /* I2C总线0 */
#define OLED_I2C_ADDR  I2C_ADDR_OLED /* I2C地址：7-bit */

#define OLED_CMD       0 /* 写命令 */
#define OLED_DATA      1 /* 写数据 */

#define OLED_X_MID     64

extern uint8_t OLED_GRAM[128][4];

void     oled_color_turn(uint8_t i);
void     oled_display_turn(uint8_t i);
int      oled_write(uint8_t buff, uint8_t mode);
void     oled_display_on(void);
void     oled_display_off(void);
void     oled_refresh(void);
void     oled_refresh_diff(void);
void     oled_clear(void);
void     oled_draw_point(uint8_t x, uint8_t y, uint8_t t);
void     oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode);
void     oled_draw_circle(uint8_t x, uint8_t y, uint8_t r);
void     oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size1, uint8_t mode);
void     oled_show_char_6x8(uint8_t x, uint8_t y, uint8_t chr, uint8_t mode);
void     oled_show_string(uint8_t x, uint8_t y, uint8_t *chr, uint8_t size1, uint8_t mode);
void     oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1, uint8_t mode);
void     oled_show_chinese(uint8_t x, uint8_t y, uint8_t num, uint8_t size1, uint8_t mode);
void     oled_scroll_display(uint8_t num, uint8_t space, uint8_t mode);
void     oled_show_picture(uint8_t x, uint8_t y, uint8_t sizex, uint8_t sizey, uint8_t BMP[], uint8_t mode);
void     oled_init(void);
void     oled_display(void);
uint32_t oled_pow(uint8_t m, uint8_t n);

#endif
