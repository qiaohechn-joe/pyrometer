/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：measure.h
 * 文件描述：红外温度测量驱动代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/03/20  陈军    初始版本
 *           V1.1 2025/04/09  乔鹤    增加单色和比色测温业务
 *           V1.2 2025/04/28  李兆越  增加温度校准和两点校准业务
 */

#ifndef _MEASURE_H_
#define _MEASURE_H_

#include "platform.h"

#define SAMPLE_NO_START      0 /* ADC没开始采样 */
#define SAMPLE_START         1 /* ADC开始采样 */

#define TEMP_AVER_LEN_UP_TH  500 /* 温度滑移平均滤波长度上限值 */
#define TEMP_AVER_LEN_LOW_TH 1   /* 温度滑移平均滤波长度下限值 */
#define NTC_TEMP_AVER_LEN    20  /* 内部温度滑移平均滤波长度 */

#define TEMP_TASK_PERIOD     (configTICK_RATE_HZ / 66) /* 20ms */

#define BUFF_CNT             50 /* 滤波样本数 */
#define SAMPLE_CNT           10 /* 滤波样本数 */
#define REMOVE               5  /* 去极值滤波使用，要去除的极值数量 */

#define TEMP_HISTORY_CNT     50 /* 记录滤波后的电压的数量 */

#define AVERAGE_FILTER       0
#define PEAK_HOLDING         1
#define VALLEY_HOLDING       2

/* AD7124各通道映射 */
#define ADC_CH_SPEC_NB       0 /* 窄带映射 */
#define ADC_CH_SPEC_WB       2 /* 宽带映射 */
#define ADC_CH_NTC_SPEC      4 /* 监测光电二极管部分温度（NTC）通道映射 */
#define ADC_CH_NTC_TIA       6 /* 监测跨阻放大器部分温度（NTC）通道映射 */

typedef struct {
    float    buff[TEMP_HISTORY_CNT]; /* 温度数据缓存 */
    int      front;                  /* 队列头部的索引 */
    uint32_t count;                  /* 目前队列中的元素数量 */
    uint8_t  buffer_size;
} circle_buffer_t;

/* 保存滑动平均相关的状态数据及温度缓存 */
typedef struct {
    int       buff[TEMP_AVER_LEN_UP_TH]; /* 温度数据缓存 */
    int       front;                     /* 队列头部的索引 */
    long long sum;                       /* 数组中元素的总和 */
    uint32_t  count;                     /* 目前队列中的元素数量 */
} slid_aver_t;

/* 传感器光谱数据 */
typedef struct {
    //    uint32_t pos;           /* 原始数据缓存位置 */
    //    int       buff[BUFF_CNT]; /* AD通道采集原始数据缓存 */
    long long volt_raw;         /* 原始电压 */
    long long volt_norm;        /* 归一化后电压 */
    long long volt_filter;      /* 滤波后电压 */
    long long volt_ambient;     /* 环温补偿后电压 */
    long long last_volt_filter; /* 上次归一化后电压 */
    long long volt_compensate;  /* 补偿后电压 */
} sens_spec_t;

/* 传感器NTC数据 */
typedef struct {
    long long volt_raw;    /* 原始电压 */
    long long volt_filter; /* 滤波后电压 */
} sens_ntc_t;

typedef struct {
    int             gl_to_gm;     /* 切档L切M */
    int             gm_to_gh;     /* 切档M切H */
    int             gh_to_gm;     /* 切档H切M */
    int             gm_to_gl;     /* 切档M切L */
    circle_buffer_t history_volt; /* 历史电压数据 */
    int             flag;         /* 切档M切L */
    // 平滑过渡
    float    smooth_lastTemp;    // 切档前温度
    float    smooth_currentTemp; // 当前温度
    uint32_t smooth_count;       // 平滑计数
    uint8_t  discard_status;     // 切档丢弃电压标志
    uint32_t discard_count;      // 切档丢弃电压计数
    bool     smooth_sta;         // 平滑状态
} gain_sw_data_t;

extern sens_spec_t sens_spec[SENS_CH_SPEC_CNT];
extern sens_ntc_t  sens_ntc[SENS_CH_NTC_CNT];

int   temp_init(void);
int   calibrate_temperature(void);
void  gain_status_init(void);
float switch_temp_smooth(float smoothRate, int32_t smoothCount, float currentTemp, int32_t spect);

#endif /* _MEASURE_H_ */
