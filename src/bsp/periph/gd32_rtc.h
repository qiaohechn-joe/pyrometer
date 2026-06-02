/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_rtc.h
 * 文件描述：RTC-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/05/21 陈军  更新RTC初始化逻辑
 *           V1.2 2024/12/04 陈军  统一结构体定义方式
 */

#ifndef _GD32_RTC_H_
#define _GD32_RTC_H_

#include "gd32f4xx.h"

/* LXTAL准备超时时间：微秒 */
#define LXTAL_READY_TIMEO 100000

typedef __packed struct {
    uint16_t year;   /* 年：2000-2099 */
    uint8_t  month;  /* 月：1-12 */
    uint8_t  mday;   /* 日：1-31 */
    uint8_t  wday;   /* 星期：1-7 */
    uint8_t  hour;   /* 时：0-23 */
    uint8_t  minute; /* 分：0-59 */
    uint8_t  second; /* 秒：0-59 */
} rtc_time_t;

int      gd32_rtc_init(void);
int      gd32_rtc_set_second(uint32_t val);
uint32_t gd32_rtc_get_second(void);
int      gd32_rtc_set_time(const rtc_time_t *time);
int      gd32_rtc_get_time(rtc_time_t *time);
int      gd32_rtc_diff_time(const rtc_time_t *time1, const rtc_time_t *time2);
uint32_t gd32_rtc_time_to_sec(const rtc_time_t *time);
void     gd32_rtc_sec_to_time(uint32_t sec, rtc_time_t *time);

#endif /* _GD32_RTC_H_ */
