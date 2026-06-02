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

#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include "platform.h"

#define SERVER_THREAD_STACK_SIZE 1024 /* 服务器线程栈大小（单位：word，即4字节） -> 256 * 4 = 1KB */
#define CLIENT_THREAD_STACK_SIZE 512  /* 客户端线程栈大小（单位：word，即4字节） -> 128 * 4 = 512B */

/* 重连间隔 */
#define RECONNECT_INTERVAL       3 /* 重连间隔，单位为秒 */

/* 超时时间 */
#define RECV_TIMEOUT_S           5 /* 接收超时时间，单位为秒 */

/*** SOP ***/
#define CMD_SOP                  "COMD  " /* 命令包 */
#define RESP_SOP                 "RESP  " /* 响应包 */
#define SOP_LEN                  6

/*** 响应 ***/
#define RESP_OK                  "OK"
#define RESP_OK_LEN              2

#define MAX_CLIENT_COUNT         4 /* 连接最大客户端个数 */
#define SERVER_THREAD_BASE       1 // 服务器监听任务用1-2
#define WORK_CLIENT_BASE         3 // 工作端口客户端3-6
#define CAMERA_CLIENT_BASE       7 // 相机端口客户端7-10
#define CAMERA_CLIENT_END        USER_THREAD_CNT

#define LISTEN_PORT_WORK         6363 /* 端口号 */
#define LISTEN_PORT_CAMERA       80   /* 端口号 */

/* 客户端离线时间 */
#define CLIENT_OFFLINE_TIMEO     30 /* 秒 */

#define SERVER_COM               11

/* 服务器状态机定义 */
typedef enum {
    SERVER_STATE_WAIT_LINK, /* 等待网口链路就绪 */
    SERVER_STATE_CREATE,    /* 创建 TCP 连接对象 */
    SERVER_STATE_BIND,      /* 绑定端口 */
    SERVER_STATE_LISTEN,    /* 开始监听 */
    SERVER_STATE_ACCEPT,    /* 接收客户端连接 */
    SERVER_STATE_RESTART,   /* 发生错误后重启任务 */
} server_state_t;
/* 连接客户端状态机定义 */
typedef enum {
    CLIENT_STATE_INIT = 0,  /* 初始化状态 */
    CLIENT_STATE_WAIT_DATA, /* 等待数据接收 */
    CLIENT_STATE_PROCESS,   /* 处理数据 */
    CLIENT_STATE_DISCONNECT /* 断开连接并清理 */
} client_state_t;

/*  网络客户端 */
typedef struct {
    int             online;       /* 在线标志 */
    int             num;          /* 序号 */
    int             offline_cntr; /* 断线计数器 */
    struct netconn *conn;
    uint8_t         ip[4];
    uint16_t        port;
    uint8_t         reply_buff[512];
    uint16_t        listen_port; /* 记录监听端口（服务器端口） */
    uint8_t         flag_send_open;
    client_state_t  state; /* 客户端状态变量 */
} client_t;

typedef struct {
    int            thread_num;
    uint16_t       listen_port;
    int            client_count;
    int            client_base_num;
    client_t       client_list[MAX_CLIENT_COUNT];
    server_state_t server_state; /* 加一个状态变量，用于调试观察 */
} server_context_t;

extern volatile uint8_t tcp_uvc_busy_flag;

void tcpsvr_init(void);
void tcpsvr_start(void);
int  tcp_server_send(int client_num, const void *buff, int outlen);
int  tcp_server_send_low(int client_num, const void *buff, int outlen);
void process_http_request(void *data, int len, client_t *client);
int  cmd_net_mac(int cmd, int op, void *data, int len, void *pkt);
#endif /* _TCP_SERVER_H_ */
