/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ov5640.h
 * 文件描述：ov5640驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/11/18  张晓博、乔鹤 初始版本
 */

#ifndef _OV5640_H_
#define _OV5640_H_

#include "gd32_ll.h"
#include "soft_i2c.h"

#define SCCB_I2C_BUS   I2C_BUS_SCCB  /* I2C总线号 */
#define SCCB_I2C_ADDR  I2C_ADDR_SCCB /* I2C地址：7bit */

/* OV5640相关寄存器定义 */
#define OV5640_CHIPIDH 0X300A /* OV5640芯片ID高字节 */
#define OV5640_CHIPIDL 0X300B /* OV5640芯片ID低字节 */

#define OV5640_CHIP_ID 0X5640 /* OV5640的芯片ID */

#define ov5640_ctrl_pin_init()                                                                                                                       \
    {                                                                                                                                                \
        rcu_periph_clock_enable(RCU_GPIOD);                                                                                                          \
        gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_4 | GPIO_PIN_13);                                                            \
        gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4 | GPIO_PIN_13);                                                  \
        gpio_bit_set(GPIOD, GPIO_PIN_4);                                                                                                             \
        gpio_bit_reset(GPIOD, GPIO_PIN_13);                                                                                                          \
    }

#define ov5640_power_on()  gpio_bit_reset(GPIOD, GPIO_PIN_4)
#define ov5640_power_off() gpio_bit_set(GPIOD, GPIO_PIN_4)

#define ov5640_rst_high()  gpio_bit_set(GPIOD, GPIO_PIN_13)
#define ov5640_rst_low()   gpio_bit_reset(GPIOD, GPIO_PIN_13)

int      ov5640_init(void);
void     ov5640_jpeg_mode(void);
void     ov5640_rgb565_mode(void);
uint16_t ov5640_get_exposure(void);
void     ov5640_set_exposure(uint8_t mode);
int      ov5640_set_shutter(int shutter);
void     ov5640_exposure(uint8_t exposure);
void     ov5640_light_mode(uint8_t mode);
void     ov5640_color_saturation(uint8_t sat);
void     ov5640_brightness(uint8_t bright);
void     ov5640_contrast(uint8_t contrast);
void     ov5640_sharpness(uint8_t sharp);
void     ov5640_special_effects(uint8_t eft);
void     ov5640_test_pattern(uint8_t mode);
void     ov5640_flash_ctrl(uint8_t sw);
void     ov5640_outsize_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height);
int      ov5640_imagewin_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height);
int      ov5640_focus_init(void);
int      ov5640_focus_single(void);
int      ov5640_focus_constant(void);
uint8_t  ov5640_set_quality(uint8_t quality);

#endif /* _OV5640_H_ */
