/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_spi.h
 * 文件描述：SPI-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/11/22 陈军  增加SPI模式
 *           V1.2 2024/11/25 陈军  增加SPI发送和接收DMA信息填充函数
 */

#ifndef _GD32_SPI_H_
#define _GD32_SPI_H_

#include "gd32f4xx.h"
#include "gd32_dma.h"

#define SPI_MAX_COUNT          6 /* GD32F4 SPI数量 */

#define get_spi_index(dev)     ((dev == SPI0) ? 0 : (dev == SPI1) ? 1 : (dev == SPI2) ? 2 : (dev == SPI3) ? 3 : (dev == SPI4) ? 4 : 5)
#define gd32_spi_busy(dev)     (spi_i2s_flag_get(dev, SPI_FLAG_TRANS) == SET)
#define gd32_spi_overrun(dev)  (SPI_STAT(dev) & SPI_STAT_RXORERR)
#define gd32_spi_tx(dev, data) spi_i2s_data_transmit(dev, data)
#define gd32_spi_tx_rdy(dev)   (spi_i2s_flag_get(dev, SPI_FLAG_TBE) == SET)
#define gd32_spi_rx(dev)       spi_i2s_data_receive(dev)
#define gd32_spi_rx_rdy(dev)   (spi_i2s_flag_get(dev, SPI_FLAG_RBNE) == SET)

void gd32_spi_init(uint32_t dev, uint32_t spi_mode, int log2div, int clock_mode, int word);
void gd32_spi_shutdown(uint32_t dev);
int  gd32_spi_xfer(uint32_t dev, int data);
void gd32_spi_fill_txdma(uint32_t dev, dma_xfer_t *dma);
void gd32_spi_fill_rxdma(uint32_t dev, dma_xfer_t *dma);

#endif /* _GD32_SPI_H_ */
