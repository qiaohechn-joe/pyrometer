/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_adc.c
 * 文件描述：ADC-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/07/19 陈军  优化初始化配置，提高灵活性，增加错误检测
 */

#include "gd32_ll.h"
#include "gd32_adc.h"

/* 内部函数 */
uint16_t get_adc_ch_value(uint32_t dev, uint8_t ch, int timeout, int *error);

/*
 * 功能描述：此函数根据提供的配置结构体初始化指定的ADC外设
 * 入参说明：dev --- 指定的ADC外设
 *           adc_desc --- 指向ADC配置结构体的指针
 * 返 回 值：无
 */
void gd32_adc_init(uint32_t dev, const adc_desc_t *adc_desc) {
    /* 时钟配置 */
    if (dev == ADC0) {
        rcu_periph_clock_enable(RCU_ADC0);
    } else if (dev == ADC1) {
        rcu_periph_clock_enable(RCU_ADC1);
    } else if (dev == ADC2) {
        rcu_periph_clock_enable(RCU_ADC2);
    } else {
        return;
    }

    /* 重置ADC */
    adc_deinit();
    /* 时钟配置 */
    adc_clock_config(adc_desc->clk_div);

    /* ADC模式配置 */
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    /* ADC连续转换功能配置 */
    adc_special_function_config(dev, ADC_CONTINUOUS_MODE, adc_desc->continuous_sw);
    /* ADC扫描模式配置 */
    adc_special_function_config(dev, ADC_SCAN_MODE, adc_desc->scan_sw);
    /* ADC数据对齐配置 */
    adc_data_alignment_config(dev, adc_desc->data_align);

    adc_resolution_config(dev, ADC_RESOLUTION_12B);

    /* ADC通道长度配置 */
    adc_channel_length_config(dev, ADC_ROUTINE_CHANNEL, adc_desc->ch_len);
    /* ADC外部触发配置 */
    adc_external_trigger_source_config(dev, ADC_ROUTINE_CHANNEL, adc_desc->exter_trig);
    adc_external_trigger_config(dev, ADC_ROUTINE_CHANNEL, adc_desc->trig_mode);
    /* 使能ADC接口 */
    adc_enable(dev);
    /* 短暂延时，确保ADC稳定 */
    gd32_delay_ms(20);
    /* ADC校准 */
    adc_calibration_enable(dev);
}

/*
 * 功能描述：此函数获ADC值（均值滤波）
 * 入参说明：dev --- 指定的ADC外设
 *           ch --- ADC通道
 *           filter_len --- 均值滤波长度
 *           timeout --- 等待转换结束的超时时间
 *           error --- 指向错误状态的指针
 * 返 回 值：无
 */
float adc_val(uint32_t dev, uint8_t ch, uint8_t filter_len, int timeout, int *error) {
    if (filter_len == 0) {
        *error = 1;
        return 0.0f;
    }

    uint32_t adc_sum_val = 0;
    int      i           = 0;
    *error               = 0; /* 初始化错误状态 */

    for (i = 0; i < filter_len; i++) {
        adc_sum_val += get_adc_ch_value(dev, ch, timeout, error);
        if (*error != 0) {
            return 0.0f;
        }
    }

    return ((adc_sum_val / filter_len) * 3.3f / 4096.0f);
}

/*
 * 功能描述：此函数获取指定通道的ADC值
 * 入参说明：dev --- 指定的ADC外设
 *           ch --- ADC通道
 *           timeout --- 等待转换结束的超时时间
 *           error --- 指向错误状态的指针
 * 返 回 值：无
 */
uint16_t get_adc_ch_value(uint32_t dev, uint8_t ch, int timeout, int *error) {
    adc_routine_channel_config(dev, 0, ch, ADC_SAMPLETIME_56);
    /* 使用软件触发 */
    adc_software_trigger_enable(dev, ADC_ROUTINE_CHANNEL);

    while (!adc_flag_get(dev, ADC_FLAG_EOC) && timeout--);
    if (timeout <= 0) {
        /* 处理超时错误 */
        *error = 1;
        return 0;
    }

    adc_flag_clear(dev, ADC_FLAG_EOC);
    return (adc_routine_data_read(dev) & 0xFFFF);
}
