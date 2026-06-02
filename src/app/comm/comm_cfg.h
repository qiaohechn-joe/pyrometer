/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：comm_cfg.h
 * 文件描述：通信端口配置及发送接口定义
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/02/13 李兆越 初始版本
 *           V1.1 2025/04/23 李兆越 加入按块发送函数
 *           V1.2 2025/05/12 李兆越 命名通用化与结构清晰化
 */

#ifndef _COMM_CFG_H_
#define _COMM_CFG_H_

#include <stdint.h>
#include "pc.h"
#include "tcp_server.h"

/* 端口支持的波特率数量 */
#define PORT_BAUD_CNT 5
extern uint32_t PORT_BAUD_TAB[PORT_BAUD_CNT];

/* 通信通道结构体：表示一个通信端口或连接，如 COM1、TCP client */
typedef struct {
    int port_id; /* 通道编号，例如 COM1 -> 1 */
} comm_channel_t;

/* 通信分组结构体：如 UART 组、TCP 组等 */
typedef struct {
    const char     *group_name;                                    /* 分组名称（如 "UART", "TCP"） */
    comm_channel_t *channels;                                      /* 通道数组（动态分配） */
    int             channel_count;                                 /* 通道数量 */
    void (*send_func)(int channel_idx, const void *data, int len); /* 分组的发送函数 */
} comm_group_t;

/* 通信管理器结构体：统一管理所有通信分组 */
typedef struct {
    comm_group_t *groups;      /* 所有分组数组（动态分配） */
    int           group_count; /* 分组总数 */
} comm_manager_t;

typedef struct {
    uint8_t group_idx;   /* 通信分组索引，例如 UART/TCP/CAN 等 */
    uint8_t channel_idx; /* 分组内的通道编号，例如 232/485/422 等 */
} comm_port_t;

extern comm_manager_t comm_mgr; /* 全局通信管理器 */

/* 初始化通信模块 */
void comm_init(void);

/* 各通信分组的发送包装函数 */
void pc_send_wrapper(int channel_idx, const void *data, int len);
void tcp_server_send_wrapper(int channel_idx, const void *data, int len);

/* 按块方式发送数据 */
void comm_send_chunk(int group_idx, int channel_idx, const void *data, int len);

/* 协议处理函数 */
void proto_process(char *rx_buff, char *tx_buff, int rx_len, int *tx_len, comm_port_t *comm_port);

/* 通信分组及通道管理接口 */
int comm_group_create(const char *group_name, void (*send_func)(int, const void *, int));
int comm_channel_add(int group_idx, int port_id);
int comm_channel_remove(int group_idx, int port_id);
int comm_group_find(const char *group_name);

/* 分组全局索引标识（在 comm_init 中赋值） */
extern int uart_group;
extern int tcp_group;

/* 通信管理器互斥锁 */
extern SemaphoreHandle_t comm_mgr_mutex;

#endif /* _COMM_CFG_H_ */
