/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_dma.c
 * 文件描述：DMA-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/12/01 陈军  增加DMA中断和标志位操作宏
 */

#include "gd32_dma.h"

/*
 * 功能描述：此函数用于初始化一个DMA流
 * 入参说明：xfer --- 指向 dma_xfer_t 结构体的指针
 *           prio --- DMA传输的优先级，DMA_PRIORITY_xxx
 *           circular --- 循环模式？
 * 返 回 值：无
 */
void gd32_dma_init(dma_xfer_t *xfer, int prio, int circular) {
    dma_single_data_parameter_struct para;

    /* DMA控制 */
    if (xfer->dmac == DMA0) {
        rcu_periph_clock_enable(RCU_DMA0);
    } else if (xfer->dmac == DMA1) {
        rcu_periph_clock_enable(RCU_DMA1);
    }

    dma_deinit(xfer->dmac, xfer->channel);

    /* 参数 */
    dma_single_data_para_struct_init(&para);

    para.periph_addr = xfer->paddr;
    if (xfer->pinc) {
        para.periph_inc = DMA_PERIPH_INCREASE_ENABLE;
    }
    if (xfer->minc) {
        para.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    }

    switch (xfer->width) {
        case 1: para.periph_memory_width = DMA_PERIPH_WIDTH_8BIT; break;
        case 2: para.periph_memory_width = DMA_PERIPH_WIDTH_16BIT; break;
        case 4: para.periph_memory_width = DMA_PERIPH_WIDTH_32BIT; break;
        default: para.periph_memory_width = DMA_PERIPH_WIDTH_8BIT; break;
    }

    para.circular_mode = circular;
    para.direction     = xfer->dir;
    para.priority      = prio;

    /* 配置 */
    dma_channel_disable(xfer->dmac, xfer->channel);
    dma_single_data_mode_init(xfer->dmac, xfer->channel, &para);
    dma_channel_subperipheral_select(xfer->dmac, xfer->channel, xfer->periph);
    dma_flow_controller_config(xfer->dmac, xfer->channel, xfer->flow);
}

/*
 * 功能描述：此函数启动一个DMA传输。
 * 入参说明：xfer --- 指向 dma_xfer_t 结构体的指针
 *           buff --- DMA传输缓冲区
 *           len --- DMA传输长度（字节）
 * 返 回 值：无
 */
void gd32_dma_start(dma_xfer_t *xfer, void *buff, int len) {
    /* 传输计数 */
    xfer->total_cnt = (xfer->flow == DMA_FLOW_CONTROLLER_PERI) ? 0x0ffff : (len / xfer->width);

    /* 失能DMA通道 */
    dma_channel_disable(xfer->dmac, xfer->channel);

    /* 内存地址，大小，方向 */
    dma_memory_address_config(xfer->dmac, xfer->channel, DMA_MEMORY_0, (uint32_t)buff);
    dma_transfer_number_config(xfer->dmac, xfer->channel, xfer->total_cnt);
    dma_transfer_direction_config(xfer->dmac, xfer->channel, xfer->dir);

    /* 清除标志 */
    dma_flag_clear(xfer->dmac, xfer->channel, DMA_FLAG_FEE);
    dma_flag_clear(xfer->dmac, xfer->channel, DMA_FLAG_SDE);
    dma_flag_clear(xfer->dmac, xfer->channel, DMA_FLAG_TAE);
    dma_flag_clear(xfer->dmac, xfer->channel, DMA_FLAG_HTF);
    dma_flag_clear(xfer->dmac, xfer->channel, DMA_FLAG_FTF);

    /* 使能DMA通道 */
    dma_channel_enable(xfer->dmac, xfer->channel);
}

/*
 * 功能描述：此函数启动一个DMA传输。
 * 入参说明：xfer --- 指向 dma_xfer_t 结构体的指针
 *           buff --- DMA传输缓冲区
 *           len --- DMA传输长度（字节）
 * 返 回 值：无
 */
void gd32_dma_rx(dma_xfer_t *xfer, void *buff, int len) {
    /* 传输计数 */
    // xfer->total_cnt = (xfer->flow == DMA_FLOW_CONTROLLER_PERI) ? 0x0ffff : (len / xfer->width);
    xfer->total_cnt = len / xfer->width;

    /* 失能DMA通道 */
    dma_channel_disable(xfer->dmac, xfer->channel);

    /* 内存地址，大小，方向 */
    dma_memory_address_config(xfer->dmac, xfer->channel, DMA_MEMORY_0, (uint32_t)buff);
    dma_transfer_number_config(xfer->dmac, xfer->channel, xfer->total_cnt);
    dma_transfer_direction_config(xfer->dmac, xfer->channel, xfer->dir);

    /* 清除标志 */
    dma_flag_clear(xfer->dmac, xfer->channel, DMA_FLAG_FEE);
    dma_flag_clear(xfer->dmac, xfer->channel, DMA_FLAG_SDE);
    dma_flag_clear(xfer->dmac, xfer->channel, DMA_FLAG_TAE);
    dma_flag_clear(xfer->dmac, xfer->channel, DMA_FLAG_HTF);
    dma_flag_clear(xfer->dmac, xfer->channel, DMA_FLAG_FTF);

    /* 使能DMA通道 */
    dma_channel_enable(xfer->dmac, xfer->channel);
}
