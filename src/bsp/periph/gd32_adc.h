/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_adc.h
 * 文件描述：ADC-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/07/19 陈军  优化初始化配置，提高灵活性，增加错误检测
 */

#ifndef _GD32_ADC_H_
#define _GD32_ADC_H_

#include "gd32f4xx.h"

typedef struct {
    rcu_periph_enum pid;  /* RCU_GPIOx */
    uint16_t        port; /* GPIOx */
    uint32_t        pin;  /* GPIO_PIN_xx */
} adc_pin_t;

/* ADCtypedef 描述符 */
typedef struct {
    uint32_t      clk_div;       /* ADC时钟分频 */
    ControlStatus continuous_sw; /* 连续模式开关（DISABLE/ENABLE） */
    ControlStatus scan_sw;       /* 扫描模式开关（DISABLE/ENABLE） */
    uint32_t      data_align;    /* 数据对齐方式，例如ADC_DATAALIGN_RIGHT */
    uint32_t      ch_len;        /* 通道长度 */
    uint32_t      exter_trig;    /* 外部触发源，例如ADC_EXTTRIG_REGULAR_T0_CH0 */
    uint32_t      trig_mode;     /* 触发模式，例如EXTERNAL_TRIGGER_DISABLE */
} adc_desc_t;

void     gd32_adc_init(uint32_t dev, const adc_desc_t *adc_desc);
float    adc_val(uint32_t dev, uint8_t ch, uint8_t filter_len, int timeout, int *error);
uint16_t get_adc_ch_value(uint32_t dev, uint8_t ch, int timeout, int *error);
#endif /* _GD32_ADC_H_ */
