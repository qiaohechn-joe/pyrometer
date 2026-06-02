/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：serial.h
 * 文件描述：通用串口管理（GD32F4x）
 * 作    者：和其光电嵌软团队
 * 备    注：COM0-COM7 <---> UART0-UART7
 * 更新记录：
 *           V1.0 2024/01/26 陈  军  初始版本
 *           V1.1 2024/05/22 陈  军  半双工控制函数加入宏开关避免警告
 *           V1.2 2024/12/04 陈  军  整理串口框架
 *           V1.3 2024/12/11 李兆越  串口配置优化
 *           V1.4 2025/03/11 陈  军  串口半双工和全双工模式集成，增加波特率配置
 */

#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "platform.h"

#define COM_SEND_DMA        (1u << 0) /* DMA发送 */
#define COM_RECV_DMA        (1u << 1) /* DMA接收 */

#define ERR_SUCCESS         0
#define ERR_LENGTH_MISMATCH -1
#define ERR_INVALID_PORT    -2
#define ERR_INVALID_LENGTH  -3

typedef enum {
    COM_MODE_TX    = 1, // 发送模式,用于半双工
    COM_MODE_RX    = 2, // 接收模式,用于半双工
    COM_MODE_RX_TX = 3  // 接收发送模式,用于全双工
} rt_mode_e;

typedef struct {
    uint32_t re_port;
    uint32_t re_pin;
    uint32_t de_port;
    uint32_t de_pin;
} com_gpio_t;

typedef struct {
    uint32_t       baudrate;
    const uint32_t rx_timeo;
    const char    *fmt;
    void (*rx_callback)(void *arg); /* 接收回调 */
    const uint8_t   port;
    const uint8_t   dma_cfg;
    const rt_mode_e mode;
} com_config_t;

typedef struct {
    uint8_t  dma_tx_circular;
    uint8_t  dma_rx_circular;
    uint32_t dma_tx_priority;     // 发送DMA中断优先级
    uint32_t dma_rx_priority;     // 接收DMA中断优先级
    void (*dma_tx_handler)(void); // 发送DMA中断处理函数
    void (*dma_rx_handler)(void); // 接收DMA中断处理函数
    IRQn_Type uart_irqn;          // uart中断号
    void (*uart_handler)(void);   // 串口中断处理函数
    void (*timer_handler)(void);  // 定时器中断处理函数
} com_config_tmp_t;

/* 环形缓冲区 */
typedef struct {
    uint8_t *buffer; // 缓冲区指针
    uint16_t write_idx;
    uint16_t read_idx;
    uint16_t count; /* 待发送字节数 */ // todo 待修改类型
    uint16_t capacity;                 /* 容量 */
} circ_buffer_t;

/* COM描述符 */
typedef struct {
    uint32_t      uart;                          /* 外设 */
    uint32_t      timer;                         /* 接收超时定时器 */
    uint32_t      mode;                          /* rs485 2/4-line mode */
    circ_buffer_t tx_fifo;                       /* 发送FIFO（环形缓冲区） */
    dma_xfer_t    tx_dma;                        /* 发送DMA传输信息 */
    circ_buffer_t rx_fifo;                       /* 接收FIFO（环形缓冲区） */
    dma_xfer_t    rx_dma;                        /* 接收DMA传输信息 */
    uint16_t      rx_len; /* 接收长度（字节） */ // todo  ,考虑删除
    void (*rx_callback)(void *arg);              /* 接收回调 */
    uint8_t dma_cfg;
    uint8_t port;
} com_desc_t;

void com_init(com_config_t *com_cfg);
int  is_baud_rate_supported(uint32_t rate);
void com_clr_tx_fifo(int port);
void com_clr_rx_fifo(int port);
void com_enable_recv(int port);
void com_disable_recv(int port);
int  com_filter_recv(int port, int min_len, int max_len);
int  com_put(int port, const void *buff, int len);
int  com_get(int port, void *buff, int size, uint32_t timeo);
int  com_send(int port, const void *buff, int len);
int  com_recv(int port, char *buff, int size);

#endif /* _SERIAL_H_ */
