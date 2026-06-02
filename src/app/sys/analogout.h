/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：analogout.h
 * 文件描述：模拟输出业务驱动代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 张韩 陈军  初始版本
 */

#ifndef _ANALOGOUT_H_
#define _ANALOGOUT_H_

#include "platform.h"
#include "self_check.h"

#define ANA_OUT_AUTO       60000000 /* 模拟自动控制输出 */
#define ANA_MUTEX_TIMEOUT  (configTICK_RATE_HZ / 2)

/* 配置 */
#define ANA_ITOV_COEF      0.2f /* 0~20ma 转 0~5v */
#define ANA_VTOV_COEF      0.5f /* 0~10v 转 0~5v */
#define ANA_VOLT_RANGE_MAX 10
#define ANA_VOLT_RANGE_MIN 1
#define CALIB_VOLT_HIGH    9500000 /* uV */
#define CALIB_VOLT_LOW     1000000
#define CALIB_CURRENT_HIGH 19500000 /* nA */
#define CALIB_CURRENT_LOW  1000000

typedef hq_packed struct {
    int     temp_upper;    /* 温度上限 */
    int     temp_lower;    /* 温度下限 */
    uint8_t current_range; /* 电流阈值差 */
    float   current_a;     /* 电流系数/斜率 */
    float   current_b;     /* 电流系数/偏移 */
    uint8_t voltage_range; /* 电压阈值差 */
    float   voltage_a;     /* 电压系数/斜率 */
    float   voltage_b;     /* 电压系数/偏移 */
} ana_status_t;

int  anolog_out_init(void);
void anolog_output(void);
void ana_threshold_range(void);
void get_current_calib_para(void);
void get_voltage_calib_para(void);

#endif /* _ANALOGOUT_H_ */
