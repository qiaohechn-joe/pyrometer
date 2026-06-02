/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ntc_cfg.h
 * 文件描述：NTC传感器配置代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/03/24 陈军  初始版本
 */

#ifndef _NTC_CFG_H_
#define _NTC_CFG_H_

#include <math.h>
#include "gd32_ll.h"

#define NTC_REF_VOL  2500000u  /* NTC参考电压，单位为微伏(uV) */
#define NTC_REF_RES  4700      /* NTC参考电阻值，单位为欧姆(ohm) */
#define NTC_STD_TEMP 25        /* 标准温度，25摄氏度 */
#define NTC_STD_RES  100000.0f /* NTC在标准温度下的阻值，单位为欧姆(ohm) */
#define NTC_BX_VAL   4100.0f   /* NTC材料的B值，决定了电阻随温度的变化关系 */
#define A_ZERO       273.15f   /* 绝对零度，开氏温度转换常数 */

/*
 * 功能描述：计算NTC传感器两端的电阻值
 * 入参说明：volt_v --- 输入电压值，单位为微伏 (uV)
 * 返 回 值：1)计算得到的电阻值，单位为欧姆 (ohm)
 *           2)如果输入电压值无效或导致除零，则返回-1.0以指示错误
 */
static inline float get_ntc_res(uint32_t volt_uv) {
    if (volt_uv >= NTC_REF_VOL || volt_uv <= 0) {
        return -1.0f;
    }
    /* 计算NTC电阻值，公式基于电压分压原理 */
    return (float)volt_uv * NTC_REF_RES / (float)(NTC_REF_VOL - volt_uv);
}

/*
 * 功能描述：依据NTC电阻值计算温度
 * 入参说明：res_val_ohm --- 输入的电阻值，单位为欧姆 (ohm)
 * 返 回 值：1)计算得到的温度值，单位为摄氏度 (°C)
 *           2)如果电阻值无效，则返回-275.0以指示错误（低于绝对零度）
 */
static inline float get_ntc_temp(float res_val_ohm) {
    if (res_val_ohm <= 0.0f) {
        return -275.0f;
    }
    /* 使用B公式，通过电阻计算温度 */
    float log_res_ratio   = log(res_val_ohm / NTC_STD_RES);                              /* 计算电阻比的自然对数 */
    float reciprocal_temp = log_res_ratio / NTC_BX_VAL + 1.0f / (A_ZERO + NTC_STD_TEMP); /* 计算温度的倒数 */
    return (1.0f / reciprocal_temp - A_ZERO);                                            /* 转换为摄氏度并返回 */
}

#endif /* _NTC_CFG_H_ */
