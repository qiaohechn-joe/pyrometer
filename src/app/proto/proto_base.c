/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：proto_base.c
 * 文件描述：和其通用协议驱动代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/11/26 陈  军  初始版本
 *           V1.1 2025/02/13 李兆越  协议层、传输层解耦
 */

#include "platform.h"
#include "proto_base.h"
#include "proto_base_cfg.h"

static void base_instr_handler(parse_base_t *parse, void *outbuf, int *outlen);

/*
 * 功能描述：检查一个协议数据包是否有效，并返回包头指针
 * 入参说明：inbuf   --- 原始数据指针
 *           len   --- 数据长度
 * 出参说明：header --- 输出：包头指针
 *           error  --- 输出：错误码（0 正确，1 CRC 错，2 长度不够）
 * 返 回 值：1 --- 成功找到完整且正确的包，0 --- 没找到
 */
int check_packet_base(parse_base_t *parse, void *inbuf, int inlen) {
    char *pos = (char *)inbuf;

    /* 起始符&包长检查 */
    if (inlen < sizeof(proto_base_head_t)) {
        parse->error = 2; /* 长度不足以组成包头 */
        return 0;
    } else if (*pos != PROT_SOP) {
        parse->error = 3; /* 起始符错误 */
        return 0;
    }
    parse->header    = (proto_base_head_t *)inbuf;
    parse->instr_len = sizeof(proto_base_head_t);
    parse->data      = (char *)inbuf + sizeof(proto_base_head_t);

    /* 包头CRC 校验 */
    if (crc16_RTU(parse->header, sizeof(proto_base_head_t)) != 0) {
        parse->error  = 1;
        parse->header = NULL;
        return 0;
    }

    /* 数据长度检查 */
    if (inlen - parse->instr_len < parse->header->data_len + 2) {
        parse->error = 4;
        return 0;
    }

    /* 校验数据段 CRC */
    if (parse->header->data_len > 0) {
        parse->instr_len += (parse->header->data_len + 2);
        if (crc16_RTU(pos + sizeof(proto_base_head_t), parse->header->data_len + 2) != 0) {
            parse->error  = 1;
            parse->header = NULL;
            return 0;
        }
    }

    return 1;
}

/*
 * 功能描述：这个函数处理缓存区中的多个数据包
 * 入参说明：buff --- 存放多个数据包的缓存区
 *           len --- 缓存区长度
 *           handler --- 处理数据包的回调函数
 *           arg --- 传递给回调函数的参数
 * 返 回 值：无
 */
void base_pkt_process(char *inbuf, int inlen, char *outbuf, int *outlen, comm_port_t *comm_port) {
    while (inlen > 0 && inbuf != NULL) {
        parse_base_t        parse_pkt = {0};
        parse_base_t *const parse     = &parse_pkt;

        if (check_packet_base(parse, inbuf, inlen)) {
            parse->port = comm_port;
            base_instr_handler(parse, outbuf, outlen);
            outbuf += *outlen;
        } else if (parse->error != 1) {
            break;
        }
        inbuf += parse->instr_len;
        inlen -= parse->instr_len;
    }
}

/*
 * 功能描述：这个回调函数处理数据包,
 * 入参说明：header --- 指向包头
 *           arg --- 参数
 * 返 回 值：无
 */
void base_instr_handler(parse_base_t *parse, void *outbuf, int *outlen) {

    /* 执行分支命令 */
    base_branch_cmd(parse, outbuf, outlen);
}

/*
 * 功能描述：这个函数用于构建一个数据包
 * 入参说明：buff --- 用于存放数据包的缓冲区
 *           dev_id --- 发送的id地址
 *           cmd --- 命令码
 *           op --- 操作类型，'R'表示读取，'W'表示写入
 *           data --- 指向负载数据的指针
 *           len --- 负载数据的长度（字节数）
 * 返 回 值：0:执行成功 非0：执行失败
 */
int proto_base_build_pkt(parse_base_t *parse, char *outbuf, int *outlen) {

    /* 包头 */
    parse->header->crc = crc16_RTU(parse->header, sizeof(proto_base_head_t) - sizeof(uint16_t));
    mem_copy(outbuf, parse->header, sizeof(proto_base_head_t));
    outbuf += sizeof(proto_base_head_t);
    *outlen += sizeof(proto_base_head_t);

    /* data部分 */
    if (parse->error != 0) {
        parse->data             = &parse->error;
        parse->header->op_type  = PROT_OP_NACK;
        parse->header->data_len = sizeof(parse->error);
    }

    if (parse->data != NULL && parse->header->data_len != 0) {
        mem_copy(outbuf, parse->data, parse->header->data_len);
        outbuf += parse->header->data_len;
        *outlen += parse->header->data_len;

        /* data CRC */
        uint16_t data_crc = crc16_RTU(parse->data, parse->header->data_len);
        mem_copy(outbuf, &data_crc, sizeof(data_crc)); // 安全拷贝，避免对齐问题
        outbuf += sizeof(data_crc);
        *outlen += sizeof(data_crc);
    }

    return 0;
}

/*
 * 功能描述：这个函数用于判断一个数据包是否为单播数据包
 * 入参说明：header --- 指向数据包头部的指针
 * 返 回 值：1 表示单播数据包，0 表示广播数据包
 */
int proto_base_unicast_pkt(proto_base_head_t *header) {
    uint16_t id;

    mem_copy(&id, (const void *)&header->dev_id, sizeof(id));

    return (id == DEV_ADDR_ANY) ? 0 : 1;
}

/*
 * 功能描述：这个函数用于将命令分发到各个功能模块
 * 入参说明：intf_type --- 接口类型
 *           pkt_in --- 输入数据包缓存区
 *           pkt_out --- 输出数据包缓存区
 * 返 回 值：输出数据包的长度
 */
int base_branch_cmd(parse_base_t *parse, void *outbuf, int *outlen) {
    int ret = 0;
    for (uint8_t i = 0; i < cmd_base_tab_size; i++) {
        if (CMD_BASE_TABLE[i].cmd == parse->header->cmd) {
            if (CMD_BASE_TABLE[i].handler) {
                ret = CMD_BASE_TABLE[i].handler(parse, outbuf, outlen);
            }
            break;
        }
    }
    return ret;
}
