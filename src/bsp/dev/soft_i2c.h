/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：soft_i2c.h
 * 文件描述：软件模拟I2C驱动
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/06/05 陈军  移除引脚配置到dev_cfg文件
 */

#ifndef _SOFT_I2C_H_
#define _SOFT_I2C_H_

#include "gd32_ll.h"

/* EEPROM 模块 I2C 映射 */
#define I2C_BUS_EEPROM   0
#define I2C_ADDR_EEPROM  0x50
/* SCCB 模块 I2C 映射 */
#define I2C_BUS_SCCB     1
#define I2C_ADDR_SCCB    0x3C
/* LTC2606 模块 I2C 映射 */
#define I2C_BUS_LTC2606  3
#define I2C_ADDR_LTC2606 0x11
/* OLED 模块 I2C 映射 */
#define I2C_BUS_OLED     2
#define I2C_ADDR_OLED    0x3C

/* 操作类型 */
#define SI2C_OP_WRITE    0 /* 写操作 */
#define SI2C_OP_READ     1 /* 读操作 */

void si2c_init(int bus);
int  si2c_send(int bus, uint32_t addr, const void *buff, int len);
int  si2c_receive(int bus, uint32_t addr, void *buff, int len);
void si2c_start(int bus);                         /* 发送一个START信号 */
void si2c_stop(int bus);                          /* 发送一个STOP信号 */
int  si2c_select(int bus, uint32_t addr, int op); /* 选择一个从设备 */
int  si2c_tx(int bus, const void *buff, int len); /* 发送数据 */
int  si2c_rx(int bus, void *buff, int len);       /* 接收数据 */

#endif /* _SOFT_I2C_H_ */
