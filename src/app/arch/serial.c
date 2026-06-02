/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：serial.c
 * 文件描述：通用串口管理（GD32F4x）
 * 作    者：和其光电嵌软团队
 * 备    注：COM0-COM6 <---> UART0-UART7
 * 更新记录：
 *           V1.0 2024/01/26 陈  军  初始版本
 *           V1.1 2024/05/22 陈  军  半双工控制函数加入宏开关避免警告
 *           V1.2 2024/12/04 陈  军  整理串口框架
 *           V1.3 2024/12/11 李兆越  串口配置优化
 *           V1.4 2025/03/11 陈  军  串口半双工和全双工模式集成，增加波特率配置
 */

#include "serial.h"

static void on_dma_tx(com_desc_t *desc);
static void on_dma_rx(com_desc_t *desc);
static void on_irq_all(com_desc_t *desc);
static void on_rx_over(com_desc_t *desc);

static void init_rx_timer(uint32_t timer, uint32_t timeo, isr_handler_t handler);

/* COM描述符数组 */
static com_desc_t com_desc[COM_CNT] = {0};

// 定义一个数组来存储每个串口的GPIO信息
static const com_gpio_t com_gpios[] = {
    //[COM1] = {COM1_RE_PORT, COM1_RE_PIN, COM1_DE_PORT, COM1_DE_PIN}, // COM1
    [COM2] = {COM2_RE_PORT, COM2_RE_PIN, COM2_DE_PORT, COM2_DE_PIN}, // COM2
    //[COM3] = {COM3_RE_PORT, COM3_RE_PIN, COM3_DE_PORT, COM3_DE_PIN}, // COM3
    //[COM4] = {COM4_RE_PORT, COM4_RE_PIN, COM4_DE_PORT, COM4_DE_PIN}, // COM4
    //[COM5] = {COM5_RE_PORT, COM5_RE_PIN, COM5_DE_PORT, COM5_DE_PIN}, // COM5
    // 可以继续添加更多串口
};

/* COM0 中断服务函数和缓冲区 */
#if defined(COM0)
static uint8_t tx_buff0[COM0_TX_BUFF_SIZE];
static uint8_t rx_buff0[COM0_RX_BUFF_SIZE];

static void com0_tx_dma_handler(void) {
    on_dma_tx(&com_desc[COM0]);
}

static void com0_rx_dma_handler(void) {
    on_dma_rx(&com_desc[COM0]);
}

static void com0_uart_handler(void) {
    on_irq_all(&com_desc[COM0]);
}

static void com0_tim_handler(void) {
    on_rx_over(&com_desc[COM0]);
}
#endif

/* COM1 中断服务函数和缓冲区 */
#if defined(COM1)
static uint8_t tx_buff1[COM1_TX_BUFF_SIZE];
static uint8_t rx_buff1[COM1_RX_BUFF_SIZE];

static void com1_tx_dma_handler(void) {
    on_dma_tx(&com_desc[COM1]);
}

static void com1_rx_dma_handler(void) {
    on_dma_rx(&com_desc[COM1]);
}

static void com1_uart_handler(void) {
    on_irq_all(&com_desc[COM1]);
}

static void com1_tim_handler(void) {
    on_rx_over(&com_desc[COM1]);
}
#endif

/* COM2 中断服务函数和缓冲区 */
#if defined(COM2)
static uint8_t tx_buff2[COM2_TX_BUFF_SIZE];
static uint8_t rx_buff2[COM2_RX_BUFF_SIZE];

static void com2_tx_dma_handler(void) {
    on_dma_tx(&com_desc[COM2]);
}

static void com2_rx_dma_handler(void) {
    on_dma_rx(&com_desc[COM2]);
}

static void com2_uart_handler(void) {
    on_irq_all(&com_desc[COM2]);
}

static void com2_tim_handler(void) {
    on_rx_over(&com_desc[COM2]);
}
#endif

/* COM3 中断服务函数和缓冲区 */
#if defined(COM3)
static uint8_t tx_buff3[COM3_TX_BUFF_SIZE];
static uint8_t rx_buff3[COM3_RX_BUFF_SIZE];

static void com3_tx_dma_handler(void) {
    on_dma_tx(&com_desc[COM3]);
}

static void com3_rx_dma_handler(void) {
    on_dma_rx(&com_desc[COM3]);
}

static void com3_uart_handler(void) {
    on_irq_all(&com_desc[COM3]);
}

static void com3_tim_handler(void) {
    on_rx_over(&com_desc[COM3]);
}
#endif

/* COM4 中断服务函数和缓冲区 */
#if defined(COM4)
static uint8_t tx_buff4[COM4_TX_BUFF_SIZE];
static uint8_t rx_buff4[COM4_RX_BUFF_SIZE];

static void com4_tx_dma_handler(void) {
    on_dma_tx(&com_desc[COM4]);
}

static void com4_rx_dma_handler(void) {
    on_dma_rx(&com_desc[COM4]);
}

static void com4_uart_handler(void) {
    on_irq_all(&com_desc[COM4]);
}

static void com4_tim_handler(void) {
    on_rx_over(&com_desc[COM4]);
}
#endif

/* COM5 中断服务函数和缓冲区 */
#if defined(COM5)
static uint8_t tx_buff5[COM5_TX_BUFF_SIZE];
static uint8_t rx_buff5[COM5_RX_BUFF_SIZE];

static void com5_tx_dma_handler(void) {
    on_dma_tx(&com_desc[COM5]);
}

static void com5_rx_dma_handler(void) {
    on_dma_rx(&com_desc[COM5]);
}

static void com5_uart_handler(void) {
    on_irq_all(&com_desc[COM5]);
}

static void com5_tim_handler(void) {
    on_rx_over(&com_desc[COM5]);
}
#endif

/* COM6 中断服务函数和缓冲区 */
#if defined(COM6)
static uint8_t tx_buff6[COM6_TX_BUFF_SIZE];
static uint8_t rx_buff6[COM6_RX_BUFF_SIZE];

static void com6_tx_dma_handler(void) {
    on_dma_tx(&com_desc[COM6]);
}

static void com6_rx_dma_handler(void) {
    on_dma_rx(&com_desc[COM6]);
}

static void com6_uart_handler(void) {
    on_irq_all(&com_desc[COM6]);
}

static void com6_tim_handler(void) {
    on_rx_over(&com_desc[COM6]);
}
#endif

/* COM7 中断服务函数和缓冲区 */
#if defined(COM7)
static uint8_t tx_buff7[COM7_TX_BUFF_SIZE];
static uint8_t rx_buff7[COM7_RX_BUFF_SIZE];

static void com7_tx_dma_handler(void) {
    on_dma_tx(&com_desc[COM7]);
}

static void com7_rx_dma_handler(void) {
    on_dma_rx(&com_desc[COM7]);
}

static void com7_uart_handler(void) {
    on_irq_all(&com_desc[COM7]);
}

static void com7_tim_handler(void) {
    on_rx_over(&com_desc[COM7]);
}
#endif

/*
 * 功能描述：校验串口波特率是否支持
 * 入参说明：baudrate --- 待检查串口波特率
 * 返 回 值：true --- 支持；false --- 不支持
 */
bool is_valid_baudrate(int baudrate) {
    const int valid_rates[] = {2400, 4800, 9600, 14400, 19200, 38400, 56000, 57600, 115200, 128000, 230400};
    const int rate_count    = sizeof(valid_rates) / sizeof(valid_rates[0]);

    for (int i = 0; i < rate_count; i++) {
        if (baudrate == valid_rates[i]) {
            return true;
        }
    }
    return false;
}

/*
 * 功能描述：这个函数控制串口5的模式
 * 入参说明：mode --- 串口模式，取值为 rt_mode_e 枚举类型
 * 返 回 值：无
 */
static void com_set_mode(const uint8_t com, rt_mode_e mode) {
    if (mode == COM_MODE_RX) {
        gpio_bit_reset(com_gpios[com].re_port, com_gpios[com].re_pin);
        gpio_bit_reset(com_gpios[com].de_port, com_gpios[com].de_pin);
    } else if (mode == COM_MODE_TX) {
        gpio_bit_set(com_gpios[com].re_port, com_gpios[com].re_pin);
        gpio_bit_set(com_gpios[com].de_port, com_gpios[com].de_pin);
    } else if (mode == COM_MODE_RX_TX) {
        gpio_bit_reset(com_gpios[com].re_port, com_gpios[com].re_pin);
        gpio_bit_set(com_gpios[com].de_port, com_gpios[com].de_pin);
    } else {
        // error
    }
    return;
}

/*
 * 功能描述：此函数清除发送（Tx）FIFO
 * 入参说明：port --- 端口号，COMx(x=1至8)
 * 返 回 值：无
 */
void com_clr_tx_fifo(int port) {
    com_desc_t    *desc;
    circ_buffer_t *fifo;

    if ((port < 0) || (port >= COM_CNT)) {
        return;
    }

    desc = &com_desc[port];
    fifo = &desc->tx_fifo;

    fifo->count    = 0;
    fifo->read_idx = fifo->write_idx = 0;
}

/*
 * 功能描述：此函数清除接收（Rx）FIFO
 * 入参说明：port --- 端口号，COMx(x=1至8)
 * 返 回 值：无
 */
void com_clr_rx_fifo(int port) {
    com_desc_t    *desc;
    circ_buffer_t *fifo;

    if ((port < 0) || (port >= COM_CNT)) {
        return;
    }

    desc = &com_desc[port];
    fifo = &desc->rx_fifo;

    fifo->count    = 0;
    fifo->read_idx = fifo->write_idx = 0;
}

/*
 * 功能描述：此函数启用通过FIFO接收数据
 * 入参说明：port --- 端口号，COMx(x=1至8)
 * 返 回 值：无
 */
void com_enable_recv(int port) {
    com_desc_t    *desc;
    circ_buffer_t *fifo;
    dma_xfer_t    *dma;
    int            temp;

    if ((port < 0) || (port >= COM_CNT)) {
        return;
    }

    desc = &com_desc[port];
    fifo = &desc->rx_fifo;
    dma  = &desc->rx_dma;

    /* 清除接收错误标志 */
    temp = USART_STAT0(desc->uart);
    temp = USART_DATA(desc->uart);
    (void)temp;

    if (desc->dma_cfg & COM_RECV_DMA) { /* DMA */
        /* 开始DMA传输 */
        gd32_dma_rx(dma, fifo->buffer + fifo->write_idx, fifo->capacity);
        enable_dma_irq(dma);
        usart_interrupt_enable(desc->uart, USART_INT_IDLE);
    } else { /* IRQ */
        usart_interrupt_enable(desc->uart, USART_INT_RBNE);
    }
}

/*
 * 功能描述：此函数禁用通过FIFO接收数据
 * 入参说明：port --- 端口号，COMx(x=1至8)
 * 返 回 值：无
 */
void com_disable_recv(int port) {
    com_desc_t *desc;
    dma_xfer_t *dma;

    if ((port < 0) || (port >= COM_CNT)) {
        return;
    }

    desc = &com_desc[port];
    dma  = &desc->rx_dma;

    if (desc->dma_cfg & COM_RECV_DMA) { /* DMA */
        disable_dma_irq(dma);
        dma_channel_disable(dma->dmac, dma->channel);
    } else { /* IRQ */
        usart_interrupt_disable(desc->uart, USART_INT_RBNE);
    }
}

/*
 * 功能描述：此函数过滤接收到的数据
 * 入参说明：port --- 端口号，COMx(x=1至6)
 *           min_len --- 最小接收字节数
 *           max_len --- 最大接收字节数
 * 返 回 值：0 = 成功   其他 = 失败
 * 备    注：由 rx_callback() 调用

 */
int com_filter_recv(int port, int min_len, int max_len) {
    if (port < 0 || port >= COM_CNT) {
        return ERR_INVALID_PORT;
    }

    if (min_len < 0 || max_len < min_len) {
        return ERR_INVALID_LENGTH;
    }

    com_desc_t *desc = &com_desc[port];
    if (desc->rx_len < min_len || desc->rx_len > max_len) { /* 清buffer */
        desc->rx_fifo.count -= desc->rx_len;
        desc->rx_fifo.read_idx += desc->rx_len;
        desc->rx_len = 0;
        return ERR_LENGTH_MISMATCH;
    }
    return 0;
}

/*
 * 功能描述：此函数将数据直接发送到串口
 * 入参说明：port --- 端口号，COMx(x = 1至8)
 *           buff --- 指向缓冲区的指针
 *           len --- 数据长度（字节）
 * 返 回 值：成功发送的数据字节数
 */
int com_put(int port, const void *buff, int len) {
    if ((port < 0) || (port >= COM_CNT)) {
        return 0;
    }

    com_desc_t *desc = &com_desc[port];

    if (desc->mode == COM_HALF_DUPLEX) { /* 半双工 */
        com_set_mode(port, COM_MODE_TX);
        gd32_uart_put(desc->uart, (const uint8_t *)buff, len);
        while (!gd32_uart_tx_empty(desc->uart)); /* 等待直到完成 */
        com_set_mode(port, COM_MODE_RX);

    } else { /* 全双工 */
        gd32_uart_put(desc->uart, (const uint8_t *)buff, len);
    }

    return len;
}

/*
 * 功能描述：此函数直接从串口接收数据
 * 入参说明：port --- 端口号，COMx(x=1至8)
 *           buff --- 指向缓冲区的指针
 *           size --- 缓冲区大小（字节）
 *           timeo --- 等待超时值（毫秒）
 * 返 回 值：接收到的数据字节数
 * 备    注：timeo 参数的精确度不高
 */
int com_get(int port, void *buff, int size, uint32_t timeo) {
    uint8_t *data = (uint8_t *)buff;
    int      ch;

    if ((port < 0) || (port >= COM_CNT)) {
        return 0;
    }

    com_desc_t *desc = &com_desc[port];

    while (1) {
        if (gd32_uart_getch_timeout(desc->uart, &ch, timeo)) {
            break; /* 超时 */
        }
        *data++ = (uint8_t)ch;
        size--;
        if (size == 0) {
            break;
        }
    }

    return data - (uint8_t *)buff;
}

/*
 * 功能描述：此函数通过FIFO向串口发送数据
 * 入参说明：port --- 端口号，COMx(x=1至8)
 *           buff --- 指向缓冲区的指针
 *           len --- 数据长度（字节）
 * 返 回 值：成功发送的数据字节数
 * 备    注：非重入函数。必须通过临界区（CRITICAL-SECTION）或互斥锁（mutex）保护
 */
int com_send(int port, const void *buff, int len) {
    if ((port < 0) || (port >= COM_CNT)) {
        return 0;
    }
    com_desc_t    *desc       = &com_desc[port];
    circ_buffer_t *fifo       = &desc->tx_fifo;
    const uint8_t *data       = (const uint8_t *)buff;
    dma_xfer_t    *dma        = &desc->tx_dma;
    int            total_sent = 0; // 总发送长度

    while (len > 0) {
        int available_space = fifo->capacity - fifo->count;
        if (available_space <= 0) {
            return total_sent;
        }

        int send_len = (len < available_space) ? len : available_space;

        // 复制数据到环形缓冲区
        fifo->read_idx = fifo->write_idx;
        for (int i = 0; i < send_len; i++) {
            fifo->buffer[fifo->write_idx] = data[i];
            fifo->write_idx               = (fifo->write_idx + 1) % fifo->capacity;
        }

        fifo->count += send_len;

        if (fifo->count > 0) { // 有数据需要发送
            if (desc->mode == COM_HALF_DUPLEX) {
                com_set_mode(port, COM_MODE_TX); // 半双工：转换到发送模式
            }
            if (desc->dma_cfg & COM_SEND_DMA) { // 使用DMA传输
                // 启动DMA传输
                int size = fifo->capacity - fifo->read_idx; // 当前可读空间大小
                if (size > fifo->count) {
                    size = fifo->count;
                }
                usart_interrupt_flag_clear(desc->uart, USART_INT_FLAG_TC);
                usart_interrupt_enable(desc->uart, USART_INT_TC);
                enable_dma_irq(dma);
                gd32_dma_start(dma, fifo->buffer + fifo->read_idx, size);
            } else { // 使用中断传输
                usart_interrupt_enable(desc->uart, USART_INT_TBE);
            }
        }

        // 更新指针和长度
        data += send_len;
        len -= send_len;
        total_sent += send_len;
    }
    return total_sent;
}

/*
 * 功能描述：此函数从串口的接收（Rx）FIFO中接收数据
 * 入参说明：port --- 端口号，COMx(x=1至8)
 *           buff --- 指向缓冲区的指针
 *           size --- 缓冲区大小（字节）
 * 返 回 值：接收到的数据字节数
 * 备    注：在使用此函数之前，必须先调用一次 com_enable_recv()
 */
int com_recv(int port, char *buff, int size) {
    int rx_len;
    int rx_stop;

    if ((port < 0) || (port >= COM_CNT)) {
        return 0;
    }

    com_desc_t    *desc = &com_desc[port];
    circ_buffer_t *fifo = &desc->rx_fifo;

    rx_len = desc->rx_len;
    if (rx_len == 0) {
        return 0; /* 没有数据 */
    }

    /* 复制数据 */
    rx_stop = (rx_len >= fifo->capacity);
    if (rx_len > size) {
        rx_len = size;
    }

    taskENTER_CRITICAL();

    /* 复制数据 */
    rx_len = fifo->count;
    for (uint16_t i = 0; i < rx_len; i++) {
        buff[i]        = fifo->buffer[fifo->read_idx];
        fifo->read_idx = (fifo->read_idx + 1) % fifo->capacity;
    }

    /* 更新FIFO */
    fifo->count -= rx_len;
    desc->rx_len -= rx_len;
    taskEXIT_CRITICAL();

    /* 重新使能接收FIFO */
    if (rx_stop) {
        com_enable_recv(port);
    }

    return rx_len;
}

/*
 * 功能描述：此函数是DMA模式下的发送（TX）中断处理函数
 * 入参说明：desc --- 指向 com_descriptor 结构的指针
 * 返 回 值：无
 * 备    注：由 DMA-TX 中断服务函数调用
 */
static void on_dma_tx(com_desc_t *desc) {
    dma_xfer_t    *dma  = &desc->tx_dma;
    circ_buffer_t *fifo = &desc->tx_fifo;
    int            sent_len;
    UBaseType_t    isr_status;

    if (dma_flag_valid(dma)) { /* 传输完成或错误 */
        clear_dma_flag(dma);   /* 清除所有标志 */

        isr_status = taskENTER_CRITICAL_FROM_ISR();

        /* 发送完成大小 */
        dma_channel_disable(dma->dmac, dma->channel);
        sent_len = (dma->total_cnt - dma_transfer_number_get(dma->dmac, dma->channel)) * dma->width;

        fifo->read_idx = (fifo->read_idx + sent_len) % fifo->capacity;
        fifo->count -= sent_len;

        if (fifo->count == 0) { /* 缓存区空 */
            /* 发送完成 */
            disable_dma_irq(dma);
        } else {
            // 启动DMA传输
            int size = min(fifo->count, fifo->capacity - fifo->read_idx);
            if (desc->mode == COM_HALF_DUPLEX) {
                com_set_mode(desc->port, COM_MODE_TX); // 半双工：转换到发送模式
            }
            enable_dma_irq(dma);
            gd32_dma_start(dma, fifo->buffer + fifo->read_idx, size);
        }

        taskEXIT_CRITICAL_FROM_ISR(isr_status);
    }
}

/*
 * 功能描述：此函数是DMA模式下的接收（RX）处理函数
 * 入参说明：desc --- 指向 com_descriptor 结构的指针
 * 返 回 值：无
 * 备    注：由 DMA-RX 中断服务函数调用
 */
static void on_dma_rx(com_desc_t *desc) {
    dma_xfer_t *dma = &desc->rx_dma;
    // circ_buffer_t *fifo = &desc->rx_fifo;
    // UBaseType_t    isr_status;

    if (dma_flag_get(dma->dmac, dma->channel, DMA_INTF_FEEIF | DMA_INTF_FTFIF | DMA_INTF_TAEIF | DMA_INTF_SDEIF) == SET) { /* 传输完成或错误 */
        clear_dma_flag(dma);                                                                                               /* 清所有标志 */
    }
}

/*
 * 功能描述：此函数是DMA模式下的接收（RX）处理函数
 * 入参说明：desc --- 指向 com_descriptor 结构的指针
 * 返 回 值：无
 * 备    注：由 DMA-RX 中断服务函数调用
 */
static void on_dma_rx_backup(com_desc_t *desc) {
    dma_xfer_t    *dma  = &desc->rx_dma;
    circ_buffer_t *fifo = &desc->rx_fifo;
    UBaseType_t    isr_status;

    if (dma_flag_valid(dma)) { /* 传输完成或错误 */
        clear_dma_flag(dma);   /* 清所有标志 */

        isr_status = taskENTER_CRITICAL_FROM_ISR();

        /* 接收完成大小 */
        int size       = (dma->total_cnt + fifo->capacity - dma_transfer_number_get(dma->dmac, dma->channel)) % fifo->capacity * dma->width;
        dma->total_cnt = (dma->total_cnt + fifo->capacity - size) % fifo->capacity;
        // dma_channel_disable(dma->dmac, dma->channel);

        fifo->write_idx = (fifo->write_idx + size) % fifo->capacity;
        fifo->count += size;

        if (fifo->count >= fifo->capacity) { /* FIFO满 */
            disable_dma_irq(dma);
            desc->rx_len = fifo->count;
            if (desc->rx_callback) {
                desc->rx_callback((void *)&desc->rx_len);
            }
        } else {
            /* 清接收错误标志 */
            size = USART_STAT0(desc->uart);
            size = USART_DATA(desc->uart);
            size = fifo->capacity - fifo->count; /* 剩余空间 */
                                                 //            if (size > (fifo->end - fifo->tail)) {
                                                 //                size = fifo->end - fifo->tail; /* [tail,end) */
                                                 //            }
                                                 //            if (size > COM_DMA_XFER_SIZE) {
                                                 //                size = COM_DMA_XFER_SIZE;
                                                 //            }

            gd32_dma_start(dma, fifo->buffer + fifo->write_idx, size);

            /* 启动超时定时器 */
            timer_enable(desc->timer);
            timer_event_software_generate(desc->timer, TIMER_EVENT_SRC_UPG);
            timer_flag_clear(desc->timer, TIMER_FLAG_UP);
            timer_interrupt_enable(desc->timer, TIMER_INT_UP);
        }

        taskEXIT_CRITICAL_FROM_ISR(isr_status);
    }
}

/*
 * 功能描述：此函数是IRQ模式下通用处理函数
 * 入参说明：desc --- 指向 com_descriptor 结构的指针
 * 返 回 值：无
 * 备    注：由串口中断服务函数调用
 */
static void on_irq_all(com_desc_t *desc) {
    circ_buffer_t *fifo;
    int            temp;
    UBaseType_t    isr_status;

    isr_status = taskENTER_CRITICAL_FROM_ISR();

    /* 发送完成 */
    if ((USART_CTL0(desc->uart) & USART_CTL0_TCIE) && gd32_uart_tx_empty(desc->uart)) {
        usart_interrupt_disable(desc->uart, USART_INT_TC);
        if (desc->mode == COM_HALF_DUPLEX) {
            com_set_mode(desc->port, COM_MODE_RX); /* 转换到接收模式 */
        }
    }

    /* 接收完成 */
    if ((USART_CTL0(desc->uart) & USART_CTL0_IDLEIE) && gd32_uart_rx_idle(desc->uart)) {
        dma_xfer_t    *dma  = &desc->rx_dma;
        circ_buffer_t *fifo = &desc->rx_fifo;

        if (desc->dma_cfg & COM_RECV_DMA) { /* DMA接收 */
                                            // usart_interrupt_disable(desc->uart, USART_INT_IDLE);
            /* 完成的大小 */
            //            int size        = (dma->total_cnt + fifo->capacity - dma_transfer_number_get(dma->dmac, dma->channel)) % fifo->capacity *
            //            dma->width; dma->total_cnt  = (dma->total_cnt + fifo->capacity - size) % fifo->capacity;
            fifo->write_idx = fifo->capacity - dma_transfer_number_get(dma->dmac, dma->channel);
            fifo->count     = (fifo->capacity + fifo->write_idx - fifo->read_idx) % fifo->capacity;

            /* 调用rx_callback() */
            desc->rx_len = fifo->count;
            if (desc->rx_callback) {
                desc->rx_callback((void *)&desc->rx_len);
            }

            int size = USART_STAT0(desc->uart); /* 清除接收错误标志 */
            size     = USART_DATA(desc->uart);
            // gd32_dma_start(dma, fifo->tail, 1); /* 启动下次接收 */
        } else {
            /* 调用rx_callback() */
            desc->rx_len = fifo->count;
            if (desc->rx_callback) {
                desc->rx_callback((void *)&desc->rx_len);
            }
        }
    }

    if (USART_CTL0(desc->uart) & USART_CTL0_RBNEIE) { /* 接收准备好且中断使能 */
        if (gd32_uart_overrun_err(desc->uart)) {      /* 接收溢出 */
            temp = gd32_uart_rx(desc->uart);          /* 清标志 */
            (void)temp;
        }

        if (gd32_uart_rx_rdy(desc->uart)) { /* 接收准备好 */
            fifo = &desc->rx_fifo;

            /* 接收字符 */
            *(fifo->buffer + fifo->write_idx) = gd32_uart_rx(desc->uart);
            fifo->write_idx                   = (fifo->write_idx + 1) % fifo->capacity;
            fifo->count++;

            if (fifo->count == fifo->capacity) { /* FIFO满 */
                usart_interrupt_disable(desc->uart, USART_INT_RBNE);

                desc->rx_len = fifo->count;
                if (desc->rx_callback) {
                    desc->rx_callback((void *)&desc->rx_len);
                }
            } else {
                /* 启动超时定时器 */
                timer_enable(desc->timer);
                timer_event_software_generate(desc->timer, TIMER_EVENT_SRC_UPG);
                timer_flag_clear(desc->timer, TIMER_FLAG_UP);
                timer_interrupt_enable(desc->timer, TIMER_INT_UP);
            }
        }
    }

    if ((USART_CTL0(desc->uart) & USART_CTL0_TBEIE) && gd32_uart_tx_rdy(desc->uart)) { /* 发送 */
        fifo = &desc->tx_fifo;

        /* 发送字符 */
        gd32_uart_tx(desc->uart, *(fifo->buffer + fifo->read_idx));
        fifo->read_idx = (fifo->read_idx + 1) % fifo->capacity;
        fifo->count--;

        if (fifo->count == 0) { /* 缓存区空 */
            /* 发送完成 */
            usart_interrupt_disable(desc->uart, USART_INT_TBE);

            if (desc->mode == COM_HALF_DUPLEX) {
                /* 半双工：处理 tx_over */
                usart_interrupt_flag_clear(desc->uart, USART_INT_FLAG_TC);
                usart_interrupt_enable(desc->uart, USART_INT_TC);
            }
        }
    }
    taskEXIT_CRITICAL_FROM_ISR(isr_status);
}

/*
 * 功能描述：这个函数是接收完成处理函数
 * 入参说明：desc --- 指向com_descriptor结构的指针
 * 返 回 值：无
 * 备    注：由接收超时定时器中断服务程序调用
 */
static void on_rx_over(com_desc_t *desc) {
    dma_xfer_t    *dma  = &desc->rx_dma;
    circ_buffer_t *fifo = &desc->rx_fifo;
    UBaseType_t    isr_status;

    if (timer_flag_get(desc->timer, TIMER_FLAG_UP) == SET) {
        timer_flag_clear(desc->timer, TIMER_FLAG_UP);
        timer_interrupt_disable(desc->timer, TIMER_INT_UP); /* 失能中断 */
        timer_disable(desc->timer);

        isr_status = taskENTER_CRITICAL_FROM_ISR();

        if (desc->dma_cfg & COM_RECV_DMA) { /* DMA接收 */
            /* 完成的大小 */
            int size       = (dma->total_cnt + fifo->capacity - dma_transfer_number_get(dma->dmac, dma->channel)) % fifo->capacity * dma->width;
            dma->total_cnt = (dma->total_cnt + fifo->capacity - size) % fifo->capacity;
            // dma_channel_disable(dma->dmac, dma->channel);

            fifo->write_idx = (fifo->write_idx + size) % fifo->capacity;
            fifo->count += size;

            /* 调用rx_callback() */
            desc->rx_len = fifo->count;
            if (desc->rx_callback) {
                desc->rx_callback((void *)&desc->rx_len);
            }

            size = USART_STAT0(desc->uart); /* 清除接收错误标志 */
            size = USART_DATA(desc->uart);
            gd32_dma_start(dma, fifo->buffer + fifo->write_idx, 1); /* 启动下次接收 */
        } else {
            /* 调用rx_callback() */
            desc->rx_len = fifo->count;
            if (desc->rx_callback) {
                desc->rx_callback((void *)&desc->rx_len);
            }
        }

        taskEXIT_CRITICAL_FROM_ISR(isr_status);
    }
}

/*
 * 功能描述：此函数初始化接收超时定时器
 * 入参说明：tim --- 接收超时定时器外设
 *           timeo --- 接收超时时间，以毫秒为单位
 *           handler --- 定时器中断服务程序
 * 返 回 值：无
 */
static void init_rx_timer(uint32_t timer, uint32_t timeo, isr_handler_t handler) {
    int scale;

    /* 时钟周期100us */
    scale = gd32_timer_clk_freq(timer) / 10000;
    timeo = timeo * 10;

    gd32_timer_init(timer, scale, timeo);

    if (timer == TIMER0) {
        gd32_setup_isr(TIMER0_UP_TIMER9_IRQn, handler, 0);
        gd32_setup_irq(TIMER0_UP_TIMER9_IRQn, 15);
    } else if (timer == TIMER1) {
        gd32_setup_isr(TIMER1_IRQn, handler, 0);
        gd32_setup_irq(TIMER1_IRQn, 15);
    } else if (timer == TIMER2) {
        gd32_setup_isr(TIMER2_IRQn, handler, 0);
        gd32_setup_irq(TIMER2_IRQn, 15);
    } else if (timer == TIMER3) {
        gd32_setup_isr(TIMER3_IRQn, handler, 0);
        gd32_setup_irq(TIMER3_IRQn, 15);
    } else if (timer == TIMER4) {
        gd32_setup_isr(TIMER4_IRQn, handler, 0);
        gd32_setup_irq(TIMER4_IRQn, 15);
    } else if (timer == TIMER5) {
        gd32_setup_isr(TIMER5_DAC_IRQn, handler, 0);
        gd32_setup_irq(TIMER5_DAC_IRQn, 15);
    } else if (timer == TIMER6) {
        gd32_setup_isr(TIMER6_IRQn, handler, 0);
        gd32_setup_irq(TIMER6_IRQn, 15);
    } else if (timer == TIMER7) {
        gd32_setup_isr(TIMER7_UP_TIMER12_IRQn, handler, 0);
        gd32_setup_irq(TIMER7_UP_TIMER12_IRQn, 15);
    } else if (timer == TIMER8) {
        gd32_setup_isr(TIMER0_BRK_TIMER8_IRQn, handler, 0);
        gd32_setup_irq(TIMER0_BRK_TIMER8_IRQn, 15);
    } else if (timer == TIMER9) {
        gd32_setup_isr(TIMER0_UP_TIMER9_IRQn, handler, 0);
        gd32_setup_irq(TIMER0_UP_TIMER9_IRQn, 15);
    } else if (timer == TIMER10) {
        gd32_setup_isr(TIMER0_TRG_CMT_TIMER10_IRQn, handler, 0);
        gd32_setup_irq(TIMER0_TRG_CMT_TIMER10_IRQn, 15);
    } else if (timer == TIMER11) {
        gd32_setup_isr(TIMER7_BRK_TIMER11_IRQn, handler, 0);
        gd32_setup_irq(TIMER7_BRK_TIMER11_IRQn, 15);
    } else if (timer == TIMER12) {
        gd32_setup_isr(TIMER7_UP_TIMER12_IRQn, handler, 0);
        gd32_setup_irq(TIMER7_UP_TIMER12_IRQn, 15);
    } else if (timer == TIMER13) {
        gd32_setup_isr(TIMER7_TRG_CMT_TIMER13_IRQn, handler, 0);
        gd32_setup_irq(TIMER7_TRG_CMT_TIMER13_IRQn, 15);
    }
}

/*
 * 功能描述：初始化指定的通信端口，配置其 UART、定时器、FIFO 缓冲区以及相关的中断和 DMA 处理函数。
 * 入参说明：desc - 指向通信端口描述符的指针，包含端口编号等信息；
 *           cfg_tmp - 指向通信端口配置临时变量的指针，用于存储配置过程中产生的临时数据。
 * 返 回 值：无
 */
void configure_com_port(com_desc_t *desc, com_config_tmp_t *cfg_tmp) {
    switch (desc->port) {
#if defined(COM0)
        case COM0:
            desc->uart               = USART0;
            desc->timer              = COM0_RX_TIMER;
            desc->tx_fifo.capacity   = sizeof(tx_buff0);
            desc->rx_fifo.capacity   = sizeof(rx_buff0);
            desc->tx_fifo.buffer     = &tx_buff0[0];
            desc->rx_fifo.buffer     = &rx_buff0[0];
            cfg_tmp->dma_tx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_rx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_tx_priority = DMA_PRIO_COM0_TX;
            cfg_tmp->dma_rx_priority = DMA_PRIO_COM0_RX;
            cfg_tmp->dma_tx_handler  = com0_tx_dma_handler;
            cfg_tmp->dma_rx_handler  = com0_rx_dma_handler;
            cfg_tmp->uart_irqn       = USART0_IRQn;
            cfg_tmp->uart_handler    = com0_uart_handler;
            cfg_tmp->timer_handler   = com0_tim_handler;
            com0_pin_init();
            break;
#endif

#if defined(COM1)
        case COM1:
            desc->uart               = USART1;
            desc->timer              = COM1_RX_TIMER;
            desc->tx_fifo.capacity   = sizeof(tx_buff1);
            desc->rx_fifo.capacity   = sizeof(rx_buff1);
            desc->tx_fifo.buffer     = &tx_buff1[0];
            desc->rx_fifo.buffer     = &rx_buff1[0];
            cfg_tmp->dma_tx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_rx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_tx_priority = DMA_PRIO_COM1_TX;
            cfg_tmp->dma_rx_priority = DMA_PRIO_COM1_RX;
            cfg_tmp->dma_tx_handler  = com1_tx_dma_handler;
            cfg_tmp->dma_rx_handler  = com1_rx_dma_handler;
            cfg_tmp->uart_irqn       = USART1_IRQn;
            cfg_tmp->uart_handler    = com1_uart_handler;
            cfg_tmp->timer_handler   = com1_tim_handler;
            com1_pin_init();
            break;
#endif

#if defined(COM2)
        case COM2:
            desc->uart               = USART2;
            desc->timer              = COM2_RX_TIMER;
            desc->tx_fifo.capacity   = sizeof(tx_buff2);
            desc->rx_fifo.capacity   = sizeof(rx_buff2);
            desc->tx_fifo.buffer     = &tx_buff2[0];
            desc->rx_fifo.buffer     = &rx_buff2[0];
            cfg_tmp->dma_tx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_rx_circular = DMA_CIRCULAR_MODE_ENABLE;
            cfg_tmp->dma_tx_priority = DMA_PRIO_COM2_TX;
            cfg_tmp->dma_rx_priority = DMA_PRIO_COM2_RX;
            cfg_tmp->dma_tx_handler  = com2_tx_dma_handler;
            cfg_tmp->dma_rx_handler  = com2_rx_dma_handler;
            cfg_tmp->uart_irqn       = USART2_IRQn;
            cfg_tmp->uart_handler    = com2_uart_handler;
            cfg_tmp->timer_handler   = com2_tim_handler;
            com2_pin_init();
            break;
#endif

#if defined(COM3)
        case COM3:
            desc->uart               = UART3;
            desc->timer              = COM3_RX_TIMER;
            desc->tx_fifo.capacity   = sizeof(tx_buff3);
            desc->rx_fifo.capacity   = sizeof(rx_buff3);
            desc->tx_fifo.buffer     = &tx_buff3[0];
            desc->rx_fifo.buffer     = &rx_buff3[0];
            cfg_tmp->dma_tx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_rx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_tx_priority = DMA_PRIO_COM3_TX;
            cfg_tmp->dma_rx_priority = DMA_PRIO_COM3_RX;
            cfg_tmp->dma_tx_handler  = com3_tx_dma_handler;
            cfg_tmp->dma_rx_handler  = com3_rx_dma_handler;
            cfg_tmp->uart_irqn       = UART3_IRQn;
            cfg_tmp->uart_handler    = com3_uart_handler;
            cfg_tmp->timer_handler   = com3_tim_handler;
            com3_pin_init();
            break;
#endif

#if defined(COM4)
        case COM4:
            desc->uart               = UART4;
            desc->timer              = COM4_RX_TIMER;
            desc->tx_fifo.capacity   = sizeof(tx_buff4);
            desc->rx_fifo.capacity   = sizeof(rx_buff4);
            desc->tx_fifo.buffer     = &tx_buff4[0];
            desc->rx_fifo.buffer     = &rx_buff4[0];
            cfg_tmp->dma_tx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_rx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_tx_priority = DMA_PRIO_COM4_TX;
            cfg_tmp->dma_rx_priority = DMA_PRIO_COM4_RX;
            cfg_tmp->dma_tx_handler  = com4_tx_dma_handler;
            cfg_tmp->dma_rx_handler  = com4_rx_dma_handler;
            cfg_tmp->uart_irqn       = UART4_IRQn;
            cfg_tmp->uart_handler    = com4_uart_handler;
            cfg_tmp->timer_handler   = com4_tim_handler;
            com4_pin_init();
            break;
#endif

#if defined(COM5)
        case COM5:
            desc->uart               = USART5;
            desc->timer              = COM5_RX_TIMER;
            desc->tx_fifo.capacity   = sizeof(tx_buff5);
            desc->rx_fifo.capacity   = sizeof(rx_buff5);
            desc->tx_fifo.buffer     = &tx_buff5[0];
            desc->rx_fifo.buffer     = &rx_buff5[0];
            cfg_tmp->dma_tx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_rx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_tx_priority = DMA_PRIO_COM5_TX;
            cfg_tmp->dma_rx_priority = DMA_PRIO_COM5_RX;
            cfg_tmp->dma_tx_handler  = com5_tx_dma_handler;
            cfg_tmp->dma_rx_handler  = com5_rx_dma_handler;
            cfg_tmp->uart_irqn       = USART5_IRQn;
            cfg_tmp->uart_handler    = com5_uart_handler;
            cfg_tmp->timer_handler   = com5_tim_handler;
            com5_pin_init();
            break;
#endif

#if defined(COM6)
        case COM6:
            desc->uart               = UART6;
            desc->timer              = COM6_RX_TIMER;
            desc->tx_fifo.capacity   = sizeof(tx_buff6);
            desc->rx_fifo.capacity   = sizeof(rx_buff6);
            desc->tx_fifo.buffer     = &tx_buff6[0];
            desc->rx_fifo.buffer     = &rx_buff6[0];
            cfg_tmp->dma_tx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dam_rx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_tx_priority = DMA_PRIO_COM6_TX;
            cfg_tmp->dma_rx_priority = DMA_PRIO_COM6_RX;
            cfg_tmp->dma_tx_handler  = com6_tx_dma_handler;
            cfg_tmp->dma_rx_handler  = com6_rx_dma_handler;
            cfg_tmp->uart_irqn       = UART6_IRQn;
            cfg_tmp->uart_handler    = com6_uart_handler;
            cfg_tmp->timer_handler   = com6_tim_handler;
            com6_pin_init();
            break;
#endif

#if defined(COM7)
        case COM7:
            desc->uart               = UART7;
            desc->timer              = COM7_RX_TIMER;
            desc->tx_fifo.capacity   = sizeof(tx_buff7);
            desc->rx_fifo.capacity   = sizeof(rx_buff7);
            desc->tx_fifo.buffer     = &tx_buff7[0];
            desc->rx_fifo.buffer     = &rx_buff7[0];
            cfg_tmp->dma_tx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_rx_circular = DMA_CIRCULAR_MODE_DISABLE;
            cfg_tmp->dma_tx_priority = DMA_PRIO_COM7_TX;
            cfg_tmp->dma_rx_priority = DMA_PRIO_COM7_RX;
            cfg_tmp->dma_tx_handler  = com7_tx_dma_handler;
            cfg_tmp->dma_rx_handler  = com7_rx_dma_handler;
            cfg_tmp->uart_irqn       = UART7_IRQn;
            cfg_tmp->uart_handler    = com7_uart_handler;
            cfg_tmp->timer_handler   = com7_tim_handler;
            com7_pin_init();
            break;
#endif

        default:
            // error
            break;
    }
}

/*
 * 功能描述：此函数初始化通信端口
 * 入参说明：com_cfg com口配置
 * 返 回 值：无
 */
void com_init(com_config_t *com_cfg) {
    if (com_cfg->port >= COM_CNT) {
        return;
    }

    com_desc_t *desc  = &com_desc[com_cfg->port];
    desc->dma_cfg     = com_cfg->dma_cfg;
    desc->mode        = com_cfg->mode;
    desc->rx_callback = com_cfg->rx_callback;
    desc->port        = com_cfg->port;

    memset(desc->rx_fifo.buffer, 0, desc->rx_fifo.capacity);
    desc->rx_fifo.read_idx = desc->rx_fifo.write_idx = 0;
    memset(desc->tx_fifo.buffer, 0, desc->tx_fifo.capacity);
    desc->tx_fifo.read_idx = desc->tx_fifo.write_idx = 0;

    /* 初始化COM */
    com_config_tmp_t cfg_tmp = {0};
    configure_com_port(desc, &cfg_tmp);

    /* 串口配置 */
    gd32_uart_init(desc->uart, com_cfg->baudrate, com_cfg->fmt);
    gd32_setup_isr(cfg_tmp.uart_irqn, cfg_tmp.uart_handler, 0);
    gd32_setup_irq(cfg_tmp.uart_irqn, 15);

    /* 定时器配置 */
    // init_rx_timer(desc->timer, com_cfg->rx_timeo, cfg_tmp.timer_handler);

    /* 发送DMA配置 */
    if (desc->dma_cfg & COM_SEND_DMA) {
        usart_dma_transmit_config(desc->uart, USART_TRANSMIT_DMA_ENABLE);
        gd32_uart_fill_txdma(desc->uart, &desc->tx_dma);
        gd32_dma_init(&desc->tx_dma, cfg_tmp.dma_tx_priority, cfg_tmp.dma_tx_circular);
        gd32_setup_isr(desc->tx_dma.irqn, cfg_tmp.dma_tx_handler, 0);
        gd32_setup_irq(desc->tx_dma.irqn, 15);
    }

    /* 接收DMA配置 */
    if (desc->dma_cfg & COM_RECV_DMA) {
        usart_dma_receive_config(desc->uart, USART_RECEIVE_DMA_ENABLE);
        gd32_uart_fill_rxdma(desc->uart, &desc->rx_dma);
        gd32_dma_init(&desc->rx_dma, cfg_tmp.dma_rx_priority, cfg_tmp.dma_rx_circular); /* 循环的 */
        gd32_setup_isr(desc->rx_dma.irqn, cfg_tmp.dma_rx_handler, 0);
        gd32_setup_irq(desc->rx_dma.irqn, 15);
    }
    /* 初始化串口收发模式 */
    if (com_cfg->mode == COM_HALF_DUPLEX) {
        com_set_mode(com_cfg->port, COM_MODE_RX);
    } else {
        com_set_mode(com_cfg->port, COM_MODE_RX_TX);
    }
}
