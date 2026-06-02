/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_dma.h
 * 文件描述：DMA-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/12/01 陈军  增加DMA中断和标志位操作宏
 */

#ifndef _GD32_DMA_H_
#define _GD32_DMA_H_

#include "gd32f4xx.h"

typedef struct {
    uint32_t               dmac;
    dma_channel_enum       tx_ch;
    dma_subperipheral_enum tx_periph;
    dma_channel_enum       rx_ch;
    dma_subperipheral_enum rx_periph;
} dma_cfg_t;

/* DMA传输描述符 */
typedef struct {
    uint32_t               dmac;      /* DMA控制器，DMA0或DMA1 */
    dma_channel_enum       channel;   /* DMA通道, DMA_CHx（x=0..7） */
    dma_subperipheral_enum periph;    /* 外设，DMA_SUBPERIx（x=0..7） */
    uint32_t               paddr;     /* 外设地址 */
    uint32_t               width;     /* 传输宽度（字节），1/2/4 */
    uint8_t                minc;      /* 是否修改内存指针？ */
    uint8_t                pinc;      /* 是否修改外设指针？ */
    uint8_t                dir;       /* 方向：DMA_PERIPH_TO_MEMORY 或 DMA_MEMORY_TO_PERIPH */
    uint8_t                flow;      /* 流控制器：DMA_FLOW_CONTROLLER_DMA 或 DMA_FLOW_CONTROLLER_PERI */
    uint32_t               total_cnt; /* 总计数,todo 循环模式下不需要，CHCNT会自动重置 */
    IRQn_Type              irqn;      /* 对应中断号 */
} dma_xfer_t;

#define dma_flag_valid(dma)                                                                                                                          \
    ((dma_flag_get(dma->dmac, dma->channel, DMA_FLAG_FTF) == SET) || (dma_flag_get(dma->dmac, dma->channel, DMA_FLAG_TAE) == SET) ||                 \
     (dma_flag_get(dma->dmac, dma->channel, DMA_FLAG_SDE) == SET))

#define clear_dma_flag(dma)                                                                                                                          \
    {                                                                                                                                                \
        dma_flag_clear(dma->dmac, dma->channel, DMA_FLAG_FTF);                                                                                       \
        dma_flag_clear(dma->dmac, dma->channel, DMA_FLAG_TAE);                                                                                       \
        dma_flag_clear(dma->dmac, dma->channel, DMA_FLAG_SDE);                                                                                       \
        dma_flag_clear(dma->dmac, dma->channel, DMA_INTF_FEEIF);                                                                                     \
    }

#define clear_all_dma_flag(dma)                                                                                                                      \
    {                                                                                                                                                \
        dma_flag_clear(dma->dmac, dma->channel, DMA_FLAG_FEE);                                                                                       \
        dma_flag_clear(dma->dmac, dma->channel, DMA_FLAG_SDE);                                                                                       \
        dma_flag_clear(dma->dmac, dma->channel, DMA_FLAG_TAE);                                                                                       \
        dma_flag_clear(dma->dmac, dma->channel, DMA_FLAG_HTF);                                                                                       \
        dma_flag_clear(dma->dmac, dma->channel, DMA_FLAG_FTF);                                                                                       \
    }
//#define enable_dma_irq(dma)                                                                                                                          \
//    {                                                                                                                                                \
//        dma_interrupt_enable(dma->dmac, dma->channel, DMA_CHXCTL_FTFIE);                                                                             \
//        dma_interrupt_enable(dma->dmac, dma->channel, DMA_CHXCTL_TAEIE);                                                                             \
//        dma_interrupt_enable(dma->dmac, dma->channel, DMA_CHXCTL_SDEIE);        \
//        dma_interrupt_enable(dma->dmac, dma->channel, DMA_CHXFCTL_FEEIE);    \
//    }

#define enable_dma_irq(dma)                                                                                                                          \
    {                                                                                                                                                \
        dma_interrupt_enable(dma->dmac, dma->channel, DMA_CHXCTL_FTFIE);                                                                             \
        dma_interrupt_enable(dma->dmac, dma->channel, DMA_CHXCTL_TAEIE);                                                                             \
        dma_interrupt_enable(dma->dmac, dma->channel, DMA_CHXCTL_SDEIE);                                                                             \
        dma_interrupt_enable(dma->dmac, dma->channel, DMA_CHXFCTL_FEEIE);                                                                            \
    }

#define disable_dma_irq(dma)                                                                                                                         \
    {                                                                                                                                                \
        dma_interrupt_disable(dma->dmac, dma->channel, DMA_CHXCTL_FTFIE);                                                                            \
        dma_interrupt_disable(dma->dmac, dma->channel, DMA_CHXCTL_TAEIE);                                                                            \
        dma_interrupt_disable(dma->dmac, dma->channel, DMA_CHXCTL_SDEIE);                                                                            \
    }

void gd32_dma_init(dma_xfer_t *xfer, int prio, int circular);
void gd32_dma_start(dma_xfer_t *xfer, void *buff, int len);
void gd32_dma_rx(dma_xfer_t *xfer, void *buff, int len);

#endif /* _GD32_DMA_H_ */
