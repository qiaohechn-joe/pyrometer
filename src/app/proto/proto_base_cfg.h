/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：proto_base_cfg.h
 * 文件描述：和其通用协议配置代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/11/26 陈  军  初始版本
 *           V1.1 2025/02/13 李兆越  加入日志56号命令
 */

#ifndef _PROTO_BASE_CFG_H_
#define _PROTO_BASE_CFG_H_

#include "platform.h"
#include "proto_base.h"

/* 包长度定义 */
#define PROT_BASE_MIN_PKT_LEN MIN_PKT_LEN
#define PROT_BASE_MAX_PKT_LEN (1536)

/* 接口类型 */
#define PROT_INTF_PC          COM1 /* PC */

/* 设备地址 */
#define DEV_ADDR              1 /* 依据参数定义设备地址修改 */
#define DEV_ADDR_SIZE         2
#define DEV_ADDR_ANY          0

/* 基础功能命令 */
#define CMD_VERSION           4 /* 版本号（软硬件） */
#define CMD_REBOOT            5 /* 设备软复位 */

/* 以太网功能命令 */
#define CMD_NET_MAC           17 /* 设备MAC地址上报 */

/* 固件升级命令 */
#define CMD_UP_NOTIFY         50 /* 升级开始。把新固件版本、总大小、包大小通知给设备 */
#define CMD_UP_DATA           51 /* 设备请求数据包，发这个命令。上位机回复这个命令，发送一个数据包 */
#define CMD_UP_FINISH         52 /* 升级确认。上位机给设备下发，表示升级完成，设备收到命令后重启，由BOOT完成固件升级 */
#define CMD_UP_CANCEL         53 /* 升级取消。上位机给设备下发，表示升级过程中止 */

/* 日志命令 */
#define CMD_REC_GET           54 /* 日志编号和内容获取 */
#define CMD_REC_STOP          55 /* 日志停止状态查询和设置 */
#define CMD_REC_TIME          56 /* 日志同步时间查询和设置 */
/* 业务命令 */
#define CMD_EMIT              60 /* 发射率查询和设置 */
#define CMD_SENSOR            61 /* 传感器系数查询和设置 */

/* 命令属性表定义 */
typedef int (*cmd_base_hand_t)(parse_base_t *parse, char *output, int *outlen);
typedef struct {
    int             cmd;
    cmd_base_hand_t handler;
} cmd_base_item_t;

extern const cmd_base_item_t CMD_BASE_TABLE[];
extern const int             cmd_base_tab_size;

int sys_branch_cmd(comm_port_t *comm_port, proto_base_head_t *header, void *data, int len, void *pkt);
int net_branch_cmd(comm_port_t *comm_port, proto_base_head_t *header, void *data, int len, void *pkt);
int upgrade_branch_cmd(parse_base_t *parse, char *outbuf, int *outlen);
int record_branch_cmd(comm_port_t *comm_port, proto_base_head_t *head, void *data, int len, void *pkt); /* 命令处理 */

int transaction_branch_cmd(int intf_type, proto_base_head_t *header, void *data, int len, void *pkt);

#endif /* _PROTO_BASE_CFG_H_ */
