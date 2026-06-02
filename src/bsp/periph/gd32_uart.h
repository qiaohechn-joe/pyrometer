/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_uart.h
 * 文件描述：UART-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/12/05 陈军  简化串口DMA配置
 */

#ifndef _GD32_UART_H_
#define _GD32_UART_H_

#include "gd32f4xx.h"
#include "gd32_dma.h"

/* UART2发送DMA通道：3或4 */
#define UART2_TXDMA_CHANNEL        3

/* UART5发送DMA通道：6或7 */
#define UART5_TXDMA_CHANNEL        6

/* UART0接收DMA通道：2或5 */
#define UART0_RXDMA_CHANNEL        2

/* UART5接收DMA通道：1或2 */
#define UART5_RXDMA_CHANNEL        1

/* 发送、接收、中断 */
#define gd32_uart_tx(dev, ch)      usart_data_transmit(dev, ch)
#define gd32_uart_tx_rdy(dev)      (usart_flag_get(dev, USART_FLAG_TBE) == SET)
#define gd32_uart_tx_empty(dev)    (usart_flag_get(dev, USART_FLAG_TC) == SET)

#define gd32_uart_rx(dev)          usart_data_receive(dev)
#define gd32_uart_rx_rdy(dev)      (usart_flag_get(dev, USART_FLAG_RBNE) == SET)
#define gd32_uart_rx_idle(dev)     (usart_flag_get(dev, USART_FLAG_IDLE) == SET)

#define gd32_uart_overrun_err(dev) (usart_flag_get(dev, USART_FLAG_ORERR) == SET)
#define gd32_uart_noise_err(dev)   (usart_flag_get(dev, USART_FLAG_NERR) == SET)
#define gd32_uart_frame_err(dev)   (usart_flag_get(dev, USART_FLAG_FERR) == SET)
#define gd32_uart_par_err(dev)     (usart_flag_get(dev, USART_FLAG_PERR) == SET)

void gd32_uart_init(uint32_t dev, uint32_t rate, const char *format);
void gd32_uart_shutdown(uint32_t dev);
void gd32_uart_putch(uint32_t dev, int ch);
void gd32_uart_put(uint32_t dev, const uint8_t *data, int len);
void gd32_uart_puts(uint32_t dev, const char *str);
int  gd32_uart_getch(uint32_t dev);
int  gd32_uart_getch_timeout(uint32_t dev, int *ch, uint32_t timeo);
void gd32_uart_fill_txdma(uint32_t dev, dma_xfer_t *xfer);
void gd32_uart_fill_rxdma(uint32_t dev, dma_xfer_t *xfer);

#endif /* _GD32_UART_H_ */
