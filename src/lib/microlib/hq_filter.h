/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：hq_filter.h
 * 文件描述：滤波算法
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/02/10 张晓博   初始版本
 *           V1.1 2025/03/26 陈  军   修改去极值滑移均值算法
 */

#ifndef _HQ_FILTER_H_
#define _HQ_FILTER_H_

#include "gd32f4xx.h"
#include "platform.h"

#define FILTER_LEN_MAX 500

/* EMA滤波算法相关的状态数据 */
typedef struct {
    float  shadow;         /* 当前的 EMA 值 */
    double decay;          /* 衰减因子 */
    int    is_initialized; /* 标记是否初始化 */
    int    is_biased;
} exp_mov_avg_t;

/* 保存滑动平均相关的状态数据 */
typedef struct {
    long long buff[FILTER_LEN_MAX]; /* 数据缓存 */
    int       front;                /* 队列头部的索引 */
    long long sum;                  /* 数组中元素的总和 */
    uint32_t  count;                /* 目前队列中的元素数量 */
} slid_remove_t;
/* 保存峰谷值保持相关的状态数据 */
typedef struct {
    long long data;  /* 数据缓存 */
    uint32_t  count; /* 保持计数 */
} slid_holding_t;

void      init_ema(exp_mov_avg_t *avg, double decay, int biased);
double    update_ema(exp_mov_avg_t *avg, float data);
long long sliding_remove_average(slid_remove_t *slid_remove_volt, int size, long long new_data, long long *buff_copy);
long long peak_holding_fliter(slid_holding_t *slid_holding_data, int size, long long new_data);
long long valley_holding_fliter(slid_holding_t *slid_holding_data, int size, long long new_data);

#endif /* _HQ_FILTER_H_ */
