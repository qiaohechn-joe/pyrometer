/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：proto_base.h
 * 文件描述：和其通用协议驱动代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/11/26 陈  军  初始版本
 *           V1.1 2025/02/13 李兆越  协议层、传输层解耦
 */

#ifndef _PROTO_BASE_H_
#define _PROTO_BASE_H_

#include "platform.h"
#include "comm_cfg.h"

#define PROT_SOP     '#' /* 数据包起始标志 */

#define DATA_CORRECT 0 /* 数据正确 */

/* 操作类型枚举 */
typedef enum {
    PROT_OP_RD = 0, /* 通用读  */
    PROT_OP_WR,     /* 通用写 */
    PROT_OP_RD2,    /* 自定义读1，日志内容获取 */
    PROT_OP_RD3,    /* 自定义读2 */
    PROT_OP_RD4,    /* 自定义读3 */
    PROT_OP_RD5,    /* 自定义读4 */
    PROT_OP_WR2,    /* 自定义写1 */
    PROT_OP_WR3,    /* 自定义写2 */
    PROT_OP_WR4,    /* 自定义写3 */
    PROT_OP_NACK,   /* 错误消息确认 */
    PROT_OP_ACK     /* 消息确认 */
} prot_op_t;

/* 包头结构体 */
typedef hq_packed struct {
    uint8_t  sop;      /* '#' */
    uint16_t dev_id;   /* 设备地址 */
    uint8_t  cmd;      /* 命令 */
    uint8_t  op_type;  /* 操作类型 */
    uint16_t data_len; /* 载荷数据长度（字节数，不包括CRC16） */
    uint16_t crc;      /* 包头CRC16 */
} proto_base_head_t;

/* 包头结构体 */
typedef hq_packed struct {
    proto_base_head_t *header;
    uint16_t           instr_len;
    char               error;
    char              *data;
    comm_port_t       *port;
} parse_base_t;

#define MIN_PKT_LEN sizeof(proto_base_head_t) /* 数据包最小长度 */

void base_pkt_process(char *inbuf, int inlen, char *outbuf, int *outlen, comm_port_t *comm_port);
int  proto_base_unicast_pkt(proto_base_head_t *header);
int  proto_base_build_pkt(parse_base_t *parse, char *outbuf, int *outlen);
int  base_branch_cmd(parse_base_t *parse, void *outbuf, int *outlen);

#endif /* _PROTO_BASE_H_ */
