/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_fmc.h
 * 文件描述：FMC-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 李兆越 初始版本
 *           V1.1 2024/11/25 李超   添加页擦除功能
 *           V1.2 2025/03/27 李兆越 优化最后一字写入bug
 *           V1.3 2025/05/06 李兆越 加入快速写入函数
 */

#ifndef _GD32_FMC_H_
#define _GD32_FMC_H_

#include <stdio.h>
#include "gd32f4xx.h"

#define FMC_PAGE_SIZE     4096 /*页大小为4K*/
#define FMC_PAGE_MAX      768
/* FLASH基地址 */
#define FLASH_SECT0_BASE  FLASH_BASE                    /* 16KB */
#define FLASH_SECT1_BASE  (FLASH_SECT0_BASE + 0x4000)   /* 16KB */
#define FLASH_SECT2_BASE  (FLASH_SECT1_BASE + 0x4000)   /* 16KB */
#define FLASH_SECT3_BASE  (FLASH_SECT2_BASE + 0x4000)   /* 16KB */
#define FLASH_SECT4_BASE  (FLASH_SECT3_BASE + 0x4000)   /* 64KB */
#define FLASH_SECT5_BASE  (FLASH_SECT4_BASE + 0x10000)  /* 128KB */
#define FLASH_SECT6_BASE  (FLASH_SECT5_BASE + 0x20000)  /* 128KB */
#define FLASH_SECT7_BASE  (FLASH_SECT6_BASE + 0x20000)  /* 128KB */
#define FLASH_SECT8_BASE  (FLASH_SECT7_BASE + 0x20000)  /* 128KB */
#define FLASH_SECT9_BASE  (FLASH_SECT8_BASE + 0x20000)  /* 128KB */
#define FLASH_SECT10_BASE (FLASH_SECT9_BASE + 0x20000)  /* 128KB */
#define FLASH_SECT11_BASE (FLASH_SECT10_BASE + 0x20000) /* 128KB */

#define FLASH_SECT12_BASE (FLASH_BASE + 0x100000)       /* 16KB */
#define FLASH_SECT13_BASE (FLASH_SECT12_BASE + 0x4000)  /* 16KB */
#define FLASH_SECT14_BASE (FLASH_SECT13_BASE + 0x4000)  /* 16KB */
#define FLASH_SECT15_BASE (FLASH_SECT14_BASE + 0x4000)  /* 16KB */
#define FLASH_SECT16_BASE (FLASH_SECT15_BASE + 0x4000)  /* 64KB */
#define FLASH_SECT17_BASE (FLASH_SECT16_BASE + 0x10000) /* 128KB */
#define FLASH_SECT18_BASE (FLASH_SECT17_BASE + 0x20000) /* 128KB */
#define FLASH_SECT19_BASE (FLASH_SECT18_BASE + 0x20000) /* 128KB */
#define FLASH_SECT20_BASE (FLASH_SECT19_BASE + 0x20000) /* 128KB */
#define FLASH_SECT21_BASE (FLASH_SECT20_BASE + 0x20000) /* 128KB */
#define FLASH_SECT22_BASE (FLASH_SECT21_BASE + 0x20000) /* 128KB */
#define FLASH_SECT23_BASE (FLASH_SECT22_BASE + 0x20000) /* 128KB */
#define FLASH_SECT24_BASE (FLASH_SECT23_BASE + 0x20000) /* 256KB */
#define FLASH_SECT25_BASE (FLASH_SECT24_BASE + 0x40000) /* 256KB */
#define FLASH_SECT26_BASE (FLASH_SECT25_BASE + 0x40000) /* 256KB */
#define FLASH_SECT27_BASE (FLASH_SECT26_BASE + 0x40000) /* 256KB */

int      gdf_erase_pages(uint32_t addr, uint32_t num);
int      gdf_erase_sector(uint32_t fmc_sector);
int      gdf_write_data_by_byte(uint32_t addr, uint32_t len, uint8_t *buff);
int      gdf_write_data_by_word(uint32_t addr, int len, void *buff);
int      gdf_write_data_by_page(uint32_t addr, int len, uint8_t *buff);
int      gdf_write_data_by_page_fast(uint32_t addr, int len, uint8_t *buff);
void     gdf_read_data(uint32_t addr, uint32_t len, uint8_t *buff);
int      flash_read(uint32_t addr, void *buff, int len);
int      flash_write(uint32_t addr, void *buff, int len);
uint32_t gdf_get_sector(uint32_t addr);

#endif /* _GD32_FMC_H_ */
