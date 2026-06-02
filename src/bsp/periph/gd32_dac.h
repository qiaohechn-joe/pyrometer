/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_dac.h
 * 文件描述：DAC-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/11/26 张晓博  初始版本
 */

#ifndef _GD32_DAC_H_
#define _GD32_DAC_H_

#include "gd32f4xx.h"

/* DAC描述符 */
typedef struct {
    uint32_t dac_align;     /* 数据宽度以及对齐方式 */
    uint32_t buffer_enable; /* 数据输出缓冲，DISABLE/ENABLE */
    uint32_t wave_mode;     /* 波形输出模式 */
    uint32_t trig_enable;   /* 触发使能，DISABLE/ENABLE*/
    uint32_t trig_source;   /* 触发源，DAC_TRIGGER_T1_TRGO */
} dac_desc_t;

void gd32_dac_init(uint32_t dev, const dac_desc_t *dac_desc);
void gd32_dac_out(uint32_t dev, uint16_t vol);

#endif
