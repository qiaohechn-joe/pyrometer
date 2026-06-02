/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：scom.h
 * 文件描述：通用SPI通信管理（GD32F4x）
 * 作    者：和其光电嵌软团队
 * 备    注：SPI0~SPI5
 * 更新记录：
 *           V1.0 2024/12/05 陈军  初始版本
 */

#ifndef _SCOM_H_
#define _SCOM_H_

#include "platform.h"

#define SCOM_SEND_DMA (1u << 0) /* DMA发送 */
#define SCOM_RECV_DMA (1u << 1) /* DMA接收 */

typedef struct {
    uint32_t spi_mode; /* SPI模式选择（SPI_MASTER/SPI_SLAVE） */
    int      rate;     /* log2(时钟分频)，1至8 */
    int      clk_mode; /* 时钟模式，CPOL和CPHA的组合，从0b00到0b11 */
    int      word;     /* 字长，8位或16位 */
    uint32_t rx_timeo; /* 超时 */
    int      flag;     /* 标志的组合（SCOM_SEND_DMA, SCOM_RECV_DMA等） */
} scom_para_t;

void scom_init(int port, scom_para_t *scom_para);
void scom_clr_tx_fifo(int port);
void scom_clr_rx_fifo(int port);
void scom_enable_recv(int port);
void scom_disable_recv(int port);
void scom_set_recv(int port, void (*callback)(void *arg));
int  scom_filter_recv(int port, int min_len, int max_len);
int  scom_put(int port, const void *buff, int len);
int  scom_get(int port, void *buff, int size);
int  scom_send(int port, const void *buff, int len);
int  scom_recv(int port, void *buff, int size);

#endif /* _SCOM_H_ */
