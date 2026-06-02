/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_dac.c
 * 文件描述：DAC-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *          V1.0 2024/11/26 张晓博 初始版本
 */

#include "gd32_dac.h"

static uint32_t dac_align;

/*
 * 功能描述：此函数根据提供的配置结构体初始化指定的DAC外设
 * 入参说明：dev --- 指定的DAC外设
 *          dac_desc --- 指向DAC配置结构体的指针
 * 返 回 值：无
 */
void gd32_dac_init(uint32_t dev, const dac_desc_t *dac_desc) {
    rcu_periph_clock_enable(RCU_DAC);

    dac_deinit();
    if (dac_desc->trig_enable) {
        dac_trigger_enable(dev);
        dac_trigger_source_config(dev, dac_desc->trig_source);
    } else {
        dac_trigger_disable(dev);
    }

    dac_wave_mode_config(dev, dac_desc->wave_mode);
    if (dac_desc->buffer_enable) {
        dac_output_buffer_enable(dev);
    } else {
        dac_output_buffer_disable(dev);
    }
    dac_data_set(dev, dac_desc->dac_align, 0);
    dac_align = dac_desc->dac_align;
    dac_dma_disable(dev);
    dac_enable(dev);
}

/*
 * 功能描述：这个函数用于设置DAC输出电压
 * 入参说明：vol --- 输出电压值，0-3300mv，
 * 返 回 值：无
 */
void gd32_dac_out(uint32_t dev, uint16_t vol) {
    double temp = vol;
    temp /= 1000;
    temp = temp * 4096 / 3.3;

    if (temp >= 4096) temp = 4096;

    dac_data_set(dev, dac_align, temp);
}
