/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_spi.c
 * 文件描述：SPI-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/11/22 陈军  增加SPI模式
 *           V1.2 2024/11/25 陈军  增加SPI发送和接收DMA信息填充函数
 */

#include "gd32_spi.h"

/* SPI的DMA映射配置表（根据实际使用配置） */
const dma_cfg_t spi_dma_cfg[SPI_MAX_COUNT] = {
    /*** SPI0 ***/
    /* 发送通道：DMA_CH3(DMA_SUBPERI3)或DMA_CH5(DMA_SUBPERI3) */
    /* 接收通道：DMA_CH0(DMA_SUBPERI3)或DMA_CH2(DMA_SUBPERI3) */
    {DMA1, DMA_CH3, DMA_SUBPERI3, DMA_CH0, DMA_SUBPERI3},
    /*** SPI1 ***/
    /* 发送通道：DMA_CH4(DMA_SUBPERI0) */
    /* 接收通道：DMA_CH3(DMA_SUBPERI0) */
    {DMA0, DMA_CH4, DMA_SUBPERI0, DMA_CH3, DMA_SUBPERI0},
    /*** SPI2 ***/
    /* 发送通道：DMA_CH5(DMA_SUBPERI0)或DMA_CH7(DMA_SUBPERI0) */
    /* 接收通道：DMA_CH0(DMA_SUBPERI0)或DMA_CH2(DMA_SUBPERI0) */
    {DMA0, DMA_CH7, DMA_SUBPERI0, DMA_CH0, DMA_SUBPERI0},
    /*** SPI3 ***/
    /* 发送通道：DMA_CH1(DMA_SUBPERI4)或DMA_CH4(DMA_SUBPERI5) */
    /* 接收通道：DMA_CH0(DMA_SUBPERI4)或DMA_CH3(DMA_SUBPERI5) */
    {DMA1, DMA_CH1, DMA_SUBPERI4, DMA_CH0, DMA_SUBPERI4},
    /*** SPI4 ***/
    /* 发送通道：DMA_CH4(DMA_SUBPERI2)或DMA_CH6(DMA_SUBPERI7) */
    /* 接收通道：DMA_CH3(DMA_SUBPERI2)或DMA_CH5(DMA_SUBPERI7) */
    {DMA1, DMA_CH4, DMA_SUBPERI2, DMA_CH3, DMA_SUBPERI2},
    /*** SPI5 ***/
    /* 发送通道：DMA_CH5(DMA_SUBPERI1) */
    /* 接收通道：DMA_CH6(DMA_SUBPERI1) */
    {DMA1, DMA_CH5, DMA_SUBPERI1, DMA_CH6, DMA_SUBPERI1}};

#define get_dmac(dev)         (spi_dma_cfg[get_spi_index(dev)].dmac)
#define get_txdma_periph(dev) (spi_dma_cfg[get_spi_index(dev)].tx_periph)
#define get_rxdma_periph(dev) (spi_dma_cfg[get_spi_index(dev)].rx_periph)
#define get_txdma_ch(dev)     (spi_dma_cfg[get_spi_index(dev)].tx_ch)
#define get_rxdma_ch(dev)     (spi_dma_cfg[get_spi_index(dev)].rx_ch)

static const uint32_t CLK_MODE_CFG[] = {SPI_CK_PL_LOW_PH_1EDGE, SPI_CK_PL_LOW_PH_2EDGE, SPI_CK_PL_HIGH_PH_1EDGE, SPI_CK_PL_HIGH_PH_2EDGE};

/* 内部函数 */
static void init_peripheral(uint32_t dev);

/*
 * 功能描述：此函数将SPI初始化为主设备
 * 入参说明：dev --- SPIx（x=0至5）
 *           spi_mode --- SPI模式选择（SPI_MASTER/SPI_SLAVE）
 *           log2div --- log2(时钟分频)，1至8
 *           clock_mode --- 时钟模式，CPOL 和 CPHA 的组合，从 0b00 到 0b11
 *           word --- 字长，8位或16位
 * 返 回 值：无
 */
void gd32_spi_init(uint32_t dev, uint32_t spi_mode, int log2div, int clock_mode, int word) {
    spi_parameter_struct para;

    /* 初始化SPI外设 */
    init_peripheral(dev);

    /* 参数 */
    if (log2div < 1) {
        log2div = 1;
    }
    if (log2div > 8) {
        log2div = 8;
    }
    if ((word != 8) && (word != 16)) {
        word = 8;
    }

    spi_struct_para_init(&para);
    para.device_mode          = spi_mode;
    para.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    para.frame_size           = (word == 8) ? SPI_FRAMESIZE_8BIT : SPI_FRAMESIZE_16BIT;
    para.nss                  = (spi_mode == SPI_MASTER) ? SPI_NSS_SOFT : SPI_NSS_HARD;
    para.clock_polarity_phase = CLK_MODE_CFG[clock_mode & 0x03];
    para.prescale             = CTL0_PSC(log2div - 1);
    para.endian               = SPI_ENDIAN_MSB;

    /* 配置 */
    spi_disable(dev);
    spi_nss_output_disable(dev);
    spi_init(dev, &para);
    spi_enable(dev);
}

/*
 * 功能描述：此函数关闭SPI
 * 入参说明：dev --- SPIx（x=0至5）
 * 返回值：无
 */
void gd32_spi_shutdown(uint32_t dev) {
    if (dev == SPI0) {
        rcu_periph_clock_disable(RCU_SPI0);
    } else if (dev == SPI1) {
        rcu_periph_clock_disable(RCU_SPI1);
    } else if (dev == SPI2) {
        rcu_periph_clock_disable(RCU_SPI2);
    } else if (dev == SPI3) {
        rcu_periph_clock_disable(RCU_SPI3);
    } else if (dev == SPI4) {
        rcu_periph_clock_disable(RCU_SPI4);
    } else if (dev == SPI5) {
        rcu_periph_clock_disable(RCU_SPI5);
    }
}

/*
 * 功能描述：此函数执行一次传输：发送数据并接收数据
 * 入参说明：dev --- SPIx（x=0至5）
 *           data --- 要发送的数据
 * 返 回 值：接收到的数据
 */
int gd32_spi_xfer(uint32_t dev, int data) {
    /* 等待直到发送缓存准备好 */
    while (!gd32_spi_tx_rdy(dev));

    /* 发送数据 */
    gd32_spi_tx(dev, data);

    /* 等待直到发送缓存满 */
    while (!gd32_spi_rx_rdy(dev));

    return gd32_spi_rx(dev);
}

/*
 * 功能描述：此函数初始化SPI外设
 * 入参说明：dev --- SPIx（x=0至5）
 * 返 回 值：无
 */
static void init_peripheral(uint32_t dev) {
    if (dev == SPI0) {
        rcu_periph_clock_enable(RCU_SPI0);
        rcu_periph_reset_enable(RCU_SPI0RST);
        rcu_periph_reset_disable(RCU_SPI0RST);
    } else if (dev == SPI1) {
        rcu_periph_clock_enable(RCU_SPI1);
        rcu_periph_reset_enable(RCU_SPI1RST);
        rcu_periph_reset_disable(RCU_SPI1RST);
    } else if (dev == SPI2) {
        rcu_periph_clock_enable(RCU_SPI2);
        rcu_periph_reset_enable(RCU_SPI2RST);
        rcu_periph_reset_disable(RCU_SPI2RST);
    } else if (dev == SPI3) {
        rcu_periph_clock_enable(RCU_SPI3);
        rcu_periph_reset_enable(RCU_SPI3RST);
        rcu_periph_reset_disable(RCU_SPI3RST);
    } else if (dev == SPI4) {
        rcu_periph_clock_enable(RCU_SPI4);
        rcu_periph_reset_enable(RCU_SPI4RST);
        rcu_periph_reset_disable(RCU_SPI4RST);
    } else if (dev == SPI5) {
        rcu_periph_clock_enable(RCU_SPI5);
        rcu_periph_reset_enable(RCU_SPI5RST);
        rcu_periph_reset_disable(RCU_SPI5RST);
    }
}

/*
 * 功能描述：此函数用于填充SPIx发送的DMA信息
 * 入参说明：dev  ---  SPIx（x=0至5）
 *           dma --- 指向保存dma信息的dma_xfer_t结构的指针
 * 返 回 值：无
 */
void gd32_spi_fill_txdma(uint32_t dev, dma_xfer_t *dma) {
    dma->dmac    = get_dmac(dev);
    dma->periph  = get_txdma_periph(dev);
    dma->channel = get_txdma_ch(dev);
    dma->paddr   = (uint32_t)(&SPI_DATA(dev)); /* 外设地址：SPIx->DATA 地址 */
    dma->width   = 1;                          /* 传输宽度为1字节 */
    dma->minc    = 1;                          /* 内存指针增加 */
    dma->pinc    = 0;                          /* 外设指针不增加 */
    dma->dir     = DMA_MEMORY_TO_PERIPH;       /* 传输方向：内存到外设 */
    dma->flow    = DMA_FLOW_CONTROLLER_DMA;    /* 流控制器：DMAC */
}

/*
 * 功能描述：此函数用于填充SPIx接收的DMA信息
 * 入参说明：dev  ---  SPIx（x=0至5）
 *           dma --- 指向保存dma信息的dma_xfer_t结构的指针
 * 返 回 值：无
 */
void gd32_spi_fill_rxdma(uint32_t dev, dma_xfer_t *dma) {
    dma->dmac    = get_dmac(dev);
    dma->periph  = get_rxdma_periph(dev);
    dma->channel = get_rxdma_ch(dev);
    dma->paddr   = (uint32_t)(&SPI_DATA(dev)); /* 外设地址：UARTx->DATA地址 */
    dma->width   = 1;                          /* 传输宽度为1字节 */
    dma->minc    = 1;                          /* 内存指针增加 */
    dma->pinc    = 0;                          /* 外设指针不增加 */
    dma->dir     = DMA_PERIPH_TO_MEMORY;       /* 传输方向：外设到内存 */
    dma->flow    = DMA_FLOW_CONTROLLER_DMA;    /* 流控制器：DMAC */
}
