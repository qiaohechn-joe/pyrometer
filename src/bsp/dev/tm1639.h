/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：tm1639.h
 * 文件描述：tm1639驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/24  乔鹤    初始版本
 *           V1.1 2024/11/15  张晓博  适配新框架
 */

#ifndef _TM1639_H_
#define _TM1639_H_

#include "gd32_ll.h"

/** TM1639 指令分类 (B7,B6) */
#define TM1639_CMD_DATA    0x40 /* 01xxxxxx: 数据命令设置 */
#define TM1639_CMD_DISPLAY 0x80 /* 10xxxxxx: 显示控制命令 */
#define TM1639_CMD_ADDR    0xC0 /* 11xxxxxx: 地址命令设置 */

/** 数据命令子选项 (与 TM1639_CMD_DATA 组合) */
#define TM1639_DATA_WR     0x00 /* 写数据到寄存器（默认） */
#define TM1639_DATA_RD     0x02 /* 读键扫描数据 */
#define TM1639_DATA_INC    0x00 /* 自动地址递增（默认） */
#define TM1639_DATA_FIX    0x04 /* 固定地址 */

/** 显示控制子选项 (与 TM1639_CMD_DISPLAY 组合) */
#define TM1639_DISP_OFF    0x00 /* 关闭显示 （默认）*/
#define TM1639_DISP_ON     0x08 /* 开启显示 */
#define TM1639_PWM(x)      (x)  /* 亮度设置 (0-7: 1/16~14/16) */

/* 引脚配置 */
#define TM1639_CLK_PIN     GPIO_PIN_11
#define TM1639_STB_PIN     GPIO_PIN_10
#define TM1639_DIO_PIN     GPIO_PIN_12

/* 引脚初始化 */
#define tm1639_pin_init()  rcu_periph_clock_enable(RCU_GPIOD) /* 启用 GPIOD 时钟 */

/* 时钟引脚操作 */
#define tm1639_clk_out()                                                                                                                             \
    {                                                                                                                                                \
        gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, TM1639_CLK_PIN);                                                                      \
        gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, TM1639_CLK_PIN);                                                            \
    }
#define tm1639_clk_high() gpio_bit_set(GPIOD, TM1639_CLK_PIN)
#define tm1639_clk_low()  gpio_bit_reset(GPIOD, TM1639_CLK_PIN)

/* STB 引脚操作 */
#define tm1639_stb_out()                                                                                                                             \
    {                                                                                                                                                \
        gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, TM1639_STB_PIN);                                                                      \
        gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, TM1639_STB_PIN);                                                            \
    }
#define tm1639_stb_high() gpio_bit_set(GPIOD, TM1639_STB_PIN)
#define tm1639_stb_low()  gpio_bit_reset(GPIOD, TM1639_STB_PIN)

/* DIO 引脚操作（输出和输入） */
#define tm1639_dio_out()                                                                                                                             \
    {                                                                                                                                                \
        gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, TM1639_DIO_PIN);                                                                      \
        gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, TM1639_DIO_PIN);                                                            \
    }
#define tm1639_dio_high()  gpio_bit_set(GPIOD, TM1639_DIO_PIN)
#define tm1639_dio_low()   gpio_bit_reset(GPIOD, TM1639_DIO_PIN)

#define tm1639_dio_in()    gpio_mode_set(GPIOD, GPIO_MODE_INPUT, GPIO_PUPD_NONE, TM1639_DIO_PIN)
#define tm1639_dio_get()   gpio_input_bit_get(GPIOD, TM1639_DIO_PIN)

/* 延时函数 */
#define tm1639_delay_us(n) gd32_delay_us(n)

extern const uint8_t numchar[];
extern const uint8_t specialchar[];

void     tm1639_init(void);
void     tm1639_write_buff(uint8_t addr, uint8_t *buff, uint8_t len);
void     tm1639_set_command(uint8_t status, uint8_t luminance);
uint16_t tm1639_key_scan(void);

#endif
