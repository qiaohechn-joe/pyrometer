/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_dci.c
 * 文件描述：数字摄像头（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           v1.0 2025/3/6 张晓博 初始版本
 */

#include "gd32_ll.h"
#include "platform.h"
#include "gd32_dci.h"

static void (*dci_over_callback)(void *arg); /* 回调函数，帧传输完成中断调用 */
static void (*dci_dma_callback)(void *arg);  /* 回调函数，dma接收中断调用 */

static void dci_irq_handler(void) {
    DEFINE_CPU_SR;

    ENTER_ISR();
    if (dci_interrupt_flag_get(DCI_INT_FLAG_EF) == SET) {
        dma_interrupt_disable(DMA1, DMA_CH7, DMA_CHXCTL_FTFIE);
        dma_channel_disable(DMA1, DMA_CH7);

        dci_interrupt_flag_clear(DCI_INT_FLAG_EF); /* 清除帧中断 */
        dci_interrupt_disable(DCI_INT_EF);         /* 关闭帧中断 */
        dci_capture_disable();

        if (dci_over_callback != NULL) {
            dci_over_callback(0);
        }
    }
    EXIT_ISR();
}

static void dci_dma_irq_handler(void) {
    DEFINE_CPU_SR;

    ENTER_ISR();

    if (dma_interrupt_flag_get(DMA1, DMA_CH7, DMA_INT_FLAG_FTF) == SET) /* DMA传输完成 */
    {
        dma_interrupt_flag_clear(DMA1, DMA_CH7, DMA_INT_FLAG_FTF); /* 清除DMA传输完成中断标志位 */
        if (dci_dma_callback != NULL) {
            dci_dma_callback(0);
        }
    }
    EXIT_ISR();
}

/*
 * 功能描述：dci接口初始化
 * 入参说明：rx_cb --- dci一帧接收完成回调函数
 * 返 回 值：无
 */
void gd32_dci_init(void (*rx_cb)(void *arg)) {
    dci_parameter_struct dci_struct;

    dci_over_callback = rx_cb;
    dci_pin_init();

    rcu_periph_clock_enable(RCU_DCI); /* 使能DCMI时钟 */

    dci_deinit();

    dci_struct.capture_mode     = DCI_CAPTURE_MODE_CONTINUOUS; /* 连续捕获模式 */
    dci_struct.clock_polarity   = DCI_CK_POLARITY_RISING;      /* PCL 上升沿有效 */
    dci_struct.hsync_polarity   = DCI_HSYNC_POLARITY_LOW;      /* HSYNC 低电平有效 */
    dci_struct.vsync_polarity   = DCI_VSYNC_POLARITY_LOW;      /* VSYNC低电平有效 */
    dci_struct.frame_rate       = DCI_FRAME_RATE_ALL;          /* 两帧捕获一次 */
    dci_struct.interface_format = DCI_INTERFACE_FORMAT_8BITS;  /* 8位 */
    dci_init(&dci_struct);                                     /* dci初始化 */

    dci_jpeg_enable();
    dci_enable();

    gd32_setup_isr(DCI_IRQn, dci_irq_handler, 0);
    gd32_setup_irq(DCI_IRQn, 13);
}

/*
 * 功能描述：dci外设dma配置
 * 入参说明：periph_addr --- 数据地址
 *           mem_addr0 --- 存储地址0
 *           mem_addr1 --- 存储地址1
 *           num --- DMA传输的数据个数
 *           dma_cb --- DMA传输完成回调函数
 * 返 回 值：无
 */
void gd32_dci_dma_init(uint32_t periph_addr, uint32_t mem_addr0, uint32_t mem_addr1, uint32_t num, void (*dma_cb)(void *arg)) {
    dma_multi_data_parameter_struct dma_init_parameter;

    dci_dma_callback = dma_cb;

    rcu_periph_clock_enable(RCU_DMA1);

    dma_deinit(DMA1, DMA_CH7);
    dma_init_parameter.periph_addr        = (uint32_t)periph_addr;
    dma_init_parameter.periph_width       = DMA_PERIPH_WIDTH_32BIT;
    dma_init_parameter.periph_inc         = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_parameter.memory0_addr       = mem_addr0;
    dma_init_parameter.memory_width       = DMA_MEMORY_WIDTH_32BIT;
    dma_init_parameter.memory_inc         = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_parameter.memory_burst_width = DMA_MEMORY_BURST_SINGLE;
    dma_init_parameter.periph_burst_width = DMA_PERIPH_BURST_SINGLE;
    dma_init_parameter.critical_value     = DMA_FIFO_2_WORD;
    dma_init_parameter.circular_mode      = DMA_CIRCULAR_MODE_ENABLE;
    dma_init_parameter.direction          = DMA_PERIPH_TO_MEMORY;
    dma_init_parameter.number             = num;
    dma_init_parameter.priority           = DMA_PRIORITY_HIGH;

    dma_multi_data_mode_init(DMA1, DMA_CH7, &dma_init_parameter);
    dma_channel_subperipheral_select(DMA1, DMA_CH7, DMA_SUBPERI1);

    if (mem_addr1 != 0) /* 使用双缓冲 */
    {
        dma_switch_buffer_mode_config(DMA1, DMA_CH7, (uint32_t)mem_addr1, DMA_MEMORY_1);
        dma_switch_buffer_mode_enable(DMA1, DMA_CH7, ENABLE);
    }

    gd32_setup_isr(DMA1_Channel7_IRQn, dci_dma_irq_handler, 0);
    gd32_setup_irq(DMA1_Channel7_IRQn, 12);
}

/*
 * 功能描述：dci dma开启
 * 入参说明：无
 * 返 回 值：无
 */
void gd32_dci_dma_start(void) {

    dma_flag_clear(DMA1, DMA_CH7, DMA_FLAG_FEE);
    dma_flag_clear(DMA1, DMA_CH7, DMA_FLAG_SDE);
    dma_flag_clear(DMA1, DMA_CH7, DMA_FLAG_TAE);
    dma_flag_clear(DMA1, DMA_CH7, DMA_FLAG_HTF);
    dma_flag_clear(DMA1, DMA_CH7, DMA_FLAG_FTF);

    dma_interrupt_enable(DMA1, DMA_CH7, DMA_CHXCTL_FTFIE); /* 开启传输完成中断 */
    dma_channel_enable(DMA1, DMA_CH7);                     /* 使能DMA */
}

/*
 * 功能描述：dci开启
 * 入参说明：无
 * 返 回 值：无
 */
void gd32_dci_start(void) {
    dci_interrupt_enable(DCI_INT_EF);
    dci_capture_enable();
}

/*
 * 功能描述：dci关闭
 * 入参说明：无
 * 返 回 值：无
 */
void gd32_dci_stop(void) {
    dci_capture_disable();
}
