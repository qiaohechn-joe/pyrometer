/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ltc2606.h
 * 文件描述：LTC2606（16Bits I2C接口DAC）驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军 初始版本
 */

#ifndef _LTC2606_H_
#define _LTC2606_H_

#include "gd32_ll.h"
#include "soft_i2c.h"

#define LTC2606_I2C_BUS              I2C_BUS_LTC2606  /* I2C总线号 */
#define LTC2606_I2C_ADDR             I2C_ADDR_LTC2606 /* I2C地址：7bit */

#define CURRENT_OUTPUT_COEF          0.2f
#define VOLT_OUTPUT_COEF             0.5f

/* LTC2606指令 */
#define LTC2606_WRITE_COMMAND        0x00 /* 写入内部寄存器，不更新输出电压 */
#define LTC2606_UPDATE_COMMAND       0x10 /* 更新输出电压，值存储在内部寄存器中 */
#define LTC2606_WRITE_UPDATE_COMMAND 0x30 /* 写入并更新，输出电压将立即改变 */
#define LTC2606_POWER_DOWN_COMMAND   0x40 /* 关闭LTC2606电源 */

#define volt_to_dac_code(volt)       (uint16_t)(volt * 65535.0f / 5000000.0f)

int ltc2606_init(int bus);
int ltc2606_write_volt(float current);

#endif /* _LTC2606_H_ */
