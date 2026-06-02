/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：platform.h
 * 文件描述：工程管理文件（BSP, OS, Library等等）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include "gd32f4xx.h"

/* C标准库头文件相关 */
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

/* periph相关 */
#include "gd32_ll.h"
#include "gd32_dma.h"
#include "gd32_rtc.h"
#include "gd32_timer.h"
#include "gd32_uart.h"
#include "gd32_spi.h"
#include "gd32_adc.h"
#include "gd32_fmc.h"
#include "gd32_crc.h"
#include "gd32_eth.h"
#include "gd32_exmc.h"
#include "gd32_dci.h"
#include "gd32_exti.h"
#include "gd32_crc.h"

/* FreeRTOS相关 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "queue.h"

/* microlib相关 */
#include "lib_cfg.h"
#include "hq_generic.h"
#include "hq_string.h"
#include "hq_io.h"
#include "hq_cdllist.h"
#include "hq_filter.h"
#include "hq_hash.h"

/* 配置头文件相关 */
#include "app_cfg.h"
#include "arch_cfg.h"
#include "para_cfg.h"
#include "sys_cfg.h"
#include "record_cfg.h"
#include "proto_base_cfg.h"
#include "proto_char_cfg.h"
#include "comm_cfg.h"

/* 调试信息打印 */
#define dbg_error(fmt, ...)                     dbg_printf(DBG_LEVEL_ERR, fmt, ##__VA_ARGS__)
#define dbg_dump_error(text, buff, len, line)   dbg_print_buffer(DBG_LEVEL_ERR, text, buff, len, line)

#define dbg_upgrade(fmt, ...)                   dbg_printf(DBG_LEVEL_UPGRADE, fmt, ##__VA_ARGS__)
#define dbg_dump_upgrade(text, buff, len, line) dbg_print_buffer(DBG_LEVEL_UPGRADE, text, buff, len, line)

#define dbg_record(fmt, ...)                    dbg_printf(DBG_LEVEL_RECORD, fmt, ##__VA_ARGS__)
#define dbg_dump_record(text, buff, len, line)  dbg_print_buffer(DBG_LEVEL_RECORD, text, buff, len, line)

#define dbg_net(fmt, ...)                       dbg_printf(DBG_LEVEL_NET, fmt, ##__VA_ARGS__)
#define dbg_dump_net(text, buff, len, line)     dbg_print_buffer(DBG_LEVEL_NET, text, buff, len, line)

#define dbg_info(fmt, ...)                      dbg_printf(DBG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define dbg_dump_info(text, buff, len, line)    dbg_print_buffer(DBG_LEVEL_INFO, text, buff, len, line)

#define dbg_camera(fmt, ...)                    dbg_printf(DBG_LEVEL_CAMERA, fmt, ##__VA_ARGS__)
#define dbg_dump_camera(text, buff, len, line)  dbg_print_buffer(DBG_LEVEL_CAMERA, text, buff, len, line)

#define dbg_dump(text, buff, len, line)         print_buffer(DBG_LEVEL_NONE, text, buff, len, line)

/* 选择是否启用操作系统 */
#define PLATFORM_USE_OS                         1 /* 1：使用操作系统，0：使用裸机 */
#if PLATFORM_USE_OS == 0                          /* 使用裸机 */
#define DEFINE_CPU_SR    uint32_t primask
#define ENTER_CRITICAL() __disable_irq()
#define EXIT_CRITICAL()  __enable_irq()
#define ENTER_ISR()                                                                                                                                  \
    do {                                                                                                                                             \
        primask = __get_PRIMASK();                                                                                                                   \
        __disable_irq();                                                                                                                             \
    } while (0)
#define EXIT_ISR() __set_PRIMASK(primask)
#else /* 使用操作系统（基于FreeRTOS） */
#define DEFINE_CPU_SR    UBaseType_t isr_status
#define ENTER_CRITICAL() taskENTER_CRITICAL()
#define EXIT_CRITICAL()  taskEXIT_CRITICAL()
#define ENTER_ISR()      isr_status = taskENTER_CRITICAL_FROM_ISR()
#define EXIT_ISR()       taskEXIT_CRITICAL_FROM_ISR(isr_status)
#endif /* PLATFORM_USE_OS */

/* 使用perf_counter */
#if USE_PERF_COUNTER_TOOL
#include "perf_counter.h"
extern float time_shift_ms(int64_t time);
#endif

#endif /* _PLATFORM_H_ */
