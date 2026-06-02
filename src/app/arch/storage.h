/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：storage.h
 * 文件描述：SPI-FLASH和EEPROM存储管理
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/05/10 陈军  增加SPI-FLASH支持
 *           V1.2 2024/05/30 陈军  去掉不必要的头文件
 *           V1.3 2024/06/05 陈军  对应存储设备头文件移入条件语句中
 */

#ifndef _STORAGE_H_
#define _STORAGE_H_

#include "platform.h"

int storage_init(void);
#if SUPPORT_EEPROM /* EEPROM */
#include "eeprom.h"

int eeprom_read(uint32_t addr, void *buff, int len);
int eeprom_write(uint32_t addr, void *buff, int len);
int eeprom_lock(void);
int eeprom_unlock(void);
int eeprom_read_lock(uint32_t addr, void *buff, int len);
int eeprom_write_lock(uint32_t addr, void *buff, int len);
int eeprom_test(void);
#endif /* SUPPORT_EEPROM */

#if SUPPORT_SPIFLASH /* SPI FLASH */
#include "w25q.h"

int spif_read(uint32_t addr, void *buff, int len);
int spif_write(uint32_t addr, const void *buff, int len);
int spif_lock(void);
int spif_unlock(void);
int spif_read_lock(uint32_t addr, void *buff, int len);
int spif_write_lock(uint32_t addr, const void *buff, int len);
int spif_test(void);
#endif /* SUPPORT_SPIFLASH */

#endif /* _STORAGE_H_ */
