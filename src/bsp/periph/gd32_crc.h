/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_crc.h
 * 文件描述：ADC-CRC层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/01/03 李兆越  初始版本
 */

#ifndef _GD32_CRC_H_
#define _GD32_CRC_H_

#include "gd32f4xx.h"

#define MAX_PKT_LEN 1024

typedef enum { ENDIAN_LITTLE, ENDIAN_BIG } EndianMode;

#define mcu_periph_crc_enable() rcu_periph_clock_enable(RCU_CRC) /* CRC外设时钟使能 */
#define mcu_reset_crc()         crc_data_register_reset()        /* 复位CRC校验寄存器 */

unsigned long calculate_crc32(uint8_t *addr, int len, EndianMode endian);

#endif /* _GD32_CRC_H_ */
