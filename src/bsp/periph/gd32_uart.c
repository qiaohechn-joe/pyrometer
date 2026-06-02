/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_uart.c
 * 文件描述：UART-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/12/05 陈军  简化串口DMA配置
 */

#include "gd32_uart.h"
#include "gd32_ll.h"

static void init_peripheral(uint32_t dev);

/*
 * 功能描述：此函数初始化串口
 * 入参说明：dev --- UARTx（x=0至7）
 *           rate --- 波特率，可选值：2400/4800/9600/19200/38400/57600/76800/115200
 *           format --- 数据格式字符串
 *                      format[0] = 字长，'7' 或 '8'，默认为 '8'
 *                      format[1] = 校验位，'N'（无校验）/ 'O'（奇校验）/'E'（偶校验），默认为 'N'
 *                      format[2] = 停止位，'1' 或 '2'，默认为 '1'
 * 返 回 值：无
 */
void gd32_uart_init(uint32_t dev, uint32_t rate, const char *format) {
    uint32_t word;
    uint32_t par;

    /* 外设初始化 */
    init_peripheral(dev);

    /* 数据格式 */
    word = format[0] - '0';
    if ((word < 7) || (word > 9)) {
        word = 8;
    }

    par = USART_PM_NONE;
    if (word < 9) {
        if ((format[1] == 'o') || (format[1] == 'O')) { /* 奇校验 */
            par = USART_PM_ODD;
            word++;
        } else if ((format[1] == 'e') || (format[1] == 'E')) { /* 偶校验 */
            par = USART_PM_EVEN;
            word++;
        }
    }

    /* 配置 */
    usart_oversample_config(dev, USART_OVSMOD_16);
    usart_baudrate_set(dev, rate);

    switch (word) {
        case 8: usart_word_length_set(dev, USART_WL_8BIT); break;
        case 9: usart_word_length_set(dev, USART_WL_9BIT); break;
        default: usart_word_length_set(dev, USART_WL_8BIT); break;
    }

    usart_parity_config(dev, par);

    switch (format[2]) {
        case '1': usart_stop_bit_set(dev, USART_STB_1BIT); break;
        case '2': usart_stop_bit_set(dev, USART_STB_2BIT); break;
        default: usart_stop_bit_set(dev, USART_STB_1BIT); break;
    }

    /* 使能发送/接收 */
    usart_transmit_config(dev, USART_TRANSMIT_ENABLE);
    usart_receive_config(dev, USART_RECEIVE_ENABLE);
    usart_enable(dev);
}

/*
 * 功能描述：此函数关闭串口
 * 入参说明：dev --- UARTx（x=0至7）
 * 返 回 值：无
 */
void gd32_uart_shutdown(uint32_t dev) {
    if (dev == USART0) {
        rcu_periph_clock_disable(RCU_USART0);
    } else if (dev == USART1) {
        rcu_periph_clock_disable(RCU_USART1);
    } else if (dev == USART2) {
        rcu_periph_clock_disable(RCU_USART2);
    } else if (dev == UART3) {
        rcu_periph_clock_disable(RCU_UART3);
    } else if (dev == UART4) {
        rcu_periph_clock_disable(RCU_UART4);
    } else if (dev == USART5) {
        rcu_periph_clock_disable(RCU_USART5);
    } else if (dev == UART6) {
        rcu_periph_clock_disable(RCU_UART6);
    } else if (dev == UART7) {
        rcu_periph_clock_disable(RCU_UART7);
    }
}

/*
 * 功能描述：此函数向串口发送一个字符
 * 入参说明：dev --- UARTx（x=0至7）
 *           ch --- 要发送的字符
 * 返 回 值：无
 */
void gd32_uart_putch(uint32_t dev, int ch) {
    /* 等待直到传输准备好 */
    while (!gd32_uart_tx_rdy(dev));

    /* 发送字符 */
    gd32_uart_tx(dev, ch);
}

/*
 * 功能描述：此函数向串口发送一个字节流
 * 入参说明：dev --- UARTx（x=0至7）
 *           data --- 数据地址
 *           len --- 数据长度
 * 返 回 值：无
 */
void gd32_uart_put(uint32_t dev, const uint8_t *data, int len) {
    while (len-- != 0) {
        gd32_uart_putch(dev, *data++);
    }
}

/*
 * 功能描述：此函数向串口发送一个以'\0'结尾的字符串
 * 入参说明：dev --- UARTx（x=0至7）
 *           str --- 要发送的字符串
 * 返回值：无
 */
void gd32_uart_puts(uint32_t dev, const char *str) {
    while (*str) {
        gd32_uart_putch(dev, *str++);
    }
}

/*
 * 功能描述：此函数从串口接收一个字符（等待直到接收到一个字符）
 * 入参说明：dev --- UARTx（x=0至7）
 * 返 回 值：接收到的字符
 */

int gd32_uart_getch(uint32_t dev) {
    /* 等待直到接收准备好 */
    while (!gd32_uart_rx_rdy(dev));

    /* 返回接收字符 */
    return gd32_uart_rx(dev);
}

/*
 * 功能描述：此函数从串口接收一个字符，带有超时设置
 * 入参说明：dev --- UARTx（x=0至7）
 *           ch --- 指向接收字符的指针
 *           timeo --- 字符间的超时值，单位为毫秒
 * 返回值：0 --- 成功  其他 --- 超时
 */
int gd32_uart_getch_timeout(uint32_t dev, int *ch, uint32_t timeo) {
    timeo = timeo * 10;

    while (timeo != 0) { /* 等待接收准备好 */
        if (gd32_uart_rx_rdy(dev)) {
            break;
        }

        gd32_delay_us(100); /* 0.1ms */
        timeo--;
    }

    if (timeo == 0) {
        return -1;
    }

    *ch = gd32_uart_rx(dev);

    return 0; /* 成功 */
}

/*
 * 功能描述：此函数填充串口发送所需的DMA传输信息
 * 入参说明：dev --- UARTx（x=0至7）
 *           xfer --- 指向dma_xfer_t结构体的指针，该结构体保存DMA传输信息
 * 返 回 值：无
 */
void gd32_uart_fill_txdma(uint32_t dev, dma_xfer_t *xfer) {
    if (dev == USART0) { /* UART0发送：DMA1通道7外设4 */
        xfer->dmac    = DMA1;
        xfer->channel = DMA_CH7;
        xfer->periph  = DMA_SUBPERI4;
        xfer->irqn    = DMA1_Channel7_IRQn;
    } else if (dev == USART1) { /* UART1发送：DMA0通道6外设4 */
        xfer->dmac    = DMA0;
        xfer->channel = DMA_CH6;
        xfer->periph  = DMA_SUBPERI4;
        xfer->irqn    = DMA0_Channel6_IRQn;
    } else if (dev == USART2) {
#if UART2_TXDMA_CHANNEL == 3 /* UART2发送：DMA0通道3外设4 */
        xfer->dmac    = DMA0;
        xfer->channel = DMA_CH3;
        xfer->periph  = DMA_SUBPERI4;
        xfer->irqn    = DMA0_Channel3_IRQn;
#elif UART2_TXDMA_CHANNEL == 4 /* UART2发送：DMA0通道4外设7 */
        xfer->dmac    = DMA0;
        xfer->channel = DMA_CH4;
        xfer->periph  = DMA_SUBPERI7;
        xfer->irqn    = DMA0_Channel4_IRQn;
#endif                         /* UART2_TXDMA_CHANNEL */
    } else if (dev == UART3) { /* UART3发送：DMA0通道4外设4 */
        xfer->dmac    = DMA0;
        xfer->channel = DMA_CH4;
        xfer->periph  = DMA_SUBPERI4;
        xfer->irqn    = DMA0_Channel4_IRQn;
    } else if (dev == UART4) { /* UART4发送：DMA0通道7外设4 */
        xfer->dmac    = DMA0;
        xfer->channel = DMA_CH7;
        xfer->periph  = DMA_SUBPERI4;
        xfer->irqn    = DMA0_Channel7_IRQn;
    } else if (dev == USART5) {
#if UART5_TXDMA_CHANNEL == 6 /* UART5发送：DMA1通道6外设5 */
        xfer->dmac    = DMA1;
        xfer->channel = DMA_CH6;
        xfer->periph  = DMA_SUBPERI5;
        xfer->irqn    = DMA1_Channel6_IRQn;
#elif UART5_TXDMA_CHANNEL == 7 /* UART5发送：DMA1通道7外设5 */
        xfer->dmac    = DMA1;
        xfer->channel = DMA_CH7;
        xfer->periph  = DMA_SUBPERI5;
        xfer->irqn    = DMA1_Channel7_IRQn;
#endif                         /* UART5_TXDMA_CHANNEL */
    } else if (dev == UART6) { /* UART6发送：DMA0通道1外设5 */
        xfer->dmac    = DMA0;
        xfer->channel = DMA_CH1;
        xfer->periph  = DMA_SUBPERI5;
        xfer->irqn    = DMA0_Channel1_IRQn;
    } else if (dev == UART7) { /* UART7发送：DMA0通道0外设5 */
        xfer->dmac    = DMA0;
        xfer->channel = DMA_CH0;
        xfer->periph  = DMA_SUBPERI5;
        xfer->irqn    = DMA0_Channel0_IRQn;
    }

    xfer->paddr = (uint32_t)(&USART_DATA(dev)); /* 外设地址：UARTx->DATA 地址 */
    xfer->width = 1;                            /* 传输宽度为1字节 */
    xfer->minc  = 1;                            /* 内存指针增加 */
    xfer->pinc  = 0;                            /* 外设指针不增加 */
    xfer->dir   = DMA_MEMORY_TO_PERIPH;         /* 传输方向：内存到外设 */
    xfer->flow  = DMA_FLOW_CONTROLLER_DMA;      /* 流控制器：DMAC */
}

/*
 * 功能描述：此函数填充串口接收所需的DMA传输信息
 * 入参说明：dev --- UARTx（x=0至7）
 *           xfer --- 指向dma_xfer_t结构体的指针，该结构体保存DMA传输信息
 * 返 回 值：无
 */
void gd32_uart_fill_rxdma(uint32_t dev, dma_xfer_t *xfer) {
    if (dev == USART0) {
#if UART0_RXDMA_CHANNEL == 2 /* UART0接收：DMA1通道2外设4 */
        xfer->dmac    = DMA1;
        xfer->channel = DMA_CH2;
        xfer->periph  = DMA_SUBPERI4;
        xfer->irqn    = DMA1_Channel2_IRQn;
#elif UART0_RXDMA_CHANNEL == 5  /* UART0接收：DMA1通道5外设4 */
        xfer->dmac    = DMA1;
        xfer->channel = DMA_CH5;
        xfer->periph  = DMA_SUBPERI4;
        xfer->irqn    = DMA1_Channel5_IRQn;
#endif                          /* UART0_RXDMA_CHANNEL  */
    } else if (dev == USART1) { /* UART1接收：DMA0通道5外设4 */
        xfer->dmac    = DMA0;
        xfer->channel = DMA_CH5;
        xfer->periph  = DMA_SUBPERI4;
        xfer->irqn    = DMA0_Channel5_IRQn;
    } else if (dev == USART2) { /* UART2接收：DMA0通道1外设4 */
        xfer->dmac    = DMA0;
        xfer->channel = DMA_CH1;
        xfer->periph  = DMA_SUBPERI4;
        xfer->irqn    = DMA0_Channel1_IRQn;
    } else if (dev == UART3) { /* UART3接收：DMA0通道2外设4 */
        xfer->dmac    = DMA0;
        xfer->channel = DMA_CH2;
        xfer->periph  = DMA_SUBPERI4;
        xfer->irqn    = DMA0_Channel2_IRQn;
    } else if (dev == UART4) { /* UART4接收：DMA0通道0外设4 */
        xfer->dmac    = DMA0;
        xfer->channel = DMA_CH0;
        xfer->periph  = DMA_SUBPERI4;
        xfer->irqn    = DMA0_Channel0_IRQn;
    } else if (dev == USART5) {
#if UART5_RXDMA_CHANNEL == 1 /* UART5接收：DMA1通道1外设5 */
        xfer->dmac    = DMA1;
        xfer->channel = DMA_CH1;
        xfer->periph  = DMA_SUBPERI5;
        xfer->irqn    = DMA1_Channel1_IRQn;
#elif UART5_RXDMA_CHANNEL == 2 /* UART5接收：DMA1通道2外设5 */
        xfer->dmac    = DMA1;
        xfer->channel = DMA_CH2;
        xfer->periph  = DMA_SUBPERI5;
        xfer->irqn    = DMA1_Channel2_IRQn;
#endif                         /* UART5_RXDMA_CHANNEL */
    } else if (dev == UART6) { /* UART6接收：DMA0通道3外设5 */
        xfer->dmac    = DMA0;
        xfer->channel = DMA_CH3;
        xfer->periph  = DMA_SUBPERI5;
        xfer->irqn    = DMA0_Channel3_IRQn;
    } else if (dev == UART7) { /* UART7接收：DMA0通道6外设5 */
        xfer->dmac    = DMA0;
        xfer->channel = DMA_CH6;
        xfer->periph  = DMA_SUBPERI5;
        xfer->irqn    = DMA0_Channel6_IRQn;
    }

    xfer->paddr = (uint32_t)(&USART_DATA(dev)); /* 外设地址：UARTx->DATA地址 */
    xfer->width = 1;                            /* 传输宽度为1字节 */
    xfer->minc  = 1;                            /* 内存指针增加 */
    xfer->pinc  = 0;                            /* 外设指针不增加 */
    xfer->dir   = DMA_PERIPH_TO_MEMORY;         /* 传输方向：外设到内存 */
    xfer->flow  = DMA_FLOW_CONTROLLER_DMA;      /* 流控制器：DMAC */
}

/*
 * 功能描述：此函数初始化串口外设
 * 入参说明：dev --- UARTx（x=0至7）
 * 返回值：无
 */
static void init_peripheral(uint32_t dev) {
    if (dev == USART0) {
        rcu_periph_clock_enable(RCU_USART0);
        rcu_periph_reset_enable(RCU_USART0RST);
        rcu_periph_reset_disable(RCU_USART0RST);
    } else if (dev == USART1) {
        rcu_periph_clock_enable(RCU_USART1);
        rcu_periph_reset_enable(RCU_USART1RST);
        rcu_periph_reset_disable(RCU_USART1RST);
    } else if (dev == USART2) {
        rcu_periph_clock_enable(RCU_USART2);
        rcu_periph_reset_enable(RCU_USART2RST);
        rcu_periph_reset_disable(RCU_USART2RST);
    } else if (dev == UART3) {
        rcu_periph_clock_enable(RCU_UART3);
        rcu_periph_reset_enable(RCU_UART3RST);
        rcu_periph_reset_disable(RCU_UART3RST);
    } else if (dev == UART4) {
        rcu_periph_clock_enable(RCU_UART4);
        rcu_periph_reset_enable(RCU_UART4RST);
        rcu_periph_reset_disable(RCU_UART4RST);
    } else if (dev == USART5) {
        rcu_periph_clock_enable(RCU_USART5);
        rcu_periph_reset_enable(RCU_USART5RST);
        rcu_periph_reset_disable(RCU_USART5RST);
    } else if (dev == UART6) {
        rcu_periph_clock_enable(RCU_UART6);
        rcu_periph_reset_enable(RCU_UART6RST);
        rcu_periph_reset_disable(RCU_UART6RST);
    } else if (dev == UART7) {
        rcu_periph_clock_enable(RCU_UART7);
        rcu_periph_reset_enable(RCU_UART7RST);
        rcu_periph_reset_disable(RCU_UART7RST);
    }
}
