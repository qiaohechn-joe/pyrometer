/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：scom.c
 * 文件描述：通用SPI通信管理（GD32F4x）
 * 作    者：和其光电嵌软团队
 * 备    注：SPI0~SPI5
 * 更新记录：
 *           V1.0 2024/12/05 陈军  初始版本
 */

#include "scom.h"

/* 环形缓冲区 */
typedef struct {
    uint8_t *start;    /* 起始地址 */
    uint8_t *end;      /* 结束地址 */
    uint8_t *head;     /* 读指针 */
    uint8_t *tail;     /* 写指针 */
    int      count;    /* 数据字节计数 */
    int      capacity; /* 容量 */
} circ_buffer_t;

/* SCOM描述符 */
typedef struct {
    uint32_t      spi;              /* 外设 */
    uint32_t      timer;            /* 接收超时定时器 */
    circ_buffer_t tx_fifo;          /* 发送FIFO（环形缓冲区） */
    dma_xfer_t    tx_dma;           /* 发送DMA传输信息 */
    circ_buffer_t rx_fifo;          /* 接收FIFO（环形缓冲区） */
    dma_xfer_t    rx_dma;           /* 接收DMA传输信息 */
    int           rx_len;           /* 接收长度（字节） */
    void (*rx_callback)(void *arg); /* 接收数据包时的回调函数 */
    int flag;
} scom_desc_t;

/* COM描述符数组 */
static scom_desc_t scom_desc[SCOM_CNT];

static void on_dma_tx(scom_desc_t *desc);
static void on_dma_rx(scom_desc_t *desc);
static void on_irq_all(scom_desc_t *desc);
static void on_rx_over(scom_desc_t *desc);

/* clang-format off */
/* SPI0中断服务函数 */
#if defined(SCOM0)
static uint8_t tx_buff0[SCOM0_TX_BUFF_SIZE];
static uint8_t rx_buff0[SCOM0_RX_BUFF_SIZE];

static void scom0_init(scom_desc_t *desc, scom_para_t *scom_para);
static void scom0_txdma_handler(void)  { on_dma_tx(&scom_desc[SCOM0]);  }
static void scom0_rxdma_handler(void)  { on_dma_rx(&scom_desc[SCOM0]);  }
static void scom0_spi_handler(void)    { on_irq_all(&scom_desc[SCOM0]); }
static void scom0_tim_handler(void)    { on_rx_over(&scom_desc[SCOM0]); }
#endif /* defined(SCOM0) */

/* SPI1中断服务函数 */
#if defined(SCOM1)
static uint8_t tx_buff1[SCOM1_TX_BUFF_SIZE];
static uint8_t rx_buff1[SCOM1_RX_BUFF_SIZE];

static void scom1_init(scom_desc_t *desc, scom_para_t *scom_para);
static void scom1_txdma_handler(void)  { on_dma_tx(&scom_desc[SCOM1]);  }
static void scom1_rxdma_handler(void)  { on_dma_rx(&scom_desc[SCOM1]);  }
static void scom1_spi_handler(void)    { on_irq_all(&scom_desc[SCOM1]); }
static void scom1_tim_handler(void)    { on_rx_over(&scom_desc[SCOM1]); }
#endif /* defined(SCOM1) */

/* SPI2中断服务函数 */
#if defined(SCOM2)
static uint8_t tx_buff2[SCOM2_TX_BUFF_SIZE];
static uint8_t rx_buff2[SCOM2_RX_BUFF_SIZE];

static void scom2_init(scom_desc_t *desc, scom_para_t *scom_para);
static void scom2_txdma_handler(void)  { on_dma_tx(&scom_desc[SCOM2]);  }
static void scom2_rxdma_handler(void)  { on_dma_rx(&scom_desc[SCOM2]);  }
static void scom2_spi_handler(void)    { on_irq_all(&scom_desc[SCOM2]); }
static void scom2_tim_handler(void)    { on_rx_over(&scom_desc[SCOM2]); }
#endif /* defined(SCOM2) */

/* SPI3中断服务函数 */
#if defined(SCOM3)
static uint8_t tx_buff3[SCOM3_TX_BUFF_SIZE];
static uint8_t rx_buff3[SCOM3_RX_BUFF_SIZE];

static void scom3_init(scom_desc_t *desc, scom_para_t *scom_para);
static void scom3_txdma_handler(void)  { on_dma_tx(&scom_desc[SCOM3]);  }
static void scom3_rxdma_handler(void)  { on_dma_rx(&scom_desc[SCOM3]);  }
static void scom3_spi_handler(void)    { on_irq_all(&scom_desc[SCOM3]); }
static void scom3_tim_handler(void)    { on_rx_over(&scom_desc[SCOM3]); }
#endif /* defined(SCOM3) */

/* SPI4中断服务函数 */
#if defined(SCOM4)
static uint8_t tx_buff4[SCOM4_TX_BUFF_SIZE];
static uint8_t rx_buff4[SCOM4_RX_BUFF_SIZE];

static void scom4_init(scom_desc_t *desc, scom_para_t *scom_para);
static void scom4_txdma_handler(void)  { on_dma_tx(&scom_desc[SCOM4]);  }
static void scom4_rxdma_handler(void)  { on_dma_rx(&scom_desc[SCOM4]);  }
static void scom4_spi_handler(void)    { on_irq_all(&scom_desc[SCOM4]); }
static void scom4_tim_handler(void)    { on_rx_over(&scom_desc[SCOM4]); }
#endif /* defined(SCOM4) */

/* SPI5中断服务函数 */
#if defined(SCOM5)
static uint8_t tx_buff5[SCOM5_TX_BUFF_SIZE];
static uint8_t rx_buff5[SCOM5_RX_BUFF_SIZE];

static void scom5_init(scom_desc_t *desc, scom_para_t *scom_para);
static void scom5_txdma_handler(void)  { on_dma_tx(&scom_desc[SCOM5]);  }
static void scom5_rxdma_handler(void)  { on_dma_rx(&scom_desc[SCOM5]);  }
static void scom5_spi_handler(void)    { on_irq_all(&scom_desc[SCOM5]); }
static void scom5_tim_handler(void)    { on_rx_over(&scom_desc[SCOM5]); }
#endif /* defined(SCOM5) */
/* clang-format on */

/*
 * 功能描述：此函数初始化通信端口
 * 入参说明：port --- 端口号，SCOMx(x=0至5)
 *           scom_para --- 指向scom_para_t结构体指针
 * 返 回 值：无
 */
void scom_init(int port, scom_para_t *scom_para) {
    scom_desc_t *desc;
    const char  *fmt;

    if ((port < 0) || (port >= SCOM_CNT)) {
        return;
    }

    desc = &scom_desc[port];
    mem_set(desc, sizeof(*desc), 0);

    /* 初始化SCOM */
    desc->flag = scom_para->flag;
    switch (port) {
#if defined(SCOM0)
        case SCOM0: scom0_init(desc, scom_para); break;
#endif

#if defined(SCOM1)
        case SCOM1: scom1_init(desc, scom_para); break;
#endif

#if defined(SCOM2)
        case SCOM2: scom2_init(desc, scom_para); break;
#endif

#if defined(SCOM3)
        case SCOM3: scom3_init(desc, scom_para); break;
#endif

#if defined(SCOM4)
        case SCOM4: scom4_init(desc, scom_para); break;
#endif

#if defined(SCOM5)
        case SCOM5: scom5_init(desc, scom_para); break;
#endif
        default: break;
    }

    /* 清FIFO */
    desc->tx_fifo.count = 0;
    desc->tx_fifo.head = desc->tx_fifo.tail = desc->tx_fifo.start;
    desc->rx_fifo.count                     = 0;
    desc->rx_fifo.head = desc->rx_fifo.tail = desc->rx_fifo.start;
}

/*
 * 功能描述：此函数清除发送（Tx）FIFO
 * 入参说明：port --- 端口号，SCOMx(x=0至5)
 * 返 回 值：无
 */
void scom_clr_tx_fifo(int port) {
    scom_desc_t   *desc;
    circ_buffer_t *fifo;

    if ((port < 0) || (port >= SCOM_CNT)) {
        return;
    }

    desc = &scom_desc[port];
    fifo = &desc->tx_fifo;

    fifo->count = 0;
    fifo->head = fifo->tail = fifo->start;
}

/*
 * 功能描述：此函数清除接收（Rx）FIFO
 * 入参说明：port --- 端口号，SCOMx(x=0至5)
 * 返 回 值：无
 */
void scom_clr_rx_fifo(int port) {
    scom_desc_t   *desc;
    circ_buffer_t *fifo;

    if ((port < 0) || (port >= SCOM_CNT)) {
        return;
    }

    desc = &scom_desc[port];
    fifo = &desc->rx_fifo;

    fifo->count = 0;
    fifo->head = fifo->tail = fifo->start;
}

/*
 * 功能描述：此函数启用通过FIFO接收数据
 * 入参说明：port --- 端口号，SCOMx(x=0至5)
 * 返 回 值：无
 */
void scom_enable_recv(int port) {
    scom_desc_t   *desc;
    circ_buffer_t *fifo;
    dma_xfer_t    *dma;
    int            temp;

    if ((port < 0) || (port >= SCOM_CNT)) {
        return;
    }

    desc = &scom_desc[port];
    fifo = &desc->rx_fifo;
    dma  = &desc->rx_dma;

    (void)gd32_spi_rx(desc->spi);
    clear_dma_flag(dma); /* 清除标志 */

    if (desc->flag & SCOM_RECV_DMA) { /* DMA */
        /* 开始DMA传输 */
        gd32_dma_start(dma, fifo->tail, 1);
        enable_dma_irq(dma);
    } else { /* IRQ */
        spi_i2s_interrupt_enable(desc->spi, SPI_I2S_INT_RBNE);
    }
}

/*
 * 功能描述：此函数禁用通过FIFO接收数据
 * 入参说明：port --- 端口号，SCOMx(x=0至5)
 * 返 回 值：无
 */
void scom_disable_recv(int port) {
    scom_desc_t *desc;
    dma_xfer_t  *dma;

    if ((port < 0) || (port >= SCOM_CNT)) {
        return;
    }

    desc = &scom_desc[port];
    dma  = &desc->rx_dma;

    if (desc->flag & SCOM_RECV_DMA) { /* DMA */
        disable_dma_irq(dma);
        dma_channel_disable(dma->dmac, dma->channel);
    } else { /* IRQ */
        spi_i2s_interrupt_disable(desc->spi, SPI_I2S_INT_RBNE);
    }
}

/*
 * 功能描述：此函数设置接收参数
 * 入参说明：port --- 端口号，SCOMx(x=0至5)
 *           callback --- 接收超时时调用的回调函数
 * 返 回 值：无
 */
void scom_set_recv(int port, void (*callback)(void *arg)) {
    scom_desc_t   *desc;
    circ_buffer_t *fifo;

    if ((port < 0) || (port >= SCOM_CNT)) {
        return;
    }

    desc = &scom_desc[port];
    fifo = &desc->rx_fifo;

    desc->rx_len      = 0;
    desc->rx_callback = callback;

    /* 清FIFO */
    fifo->count = 0;
    fifo->head = fifo->tail = fifo->start;
}

/*
 * 功能描述：此函数过滤接收到的数据
 * 入参说明：port --- 端口号，SCOMx(x=0至5)
 *           min_len --- 最小接收字节数
 *           max_len --- 最大接收字节数
 * 返 回 值：0 = 成功   其他 = 失败
 * 备    注：由 rx_callback() 调用

 */
int scom_filter_recv(int port, int min_len, int max_len) {
    scom_desc_t   *desc = &scom_desc[port];
    circ_buffer_t *fifo = &desc->rx_fifo;

    if ((desc->rx_len < min_len) || (desc->rx_len > max_len)) { /* 长度不匹配 */
        /* 清FIFO */
        fifo->count = 0;
        fifo->head = fifo->tail = fifo->start;
        desc->rx_len            = 0;
        return -1;
    }

    return 0;
}

/*
 * 功能描述：此函数将数据直接发送到SPI口
 * 入参说明：port --- 端口号，SCOMx(x = 0至55)
 *           buff --- 指向缓冲区的指针
 *           len --- 数据长度（字节）
 * 返 回 值：成功发送的数据字节数
 */
int scom_put(int port, const void *buff, int len) {
    scom_desc_t *desc;
    uint8_t     *data;
    int          i;

    if ((port < 0) || (port >= SCOM_CNT)) {
        return 0;
    }

    data = (uint8_t *)buff;
    desc = &scom_desc[port];

    for (i = 0; i < len; i++) {
        while (!gd32_spi_tx_rdy(desc->spi));
        gd32_spi_tx(desc->spi, *data++);
    }
    while (gd32_spi_busy(desc->spi));

    return len;
}

/*
 * 功能描述：此函数直接从SPI口接收数据
 * 入参说明：port --- 端口号，SCOMx(x=0至5)
 *           buff --- 指向缓冲区的指针
 *           size --- 缓冲区大小（字节）
 * 返 回 值：接收到的数据字节数
 */
int scom_get(int port, void *buff, int size) {
    scom_desc_t *desc;
    uint8_t     *data = (uint8_t *)buff;
    int          val, i;

    if ((port < 0) || (port >= SCOM_CNT)) {
        return 0;
    }

    desc = &scom_desc[port];

    for (i = 0; i < size; i++) {
        val     = gd32_spi_xfer(desc->spi, 0x00);
        *data++ = (uint8_t)val;
    }

    return data - (uint8_t *)buff;
}

/*
 * 功能描述：此函数通过FIFO向串口发送数据
 * 入参说明：port --- 端口号，SCOMx(x=0至5)
 *           buff --- 指向缓冲区的指针
 *           len --- 数据长度（字节）
 * 返 回 值：成功发送的数据字节数
 * 备    注：非重入函数。必须通过临界区（CRITICAL-SECTION）或互斥锁（mutex）保护
 */
int scom_send(int port, const void *buff, int len) {
    scom_desc_t   *desc;
    dma_xfer_t    *dma;
    circ_buffer_t *fifo;
    const uint8_t *data;
    int            size;
    int            tx_stop;

    if ((port < 0) || (port >= SCOM_CNT)) {
        return 0;
    }

    desc = &scom_desc[port];
    dma  = &desc->tx_dma;
    fifo = &desc->tx_fifo;

    if (len > (fifo->capacity - fifo->count)) {
        len = (fifo->capacity - fifo->count);
    }

    data    = (const uint8_t *)buff;
    tx_stop = (fifo->count == 0);

    /* 复制数据 */
    size = fifo->end - fifo->tail; /* 队列中当前有多少数据待处理 */
    if (len > size) {
        mem_copy(fifo->tail, data, size);               /* 第一段 */
        mem_copy(fifo->start, data + size, len - size); /* 第二段 */
        fifo->tail = fifo->start + len - size;
    } else {
        mem_copy(fifo->tail, data, len);
        fifo->tail += len;
        if (fifo->tail == fifo->end) {
            fifo->tail = fifo->start;
        }
    }

    fifo->count += len;

    /* 开始传输 */
    if (tx_stop) {                        /* 传输停止 */
        if (desc->flag & SCOM_SEND_DMA) { /* DMA */
            /* 开始DMA传输 */
            size = fifo->end - fifo->head;
            if (size > fifo->count) {
                size = fifo->count;
            }
            if (size > COM_DMA_XFER_SIZE) {
                size = COM_DMA_XFER_SIZE;
            }

            gd32_dma_start(dma, fifo->head, size);
            enable_dma_irq(dma);
        } else { /* IRQ */
            spi_i2s_interrupt_enable(desc->spi, SPI_I2S_INT_TBE);
        }
    }

    return len;
}

/*
 * 功能描述：此函数从串口的接收（Rx）FIFO中接收数据
 * 入参说明：port --- 端口号，SCOMx(x=0至5)
 *           buff --- 指向缓冲区的指针
 *           size --- 缓冲区大小（字节）
 * 返 回 值：接收到的数据字节数
 * 备    注：在使用此函数之前，必须先调用一次 com_enable_recv()
 */
int scom_recv(int port, void *buff, int size) {
    scom_desc_t   *desc = &scom_desc[port];
    circ_buffer_t *fifo = &desc->rx_fifo;
    uint8_t       *data;
    int            len;
    int            count;
    int            rx_stop;

    if ((port < 0) || (port >= SCOM_CNT)) {
        return 0;
    }

    desc = &scom_desc[port];
    fifo = &desc->rx_fifo;

    len = desc->rx_len;
    if (len == 0) {
        return 0; /* 没有数据 */
    }

    /* 复制数据 */
    data    = (uint8_t *)buff;
    rx_stop = (len == fifo->capacity);
    if (len > size) {
        len = size;
    }

    count = fifo->end - fifo->head; /* 队列中当前有多少数据待处理 */
    if (len > count) {
        mem_copy(data, fifo->head, count);                /* 第一段 */
        mem_copy(data + count, fifo->start, len - count); /* 第二段 */
        fifo->head = fifo->start + len - count;
    } else {
        mem_copy(data, fifo->head, len);
        fifo->head += len;
        if (fifo->head == fifo->end) {
            fifo->head = fifo->start;
        }
    }

    /* 更新FIFO */
    taskENTER_CRITICAL();
    fifo->count -= len;
    desc->rx_len = 0;
    taskEXIT_CRITICAL();

    /* 重新使能接收FIFO */
    if (rx_stop) {
        scom_enable_recv(port);
    }

    return len;
}

/*
 * 功能描述：此函数是DMA模式下的发送（TX）处理函数
 * 入参说明：desc --- 指向 com_descriptor 结构的指针
 * 返 回 值：无
 * 备    注：由 DMA-TX 中断服务函数调用
 */
static void on_dma_tx(scom_desc_t *desc) {
    dma_xfer_t    *dma  = &desc->tx_dma;
    circ_buffer_t *fifo = &desc->tx_fifo;
    int            size;
    UBaseType_t    isr_status;

    if (dma_flag_valid(dma)) { /* 传输完成或错误 */
        clear_dma_flag(dma);   /* 清除所有标志 */

        isr_status = taskENTER_CRITICAL_FROM_ISR();

        /* 发送完成大小 */
        size = (dma->total_cnt - dma_transfer_number_get(dma->dmac, dma->channel));
        size *= dma->width;
        dma_channel_disable(dma->dmac, dma->channel);

        fifo->head += size;
        if (fifo->head == fifo->end) {
            fifo->head = fifo->start;
        }
        fifo->count -= size;

        if (fifo->count == 0) { /* 缓存区空 */
            /* 发送完成 */
            disable_dma_irq(dma);
        } else {
            /* 重新开始传输 */
            size = fifo->end - fifo->head;
            if (size > fifo->count) {
                size = fifo->count;
            }
            if (size > COM_DMA_XFER_SIZE) {
                size = COM_DMA_XFER_SIZE;
            }

            gd32_dma_start(dma, fifo->head, size);
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
static void on_dma_rx(scom_desc_t *desc) {
    dma_xfer_t    *dma  = &desc->rx_dma;
    circ_buffer_t *fifo = &desc->rx_fifo;
    int            size;
    UBaseType_t    isr_status;

    if (dma_flag_valid(dma)) { /* 传输完成或错误 */
        clear_dma_flag(dma);   /* 清所有标志 */

        isr_status = taskENTER_CRITICAL_FROM_ISR();

        /* 接收完成大小 */
        size = (dma->total_cnt - dma_transfer_number_get(dma->dmac, dma->channel));
        size *= dma->width;
        dma_channel_disable(dma->dmac, dma->channel);

        fifo->tail += size;
        if (fifo->tail == fifo->end) {
            fifo->tail = fifo->start;
        }
        fifo->count += size;

        if (fifo->count == fifo->capacity) { /* FIFO满 */
            disable_dma_irq(dma);
            desc->rx_len = fifo->count;
            if (desc->rx_callback) {
                desc->rx_callback((void *)desc->rx_len);
            }
        } else {
            /* 清接收错误标志 */
            size = SPI_STAT(desc->spi);
            size = SPI_DATA(desc->spi);
            size = fifo->capacity - fifo->count; /* 剩余空间 */
            if (size > (fifo->end - fifo->tail)) {
                size = fifo->end - fifo->tail;
            }
            if (size > COM_DMA_XFER_SIZE) {
                size = COM_DMA_XFER_SIZE;
            }

            gd32_dma_start(dma, fifo->tail, size);

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
static void on_irq_all(scom_desc_t *desc) {
    circ_buffer_t *fifo;
    int            temp;
    UBaseType_t    isr_status;

    isr_status = taskENTER_CRITICAL_FROM_ISR();

    if ((spi_i2s_interrupt_flag_get(desc->spi, SPI_I2S_INT_FLAG_RBNE) == SET) ||
        spi_i2s_interrupt_flag_get(desc->spi, SPI_I2S_INT_FLAG_RXORERR) == SET) { /* 接收准备好且中断使能 */
        if (gd32_spi_overrun(desc->spi)) {                                        /* 接收溢出 */
            temp = gd32_uart_rx(desc->spi);                                       /* 清标志 */
            (void)temp;
        }

        if (gd32_spi_rx_rdy(desc->spi)) { /* 接收准备好 */
            fifo = &desc->rx_fifo;

            /* 接收字符 */
            *(fifo->tail) = gd32_spi_rx(desc->spi);
            fifo->tail++;
            if (fifo->tail == fifo->end) {
                fifo->tail = fifo->start;
            }
            fifo->count++;

            if (fifo->count == fifo->capacity) { /* FIFO满 */
                usart_interrupt_disable(desc->spi, USART_INT_RBNE);

                desc->rx_len = fifo->count;
                if (desc->rx_callback) {
                    desc->rx_callback((void *)desc->rx_len);
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

    if ((spi_i2s_interrupt_flag_get(desc->spi, SPI_I2S_INT_FLAG_TBE) == SET) && gd32_spi_tx_rdy(desc->spi)) { /* 发送 */
        fifo = &desc->tx_fifo;

        /* 发送字符 */
        gd32_spi_tx(desc->spi, *(fifo->head));
        fifo->head++;
        if (fifo->head == fifo->end) {
            fifo->head = fifo->start;
        }
        fifo->count--;

        if (fifo->count == 0) { /* 缓存区空 */
            /* 发送完成 */
            spi_i2s_interrupt_disable(desc->spi, SPI_I2S_INT_TBE);
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
static void on_rx_over(scom_desc_t *desc) {
    dma_xfer_t    *dma  = &desc->rx_dma;
    circ_buffer_t *fifo = &desc->rx_fifo;
    int            size;
    UBaseType_t    isr_status;

    if (timer_flag_get(desc->timer, TIMER_FLAG_UP) == SET) {
        timer_flag_clear(desc->timer, TIMER_FLAG_UP);
        timer_interrupt_disable(desc->timer, TIMER_INT_UP); /* 失能中断 */
        timer_disable(desc->timer);

        isr_status = taskENTER_CRITICAL_FROM_ISR();

        if (desc->flag & SCOM_RECV_DMA) { /* DMA接收 */
            /* 完成的大小 */
            size = (dma->total_cnt - dma_transfer_number_get(dma->dmac, dma->channel));
            size *= dma->width;
            dma_channel_disable(dma->dmac, dma->channel);

            fifo->tail += size;
            if (fifo->tail == fifo->end) {
                fifo->tail = fifo->start;
            }
            fifo->count += size;

            /* 调用rx_callback() */
            desc->rx_len = fifo->count;
            if (desc->rx_callback) {
                desc->rx_callback((void *)desc->rx_len);
            }

            /* 清接收错误标志 */
            size = SPI_STAT(desc->spi);
            size = SPI_DATA(desc->spi);
            gd32_dma_start(dma, fifo->tail, 1); /* 启动下次接收 */
        } else {
            /* 调用rx_callback() */
            desc->rx_len = fifo->count;
            if (desc->rx_callback) {
                desc->rx_callback((void *)desc->rx_len);
            }
        }

        taskEXIT_CRITICAL_FROM_ISR(isr_status);
    }
}

#if defined(SCOM0)
/*
 * 功能描述：此函数初始化SPI0
 * 入参说明：desc --- 指向scom_desc_t结构的指针
 *           scom_para --- 指向scom_para_t结构体指针
 * 返 回 值：无
 */
static void scom0_init(scom_desc_t *desc, scom_para_t *scom_para) {
    /* 数据结构 */
    desc->spi              = SPI0;
    desc->timer            = SCOM0_RX_TIMER;
    desc->tx_fifo.capacity = sizeof(tx_buff0);
    desc->tx_fifo.start    = &tx_buff0[0];
    desc->tx_fifo.end      = &tx_buff0[0] + sizeof(tx_buff0);
    desc->rx_fifo.capacity = sizeof(rx_buff0);
    desc->rx_fifo.start    = &rx_buff0[0];
    desc->rx_fifo.end      = &rx_buff0[0] + sizeof(rx_buff0);

    /* SPI0配置 */
    gd32_spi_init(SPI0, scom_para->spi_mode, scom_para->rate, scom_para->clk_mode, scom_para->word);
    scom0_pin_init();
    gd32_setup_isr(SPI0_IRQn, scom0_spi_handler, 0);
    gd32_setup_irq(SPI0_IRQn, 15);

    /* 定时器配置 */
    timer_timeo_init(desc->timer, scom_para->rx_timeo, scom0_tim_handler);

    /* 发送DMA配置 */
    if (desc->flag & SCOM_SEND_DMA) {
        spi_dma_enable(SPI0, SPI_DMA_TRANSMIT);
        gd32_spi_fill_txdma(SPI0, &desc->tx_dma);
        gd32_dma_init(&desc->tx_dma, DMA_PRIO_SPI0_TX, 0); /* 非循环的 */
        if (desc->tx_dma.channel == DMA_CH3) {
            gd32_setup_isr(DMA1_Channel3_IRQn, scom0_txdma_handler, 0);
            gd32_setup_irq(DMA1_Channel3_IRQn, 15);
        } else {
            gd32_setup_isr(DMA1_Channel5_IRQn, scom0_txdma_handler, 0);
            gd32_setup_irq(DMA1_Channel5_IRQn, 15);
        }
    }

    /* 接收DMA配置 */
    if (desc->flag & SCOM_RECV_DMA) {
        spi_dma_enable(SPI0, SPI_DMA_RECEIVE);
        gd32_spi_fill_rxdma(SPI0, &desc->rx_dma);
        gd32_dma_init(&desc->rx_dma, DMA_PRIO_SPI0_RX, 0); /* 非循环的 */
        if (desc->rx_dma.channel == DMA_CH0) {
            gd32_setup_isr(DMA1_Channel0_IRQn, scom0_rxdma_handler, 0);
            gd32_setup_irq(DMA1_Channel0_IRQn, 15);
        } else {
            gd32_setup_isr(DMA1_Channel2_IRQn, scom0_rxdma_handler, 0);
            gd32_setup_irq(DMA1_Channel2_IRQn, 15);
        }
    }
}
#endif /* defined (SCOM0) */

#if defined(SCOM1)
/*
 * 功能描述：此函数初始化SPI1
 * 入参说明：desc --- 指向scom_desc_t结构的指针
 *           scom_para --- 指向scom_para_t结构体指针
 * 返 回 值：无
 */
static void scom1_init(scom_desc_t *desc, scom_para_t *scom_para) {
    /* 数据结构 */
    desc->spi              = SPI1;
    desc->timer            = SCOM1_RX_TIMER;
    desc->tx_fifo.capacity = sizeof(tx_buff1);
    desc->tx_fifo.start    = &tx_buff1[0];
    desc->tx_fifo.end      = &tx_buff1[0] + sizeof(tx_buff1);
    desc->rx_fifo.capacity = sizeof(rx_buff1);
    desc->rx_fifo.start    = &rx_buff1[0];
    desc->rx_fifo.end      = &rx_buff1[0] + sizeof(rx_buff1);

    /* SPI1配置 */
    gd32_spi_init(SPI1, scom_para->spi_mode, scom_para->rate, scom_para->clk_mode, scom_para->word);
    scom1_pin_init();
    gd32_setup_isr(SPI1_IRQn, scom1_spi_handler, 0);
    gd32_setup_irq(SPI1_IRQn, 15);

    /* 定时器配置 */
    timer_timeo_init(desc->timer, scom_para->rx_timeo, scom1_tim_handler);

    /* 发送DMA配置 */
    if (desc->flag & SCOM_SEND_DMA) {
        spi_dma_enable(SPI1, SPI_DMA_TRANSMIT);
        gd32_spi_fill_txdma(SPI1, &desc->tx_dma);
        gd32_dma_init(&desc->tx_dma, DMA_PRIO_SPI1_TX, 0); /* 非循环的 */
        gd32_setup_isr(DMA0_Channel4_IRQn, scom1_txdma_handler, 0);
        gd32_setup_irq(DMA0_Channel4_IRQn, 15);
    }

    /* 接收DMA配置 */
    if (desc->flag & SCOM_RECV_DMA) {
        spi_dma_enable(SPI1, SPI_DMA_RECEIVE);
        gd32_spi_fill_rxdma(SPI1, &desc->rx_dma);
        gd32_dma_init(&desc->rx_dma, DMA_PRIO_SPI1_RX, 0); /* 非循环的 */
        gd32_setup_isr(DMA0_Channel3_IRQn, scom1_rxdma_handler, 0);
        gd32_setup_irq(DMA0_Channel3_IRQn, 15);
    }
}
#endif /* defined (SCOM1) */

#if defined(SCOM2)
/*
 * 功能描述：此函数初始化SPI2
 * 入参说明：desc --- 指向scom_desc_t结构的指针
 *           scom_para --- 指向scom_para_t结构体指针
 * 返 回 值：无
 */
static void scom2_init(scom_desc_t *desc, scom_para_t *scom_para) {
    /* 数据结构 */
    desc->spi              = SPI2;
    desc->timer            = SCOM2_RX_TIMER;
    desc->tx_fifo.capacity = sizeof(tx_buff2);
    desc->tx_fifo.start    = &tx_buff2[0];
    desc->tx_fifo.end      = &tx_buff2[0] + sizeof(tx_buff2);
    desc->rx_fifo.capacity = sizeof(rx_buff2);
    desc->rx_fifo.start    = &rx_buff2[0];
    desc->rx_fifo.end      = &rx_buff2[0] + sizeof(rx_buff2);

    /* SPI2配置 */
    gd32_spi_init(SPI2, scom_para->spi_mode, scom_para->rate, scom_para->clk_mode, scom_para->word);
    scom2_pin_init();
    gd32_setup_isr(SPI2_IRQn, scom2_spi_handler, 0);
    gd32_setup_irq(SPI2_IRQn, 15);

    /* 定时器配置 */
    timer_timeo_init(desc->timer, scom_para->rx_timeo, scom2_tim_handler);

    /* 发送DMA配置 */
    if (desc->flag & SCOM_SEND_DMA) {
        spi_dma_enable(SPI2, SPI_DMA_TRANSMIT);
        gd32_spi_fill_txdma(SPI2, &desc->tx_dma);
        gd32_dma_init(&desc->tx_dma, DMA_PRIO_SPI2_TX, 0); /* 非循环的 */
        if (desc->tx_dma.channel == DMA_CH5) {
            gd32_setup_isr(DMA0_Channel5_IRQn, scom2_txdma_handler, 0);
            gd32_setup_irq(DMA0_Channel5_IRQn, 15);
        } else {
            gd32_setup_isr(DMA0_Channel7_IRQn, scom2_txdma_handler, 0);
            gd32_setup_irq(DMA0_Channel7_IRQn, 15);
        }
    }

    /* 接收DMA配置 */
    if (desc->flag & SCOM_RECV_DMA) {
        spi_dma_enable(SPI2, SPI_DMA_RECEIVE);
        gd32_spi_fill_rxdma(SPI2, &desc->rx_dma);
        gd32_dma_init(&desc->rx_dma, DMA_PRIO_SPI2_RX, 0); /* 非循环的 */
        if (desc->rx_dma.channel == DMA_CH0) {
            gd32_setup_isr(DMA0_Channel0_IRQn, scom2_rxdma_handler, 0);
            gd32_setup_irq(DMA0_Channel0_IRQn, 15);
        } else {
            gd32_setup_isr(DMA0_Channel2_IRQn, scom2_rxdma_handler, 0);
            gd32_setup_irq(DMA0_Channel2_IRQn, 15);
        }
    }
}
#endif /* defined (SCOM2) */

#if defined(SCOM3)
/*
 * 功能描述：此函数初始化SPI3
 * 入参说明：desc --- 指向scom_desc_t结构的指针
 *           scom_para --- 指向scom_para_t结构体指针
 * 返 回 值：无
 */
static void scom3_init(scom_desc_t *desc, scom_para_t *scom_para) {
    /* 数据结构 */
    desc->spi              = SPI3;
    desc->timer            = SCOM3_RX_TIMER;
    desc->tx_fifo.capacity = sizeof(tx_buff3);
    desc->tx_fifo.start    = &tx_buff3[0];
    desc->tx_fifo.end      = &tx_buff3[0] + sizeof(tx_buff3);
    desc->rx_fifo.capacity = sizeof(rx_buff3);
    desc->rx_fifo.start    = &rx_buff3[0];
    desc->rx_fifo.end      = &rx_buff3[0] + sizeof(rx_buff3);

    /* SPI3配置 */
    gd32_spi_init(SPI3, scom_para->spi_mode, scom_para->rate, scom_para->clk_mode, scom_para->word);
    scom3_pin_init();
    gd32_setup_isr(SPI3_IRQn, scom3_spi_handler, 0);
    gd32_setup_irq(SPI3_IRQn, 15);

    /* 定时器配置 */
    timer_timeo_init(desc->timer, scom_para->rx_timeo, scom3_tim_handler);

    /* 发送DMA配置 */
    if (desc->flag & SCOM_SEND_DMA) {
        spi_dma_enable(SPI3, SPI_DMA_TRANSMIT);
        gd32_spi_fill_txdma(SPI3, &desc->tx_dma);
        gd32_dma_init(&desc->tx_dma, DMA_PRIO_SPI3_TX, 0); /* 非循环的 */
        if (desc->tx_dma.channel == DMA_CH1) {
            gd32_setup_isr(DMA1_Channel1_IRQn, scom3_txdma_handler, 0);
            gd32_setup_irq(DMA1_Channel1_IRQn, 15);
        } else {
            gd32_setup_isr(DMA1_Channel4_IRQn, scom3_txdma_handler, 0);
            gd32_setup_irq(DMA1_Channel4_IRQn, 15);
        }
    }

    /* 接收DMA配置 */
    if (desc->flag & SCOM_RECV_DMA) {
        spi_dma_enable(SPI3, SPI_DMA_RECEIVE);
        gd32_spi_fill_rxdma(SPI3, &desc->rx_dma);
        gd32_dma_init(&desc->rx_dma, DMA_PRIO_SPI3_RX, 0); /* 非循环的 */
        if (desc->rx_dma.channel == DMA_CH0) {
            gd32_setup_isr(DMA1_Channel0_IRQn, scom3_rxdma_handler, 0);
            gd32_setup_irq(DMA1_Channel0_IRQn, 15);
        } else {
            gd32_setup_isr(DMA1_Channel3_IRQn, scom3_rxdma_handler, 0);
            gd32_setup_irq(DMA1_Channel3_IRQn, 15);
        }
    }
}
#endif /* defined (SCOM3) */

#if defined(SCOM4)
/*
 * 功能描述：此函数初始化SPI4
 * 入参说明：desc --- 指向scom_desc_t结构的指针
 *           scom_para --- 指向scom_para_t结构体指针
 * 返 回 值：无
 */
static void scom4_init(scom_desc_t *desc, scom_para_t *scom_para) {
    /* 数据结构 */
    desc->spi              = SPI4;
    desc->timer            = SCOM4_RX_TIMER;
    desc->tx_fifo.capacity = sizeof(tx_buff4);
    desc->tx_fifo.start    = &tx_buff4[0];
    desc->tx_fifo.end      = &tx_buff4[0] + sizeof(tx_buff4);
    desc->rx_fifo.capacity = sizeof(rx_buff4);
    desc->rx_fifo.start    = &rx_buff4[0];
    desc->rx_fifo.end      = &rx_buff4[0] + sizeof(rx_buff4);

    /* SPI4配置 */
    gd32_spi_init(SPI4, scom_para->spi_mode, scom_para->rate, scom_para->clk_mode, scom_para->word);
    scom4_pin_init();
    gd32_setup_isr(SPI4_IRQn, scom4_spi_handler, 0);
    gd32_setup_irq(SPI4_IRQn, 15);

    /* 定时器配置 */
    timer_timeo_init(desc->timer, scom_para->rx_timeo, scom4_tim_handler);

    /* 发送DMA配置 */
    if (desc->flag & SCOM_SEND_DMA) {
        spi_dma_enable(SPI4, SPI_DMA_TRANSMIT);
        gd32_spi_fill_txdma(SPI4, &desc->tx_dma);
        gd32_dma_init(&desc->tx_dma, DMA_PRIO_SPI4_TX, 0); /* 非循环的 */
        if (desc->tx_dma.channel == DMA_CH4) {
            gd32_setup_isr(DMA1_Channel4_IRQn, scom4_txdma_handler, 0);
            gd32_setup_irq(DMA1_Channel4_IRQn, 15);
        } else {
            gd32_setup_isr(DMA1_Channel6_IRQn, scom4_txdma_handler, 0);
            gd32_setup_irq(DMA1_Channel6_IRQn, 15);
        }
    }

    /* 接收DMA配置 */
    if (desc->flag & SCOM_RECV_DMA) {
        spi_dma_enable(SPI4, SPI_DMA_RECEIVE);
        gd32_spi_fill_rxdma(SPI4, &desc->rx_dma);
        gd32_dma_init(&desc->rx_dma, DMA_PRIO_SPI4_RX, 0); /* 非循环的 */
        if (desc->rx_dma.channel == DMA_CH3) {
            gd32_setup_isr(DMA1_Channel3_IRQn, scom4_rxdma_handler, 0);
            gd32_setup_irq(DMA1_Channel3_IRQn, 15);
        } else {
            gd32_setup_isr(DMA1_Channel5_IRQn, scom4_rxdma_handler, 0);
            gd32_setup_irq(DMA1_Channel5_IRQn, 15);
        }
    }
}
#endif /* defined (SCOM4) */

#if defined(SCOM5)
/*
 * 功能描述：此函数初始化SPI5
 * 入参说明：desc --- 指向scom_desc_t结构的指针
 *           scom_para --- 指向scom_para_t结构体指针
 * 返 回 值：无
 */
static void scom5_init(scom_desc_t *desc, scom_para_t *scom_para) {
    /* 数据结构 */
    desc->spi              = SPI5;
    desc->timer            = SCOM5_RX_TIMER;
    desc->tx_fifo.capacity = sizeof(tx_buff5);
    desc->tx_fifo.start    = &tx_buff5[0];
    desc->tx_fifo.end      = &tx_buff5[0] + sizeof(tx_buff5);
    desc->rx_fifo.capacity = sizeof(rx_buff5);
    desc->rx_fifo.start    = &rx_buff5[0];
    desc->rx_fifo.end      = &rx_buff5[0] + sizeof(rx_buff5);

    /* SPI5配置 */
    gd32_spi_init(SPI5, scom_para->spi_mode, scom_para->rate, scom_para->clk_mode, scom_para->word);
    scom5_pin_init();
    gd32_setup_isr(SPI5_IRQn, scom5_spi_handler, 0);
    gd32_setup_irq(SPI5_IRQn, 15);

    /* 定时器配置 */
    timer_timeo_init(desc->timer, scom_para->rx_timeo, scom5_tim_handler);

    /* 发送DMA配置 */
    if (desc->flag & SCOM_SEND_DMA) {
        spi_dma_enable(SPI5, SPI_DMA_TRANSMIT);
        gd32_spi_fill_txdma(SPI5, &desc->tx_dma);
        gd32_dma_init(&desc->tx_dma, DMA_PRIO_SPI5_TX, 0); /* 非循环的 */
        gd32_setup_isr(DMA1_Channel5_IRQn, scom5_txdma_handler, 0);
        gd32_setup_irq(DMA1_Channel5_IRQn, 15);
    }

    /* 接收DMA配置 */
    if (desc->flag & SCOM_RECV_DMA) {
        spi_dma_enable(SPI5, SPI_DMA_RECEIVE);
        gd32_spi_fill_rxdma(SPI5, &desc->rx_dma);
        gd32_dma_init(&desc->rx_dma, DMA_PRIO_SPI5_RX, 0); /* 非循环的 */
        gd32_setup_isr(DMA1_Channel6_IRQn, scom5_rxdma_handler, 0);
        gd32_setup_irq(DMA1_Channel6_IRQn, 15);
    }
}
#endif /* defined (SCOM5) */
