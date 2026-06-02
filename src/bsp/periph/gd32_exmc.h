/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_exmc.c
 * 文件描述：外部存储控制器（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           v1.0 2024/07/2  张晓博 初始版本
 *           v1.1 2024/11/18 张晓博 增加psram
 */

#ifndef _GD32_EXMC_H_
#define _GD32_EXMC_H_

#include <stdio.h>
#include "gd32f4xx.h"

int sdram_init(void);

void sqpipsram_init(void);
void sqpipsram_write_set(uint32_t write_cmd_mode, uint32_t write_wait_cycle, uint32_t write_cmd_code, uint8_t is_special_cmd);
void sqpipsram_read_set(uint32_t read_cmd_mode, uint32_t read_wait_cycle, uint32_t read_cmd_code, uint8_t is_special_cmd);

#endif
