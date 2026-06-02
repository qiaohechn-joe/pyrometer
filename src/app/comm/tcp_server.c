/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：tcp_server.c
 * 文件描述：TCP服务端管理代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/09/02 陈  军  初始版本
 *           V1.1 2025/03/05 李兆越  适配LED102项目
 */

#include "lwip.h"
#include "agent.h"
#include "thread.h"
#include "system.h"
#include "camera.h"
#include "ostimer.h"
#include "tcp_server.h"
#include "lwip/tcp.h"
#include "burst.h"

#define MAX_SEND_SIZE   1460 /* 根据MTU调整适合的发送最大尺寸 */
#define MAX_RETRY_COUNT 3    /* 定义最大重试次数 */

ALIGN(4) static char server_rx_buffer[PROT_MAX_PKT_LEN];
ALIGN(4) static char server_tx_buffer[PROT_MAX_PKT_LEN];

volatile uint8_t tcp_uvc_busy_flag = 0;

static comm_port_t tcp_port;

/* 指令处理函数指针 */
typedef int (*cmd_handler_t)(client_t *client, void *buff, int len);

static server_context_t work_context = {.thread_num      = SERVER_THREAD_BASE,
                                        .listen_port     = LISTEN_PORT_WORK,
                                        .client_base_num = WORK_CLIENT_BASE,
                                        .server_state    = SERVER_STATE_WAIT_LINK};

static server_context_t camera_context = {.thread_num      = SERVER_THREAD_BASE + 1,
                                          .listen_port     = LISTEN_PORT_CAMERA,
                                          .client_base_num = CAMERA_CLIENT_BASE,
                                          .server_state    = SERVER_STATE_WAIT_LINK};

/* 内部函数 */
static void  server_task(void *arg);
static void  client_thread(void *arg);
static void  get_ipconfig_info(void);
static int   parse_ip_string(const char *src, uint8_t *dst);
static err_t mycallback(void *arg, struct tcp_pcb *tpcb, u16_t len);
/*
 * 功能描述：这个函数初始化TCP服务端
 * 入参说明：无
 * 返 回 值：无
 * 备    注：网络配置最前
 */
void tcpsvr_init(void) {

    /* 网络参数配置项更新 */
    get_ipconfig_info();
}

/*
 * 功能描述：TCP服务器启动，监听多个端口
 * 入参说明：无
 * 返 回 值：无
 */
void tcpsvr_start(void) {
    if (sys_para.net.port < 65536) {
        work_context.listen_port = sys_para.net.port;
    }

    if (thread_create(work_context.thread_num, server_task, &work_context, SERVER_THREAD_STACK_SIZE)) {
        dbg_net("NET: Work thread %d created successfully.\r\n", work_context.thread_num);
    } else {
        dbg_error("NET: Work thread %d creation failed!\r\n", work_context.thread_num);
    }

    if (thread_create(camera_context.thread_num, server_task, &camera_context, SERVER_THREAD_STACK_SIZE)) {
        dbg_net("NET: Camera thread %d created successfully.\r\n", camera_context.thread_num);
    } else {
        dbg_error("NET: Camera thread %d creation failed!\r\n", camera_context.thread_num);
    }
}

static err_t mycallback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    printf("Test......len = %d\r\n", len);

    //    if (len < 2000 )
    //    {
    //        tcp_uvc_busy_flag = 0 ;
    //    }
}

/*
 * 功能描述：TCP服务器状态机任务，管理客户端连接
 * 入参说明：arg --- 入参(强转为监听端口号)
 * 返 回 值：无
 */
/*
 * 功能描述：TCP服务器任务，监听端口并接受客户端连接
 * 入参说明：arg --- 指向 server_context_t 上下文结构体
 * 返 回 值：无
 */
static void server_task(void *arg) {
    server_context_t *context  = (server_context_t *)arg;
    uint16_t          port     = context->listen_port; /* 当前监听端口 */
    struct netconn   *conn     = NULL;                 /* 监听连接对象 */
    struct netconn   *new_conn = NULL;                 /* 新接受的客户端连接 */
    ip_addr_t         client_addr;
    u16_t             client_port;
    err_t             err;
    TaskHandle_t      self = xTaskGetCurrentTaskHandle();

    server_state_t *state = &context->server_state;

    while (1) {
        switch (*state) {
            case SERVER_STATE_WAIT_LINK:
                /* 检查网络是否就绪，未就绪则延迟等待 */
                if (!netif_is_link_up(netif_default)) {
                    vTaskDelay(configTICK_RATE_HZ);
                } else {
                    *state = SERVER_STATE_CREATE;
                }
                break;

            case SERVER_STATE_CREATE:
                /* 创建 TCP 服务连接 */
                conn = netconn_new(NETCONN_TCP);
                if (conn == NULL) {
                    dbg_error("NET: TCP-Server create failed! port=%d\r\n", port);
                    *state = SERVER_STATE_RESTART;
                } else {
                    conn->recv_timeout = 1000; /* 设置接收超时 1000ms */
                    netconn_set_recvtimeout(conn, conn->recv_timeout);
                    *state = SERVER_STATE_BIND;
                }
                break;

            case SERVER_STATE_BIND:
                /* 将连接绑定到指定端口 */
                err = netconn_bind(conn, NULL, port);
                if (err != ERR_OK) {
                    dbg_error("NET: TCP-Server bind failed! port=%d, err=%d\r\n", port, err);
                    netconn_delete(conn);
                    conn   = NULL;
                    *state = SERVER_STATE_RESTART;
                } else {
                    *state = SERVER_STATE_LISTEN;
                }
                break;

            case SERVER_STATE_LISTEN:
                /* 开始监听 TCP 连接 */
                err = netconn_listen(conn);
                if (err != ERR_OK) {
                    dbg_error("NET: TCP-Server listen failed! port=%d, err=%d\r\n", port, err);
                    netconn_delete(conn);
                    conn   = NULL;
                    *state = SERVER_STATE_RESTART;
                } else {
                    dbg_net("NET: TCP Server listening on port %d...\r\n", port);
                    *state = SERVER_STATE_ACCEPT;
                }
                break;

            case SERVER_STATE_ACCEPT: {
                /* 接收客户端连接请求 */
                err = netconn_accept(conn, &new_conn);
                if (err == ERR_TIMEOUT) {
                    continue;
                } else if (err == ERR_ABRT) {
                    dbg_error("NET: -13!Connection aborted;continue;\r\n");
                    break;
                } else if (err != ERR_OK) {
                    dbg_error("NET: Accept failed! port=%d, err=%d\r\n", port, err);
                    *state = SERVER_STATE_RESTART;
                    break;
                }

                /* 分配一个空闲的客户端槽位 */
                client_t *client = NULL;
                for (int i = 0; i < MAX_CLIENT_COUNT; i++) {
                    if (!context->client_list[i].online) {
                        client      = &context->client_list[i];
                        client->num = context->client_base_num + i;
                        break;
                    }
                }

                if (!client) {
                    dbg_error("NET: Too many clients on port %d\r\n", port);
                    netconn_close(new_conn);
                    netconn_delete(new_conn);
                    break;
                }

                /* 初始化客户端信息 */
                client->conn        = new_conn;
                client->online      = 1;
                client->listen_port = port;
                netconn_getaddr(new_conn, &client_addr, &client_port, 0);
                client->ip[0] = client_addr.addr & 0xff;
                client->ip[1] = (client_addr.addr >> 8) & 0xff;
                client->ip[2] = (client_addr.addr >> 16) & 0xff;
                client->ip[3] = (client_addr.addr >> 24) & 0xff;
                client->port  = client_port;
                client->state = CLIENT_STATE_INIT;

                dbg_net("NET: Client connected from %d.%d.%d.%d:%d on port %d\r\n", client->ip[0], client->ip[1], client->ip[2], client->ip[3],
                        client->port, port);

                /* 添加到通信通道管理 */
                comm_channel_add(tcp_group, client->num);

                /* 启动客户端处理线程 */
                if (thread_create(client->num, client_thread, client, CLIENT_THREAD_STACK_SIZE)) {

                    if (80 == port) {
                        //                        ostimer_open(OSTIMER_DATA_SEND_TASK, 1);
                        timer_enable(TIMER11);
                    }

                    // tcp_sent(client->conn->pcb.tcp, mycallback);

                    dbg_net("NET: Client thread %d created successfully.\r\n", client->num);
                    context->client_count++;
                } else {
                    dbg_error("NET: Client thread %d creation failed!\r\n", client->num);
                }
                break;
            }

            case SERVER_STATE_RESTART:
                /* 关闭并清理旧连接，准备重启服务器任务 */
                if (conn) {
                    netconn_close(conn);
                    netconn_delete(conn);
                    conn = NULL;
                }

                dbg_net("NET: Server task restarting for port %d\r\n", port);

                /* 创建新服务器任务并删除当前任务 */
                if (thread_create(context->thread_num, server_task, context, SERVER_THREAD_STACK_SIZE)) {
                    vTaskDelay(pdMS_TO_TICKS(10)); // 等待新任务启动
                    vTaskDelete(self);             // 删除自身
                } else {
                    dbg_error("NET: Server task restart failed!\r\n");
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
                break;
        }
    }
}

/*
 * 功能描述：这个函数处理客户端指令，基于状态机方式处理接收与协议解析
 * 入参说明：arg --- 入参(客户端指针)
 * 返 回 值：无
 */
static void client_thread(void *arg) {

    err_t           err;
    struct netbuf  *buff   = NULL;
    void           *data   = NULL;
    u16_t           len    = 0;
    int             outlen = 0;
    client_t       *client = (client_t *)arg;
    client_state_t *state  = &client->state; /* 指向client结构体中的状态变量 */
    TaskHandle_t    self   = xTaskGetCurrentTaskHandle();

    while (1) {
        switch (*state) {
            case CLIENT_STATE_INIT:
                /* 设置接收超时时间 */
                client->conn->recv_timeout = 100;
                netconn_set_recvtimeout(client->conn, client->conn->recv_timeout);

                /* 每次新连接默认关闭网口突发，需收到V=B后再打开 */
                if (client->listen_port == work_context.listen_port) {
                    burst_clear_tcp_enable();
                }

                /* 新连接默认关闭网口突发，需网口V=B再开启 */
                if (client->listen_port == work_context.listen_port) {
                    burst_clear_tcp_enable();
                }

                client->offline_cntr = 0;
                dbg_net("NET: CLIENT #%d wait command...\r\n", client->num);

#if 0 /* 自动发送 */
                if (client->num >= CAMERA_CLIENT_BASE && client->num <= CAMERA_CLIENT_END) {
                    sys_state.camera_flag_open_send = 1;
                    sys_state.camera_client_num     = client->num;
                }
#endif
                if (client->listen_port == 80) {
                    sys_state.camera_client_num = client->num;
                }

                *state = CLIENT_STATE_WAIT_DATA;
                break;

            case CLIENT_STATE_WAIT_DATA:
                // client->offline_cntr++; /* 需要添加"心跳包"支持 */

                /* 检查网络接口是否在线 */
                if (netif_is_up(netif_default)) {
                    client->offline_cntr = 0;
                } else {
                    client->offline_cntr++;
                }

                if (client->offline_cntr >= CLIENT_OFFLINE_TIMEO) {
                    *state = CLIENT_STATE_DISCONNECT;
                    break;
                }

                /* 接收数据 */
                err = netconn_recv(client->conn, &buff);
                if (err == ERR_TIMEOUT) {
                    break; /* 继续等待 */
                } else if (err != ERR_OK) {
                    dbg_error("NET: CLIENT #%d recv failed! err=%d\r\n", client->num, err);
                    *state = CLIENT_STATE_DISCONNECT;
                    break;
                }

                // client->offline_cntr = 0; /* 清除心跳包计数 */
                netbuf_data(buff, &data, &len);

                if (data != NULL && len > 0) {
                    *state = CLIENT_STATE_PROCESS;
                } else {
                    netbuf_delete(buff);
                    buff = NULL;
                }
                break;

            case CLIENT_STATE_PROCESS:
                dbg_net("NET: CLIENT #%d received %d bytes\r\n", client->num, len);
                // dbg_dump_net("", data, len, 64);

                /* 拷贝数据 */
                mem_copy(server_rx_buffer, data, len);
                tcp_port.group_idx   = tcp_group;
                tcp_port.channel_idx = client->num;

                /* 根据端口处理不同协议 */
                if (client->listen_port == 80) {
                    process_http_request(data, len, client);
                    // 启动传输任务
                    // ostimer_open(OSTIMER_DATA_SEND_TASK, 1);
                } else {
                    /* 协议处理 */ // todo  comm_port待去掉，在调用处处理发送。
                    ENTER_CRITICAL();
                    proto_process(server_rx_buffer, server_tx_buffer, len, &outlen, &tcp_port);
                    EXIT_CRITICAL();
                }

                netbuf_delete(buff);
                buff   = NULL;
                *state = CLIENT_STATE_WAIT_DATA;
                break;

            case CLIENT_STATE_DISCONNECT:
                /* 移除通道管理 */
                comm_channel_remove(tcp_group, client->num);

                if (client->listen_port == work_context.listen_port) {
                    /* 断开时关闭网口突发 */
                    burst_clear_tcp_enable();
                    work_context.thread_num--;
                } else if (client->listen_port == camera_context.listen_port) {
                    camera_context.thread_num--;
                    timer_disable(TIMER11);
                    //                    printf("Timer11 closed \r\n");
                }

                client->online = 0;

                /* 如果是相机通道，关闭发送标志 */
                if (client->num >= CAMERA_CLIENT_BASE && client->num <= CAMERA_CLIENT_END) {
                    sys_state.camera_flag_open_send = 0;
                }

                /* 关闭 TCP 连接并释放资源 */
                if (client->conn) {
                    netconn_close(client->conn);
                    netconn_delete(client->conn);
                    client->conn = NULL;
                }

                dbg_net("NET: CLIENT #%d disconnected and cleaned up.\r\n", client->num);

                //                if (client->listen_port == 80) {
                //                    // 停止传输任务
                //                    ostimer_close(OSTIMER_DATA_SEND_TASK);
                //                }

                vTaskDelete(self); /* 删除当前任务 */
                break;

            default:
                dbg_error("NET: CLIENT #%d unknown state=%d\r\n", client->num, state);
                *state = CLIENT_STATE_DISCONNECT;
                break;
        }
    }
}

/*
 * 功能描述：处理 HTTP 请求，响应单帧视频图像
 * 入参说明：data --- 接收到的数据指针
 *          len  --- 数据长度
 *          client --- 客户端信息
 * 返 回 值：无
 */
void process_http_request(void *data, int len, client_t *client) {
    char *req = (char *)data;

    /* 简单检查是否为 GET 请求 */
    if (len < 5 || strncmp(req, "GET ", 4) != 0) {
        return;
    }

    /* 检查是否为 /camera?action=stream */
    if (strstr(req, "/camera?action=stream") != NULL) {

        /* 发送 HTTP 头部响应，表明是 multipart 数据 */
        const char *http_header = "HTTP/1.0 200 OK\r\n"
                                  "Connection: close\r\n"
                                  "Server: EN-Streamer/1.2\r\n"
                                  "Cache-Control: no-store, no-cache, must-revalidate\r\n"
                                  "Pragma: no-cache\r\n"
                                  "Expires: 0\r\n"
                                  "Content-Type: multipart/x-mixed-replace;boundary=boundarydonotcross\r\n\r\n";
        // tcp_server_send(client->num, (char *)http_header, strlen(http_header));

        tcp_server_send_low(client->num, (char *)http_header, strlen(http_header));

        /* 开启发送 */
        client->flag_send_open          = 1;
        sys_state.camera_flag_open_send = 1;
        sys_state.camera_client_num     = client->num;
    }
}

/*
 * 功能描述：向指定客户端发送数据，不分包、不重试
 * 入参说明：client_num --- 客户端编号
 *          buff       --- 发送缓冲区指针
 *          outlen     --- 发送数据长度
 * 返 回 值：0表示成功，-1表示失败
 */
int tcp_server_send_low(int client_num, const void *buff, int outlen) {
    int len;
    if (!netif_is_up(netif_default)) return -1;
    if (client_num < 0 || client_num >= USER_THREAD_CNT) return -1;

    server_context_t *ctx = NULL;
    if (client_num >= WORK_CLIENT_BASE && client_num < CAMERA_CLIENT_BASE) {
        ctx = &work_context;
    } else if (client_num >= CAMERA_CLIENT_BASE && client_num <= CAMERA_CLIENT_END) {
        ctx = &camera_context;
    } else {
        return -1;
    }

    int local_index = client_num - ctx->client_base_num;
    if (local_index < 0 || local_index >= ctx->client_count) return -1;

    struct netconn *conn = ctx->client_list[local_index].conn;
    if (!conn) return -1;

    tcp_uvc_busy_flag = 1;
    /* 只发送一次，直接返回结果 tcp_write(pcb, data, len, TCP_WRITE_FLAG_COPY) */
    // err_t err = netconn_write(conn, buff, outlen, NETCONN_NOFLAG); // NETCONN_NOFLAG  TCP_WRITE_FLAG_COPY
    conn->pcb.tcp->flags |= TF_NODELAY;

    len = tcp_sndbuf(conn->pcb.tcp);

    if (len < 23360) return -1;

    // err_t err = tcp_write(conn->pcb.tcp, buff, outlen, 0x01);
    err_t err = tcp_write(conn->pcb.tcp, buff, outlen, 0x00);

    if (err == ERR_OK) tcp_output(conn->pcb.tcp);
    return (err == ERR_OK) ? 0 : -1;
}

/*
 * 功能描述：向指定客户端发送数据，支持分包、重试机制，连接中断保护
 * 入参说明：client_num --- 客户端编号
 *          buff       --- 发送缓冲区指针
 *          outlen     --- 发送数据长度
 * 返 回 值：0表示成功，-1表示失败
 */
int tcp_server_send(int client_num, const void *buff, int outlen) {
    /* 提前检查网络状态和客户端编号有效性 */
    if (!netif_is_up(netif_default)) return -1;
    if (client_num < 0 || client_num >= USER_THREAD_CNT) return -1;

    server_context_t *ctx = NULL;

    /* 判断客户端编号所属的上下文 */
    if (client_num >= WORK_CLIENT_BASE && client_num < CAMERA_CLIENT_BASE) {
        ctx = &work_context;
    } else if (client_num >= CAMERA_CLIENT_BASE && client_num <= CAMERA_CLIENT_END) {
        ctx = &camera_context;
    } else {
        return -1; /* 无效客户端编号 */
    }

    /* 转换为相对索引并检查范围有效性 */
    int local_index = client_num - ctx->client_base_num;
    if (local_index < 0 || local_index >= ctx->client_count) return -1;

    /* 检查连接是否有效 */
    struct netconn *conn = ctx->client_list[local_index].conn;
    if (conn == NULL) {
        return -1; /* 无效连接 */
    }

    const char *data       = (const char *)buff;
    int         bytes_sent = 0;
    int         remaining  = outlen;

    /* 发送数据循环，支持分包和重试机制 */
    while (remaining > 0) {
        int   send_size = (remaining > MAX_SEND_SIZE) ? MAX_SEND_SIZE : remaining;
        err_t err;
        int   retry_count = 0;

        do {
            /* 发送数据，分包时使用 NETCONN_MORE 标志 */
            err = netconn_write(conn, data + bytes_sent, send_size, (remaining <= MAX_SEND_SIZE) ? NETCONN_COPY : (NETCONN_COPY | NETCONN_MORE));
            if (err == ERR_OK) break; /* 成功发送时立即退出重试 */

            dbg_error("ERROR: Send failed, err %d, Retry %d/%d\r\n", err, retry_count + 1, MAX_RETRY_COUNT);
            retry_count++;

            /* 如果是不可恢复的错误，直接退出 */
            if (err == ERR_ABRT || err == ERR_CLSD || err == ERR_CONN) {
                dbg_net("ERROR: Irrecoverable error %d, aborting send\r\n", err);
                return -1;
            }
        } while (retry_count < MAX_RETRY_COUNT);

        /* 重试失败后退出 */
        if (err != ERR_OK) {
            dbg_error("ERROR: Send failed after %d retries, err %d\r\n", MAX_RETRY_COUNT, err);
            return -1;
        }

        /* 更新发送状态 */
        bytes_sent += send_size;
        remaining -= send_size;
    }

    /* 释放锁并返回成功 */
    return 0;
}

/*
 * 功能描述：这个函数处理tp1协议的这个命令：CMD_NET_MAC
 * 入参说明：op --- 操作类型
 *           data --- 指向包有效载荷
 *           len --- 有效载荷长度
 *           pkt --- 输出包缓冲
 * 返 回 值：输出包长度
 */
int cmd_net_mac(int cmd, int op, void *data, int len, void *pkt) {
    int     ret = 0;
    uint8_t device_mac[6]; /* 以太网MAC地址 */
    if (op == PROT_OP_ACK) {
        uint8_t *mac = gd32_eth_get_mac();
        mem_copy(device_mac, mac, 6);

        // ret = proto_base_build_pkt(pkt, DEV_ID, cmd, PROT_OP_RD, device_mac, sizeof(device_mac));
    }
    return ret;
}

/*
 * 功能描述：获取IP配置信息（从sys_para中解析出IP等信息存到sys_state中）
 * 入参说明：无
 * 返 回 值：无
 */
void get_ipconfig_info(void) {
    int ret1, ret2, ret3, ret4;

    ret1 = parse_ip_string((char *)sys_para.net.remote_ip, sys_state.remote_ip);
    ret2 = parse_ip_string((char *)sys_para.net.ip_addr, sys_state.ip_addr);
    ret3 = parse_ip_string((char *)sys_para.net.gateway, sys_state.gateway);
    ret4 = parse_ip_string((char *)sys_para.net.net_mask, sys_state.net_mask);

    /* 简单校验 remote_ip[0] == 0 则视为非法，恢复默认 */
    if ((ret1 != 0) || (sys_state.remote_ip[0] == 0)) {
        uint8_t default_remote_ip[4] = {192, 168, 42, 100};
        uint8_t default_ip_addr[4]   = {192, 168, 42, 132};
        uint8_t default_gateway[4]   = {192, 168, 42, 1};
        uint8_t default_netmask[4]   = {255, 255, 255, 0};

        memcpy(sys_state.remote_ip, default_remote_ip, sizeof(default_remote_ip));
        memcpy(sys_state.ip_addr, default_ip_addr, sizeof(default_ip_addr));
        memcpy(sys_state.gateway, default_gateway, sizeof(default_gateway));
        memcpy(sys_state.net_mask, default_netmask, sizeof(default_netmask));
    }

    return;
}

/*
 * 功能描述：将字符串IP解析为uint8_t数组（若非法则填0）
 * 入参说明：src - IP字符串，dst - 目标数组
 * 返 回 值：0=成功，-1=非法
 */
static int parse_ip_string(const char *src, uint8_t *dst) {
    unsigned int ip[4];

    if (sscanf(src, "%3u.%3u.%3u.%3u", &ip[0], &ip[1], &ip[2], &ip[3]) != 4) {
        memset(dst, 0, 4);
        return -1;
    }

    for (int i = 0; i < 4; i++) {
        if (ip[i] > 255) {
            memset(dst, 0, 4);
            return -1;
        }
        dst[i] = (uint8_t)ip[i];
    }

    return 0;
}
