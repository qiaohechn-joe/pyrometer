/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：pc.c
 * 文件描述：PC接口业务代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#include "agent.h"
#include "ostimer.h"
#include "upgrade.h"
#include "pc.h"
#include "para.h"
#include "proto_char.h"

/* PC数据包缓存（4字节对齐） */
ALIGN(4) static char pc_tx_buffer[1056];
ALIGN(4) static char pc_rx_buffer[1056];

static comm_port_t pc_port;

/* 内部函数 */
static void pc_on_rx_data(void *arg);
static void pc_rx_packet_task(void *arg);

/*
 * 功能描述：这个函数初始化PC接口
 * 入参说明：无
 * 返 回 值：0 -- 成功，其它 -- 失败
 */
int pc_init(void) {

    if (!is_valid_baudrate(sys_para.port.baudrate)) {
        sys_para.port.baudrate = 115200; // 默认波特率
    }
    pc_port.group_idx    = uart_group;
    pc_port.channel_idx  = PC_COM;
    com_config_t com_cfg = {.baudrate    = sys_para.port.baudrate,
                            .rx_timeo    = PC_RX_TIMEOUT,
                            .fmt         = sys_para.port.check_fmt,
                            .port        = PC_COM,
                            .dma_cfg     = COM_RECV_DMA | COM_SEND_DMA,
                            .rx_callback = pc_on_rx_data,
                            .mode        = (rt_mode_e)sys_para.port.mode};

    com_init(&com_cfg);
    com_enable_recv(PC_COM);
    rcu_periph_clock_enable(RCU_CRC);
    return 0;
}

/*
 * 功能描述：这个函数用于PC发送数据
 * 入参说明：data --- 发送数据缓存区指针
 *           len --- 发送数据长度（字节数）
 * 返 回 值：无
 * 备    注：中断或DMA使用com_send()，轮询使用com_put()
 */
void pc_send(int port, const void *data, int len) {
    vTaskSuspendAll();
    com_send(port, data, len);
    xTaskResumeAll();
}

/*
 * 功能描述：这个函数是RS232接收回调
 * 入参说明：arg --- 入参
 * 返 回 值：无
 */
static void pc_on_rx_data(void *arg) {
    /* 接收过滤 */
    if (com_filter_recv(PC_COM, PC_MIN_LEN, PC_MAX_LEN) == 0) {
        agent_post(FROM_ISR, AGENT_PRIO_HI, pc_rx_packet_task, (void *)&pc_port);
    }
}

/*
 * 功能描述：这个函数处理来自PC的数据包（代理方式调用）
 * 入参说明：arg --- 入参
 * 返 回 值：无
 * 备    注：一个字节识别为空包不处理
 */
static void pc_rx_packet_task(void *arg) {
    // start_cycle_counter();           //!< 开始总计时
    int rx_len;
    int tx_len = 0;

    comm_port_t *comm_port = (comm_port_t *)arg;
    /* 读FIFO */
    rx_len = com_recv(comm_port->channel_idx, pc_rx_buffer, sizeof(pc_rx_buffer));
    if ((rx_len <= 1) || (rx_len > 1056)) {
        return;
    }

    ENTER_CRITICAL();
    /* 协议处理 */ // todo  comm_port待去掉，在调用处处理发送。
    proto_process(pc_rx_buffer, pc_tx_buffer, rx_len, &tx_len, comm_port);
    EXIT_CRITICAL();
    mem_set(pc_rx_buffer, sizeof(pc_rx_buffer), 0x00);
    // sys_state.long_cycles = time_shift_ms(stop_cycle_counter());  // 获取 start_cycle_counter 以来的时间
    // dbg_info("%f\r\n", sys_state.long_cycles);
}
