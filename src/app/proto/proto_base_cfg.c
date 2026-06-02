/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：proto_tp1_cfg.c
 * 文件描述：和其通用协议配置代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/11/26 陈  军  初始版本
 *           V1.1 2025/02/13 李兆越  加入日志56号命令
 */

#include "proto_base_cfg.h"
#include "upgrade.h"
#include "record.h"
#include "ostimer.h"

const cmd_base_item_t CMD_BASE_TABLE[];

/* 命令表 */
/* clang-format off */
static const cmd_base_item_t CMD_BASE_TABLE[] = {
    /* 设备基础功能 */
//    {CMD_VERSION,       sys_branch_cmd},      /* 4  设备软硬件版本查询 */
//    {CMD_REBOOT,        sys_branch_cmd},      /* 5  设备重启 */
//    /* 以太网功能 */
//    { CMD_NET_MAC,      net_branch_cmd},      /* 17 设备MAC地址上报 */
    /* 固件升级 */
    { CMD_UP_NOTIFY,    upgrade_branch_cmd},  /* 50 升级通知（上位机3次重试） */
    { CMD_UP_DATA,      upgrade_branch_cmd},  /* 51 升级数据传输 */
    { CMD_UP_FINISH,    upgrade_branch_cmd},  /* 52 升级完成确认 */
    { CMD_UP_CANCEL,    upgrade_branch_cmd},  /* 53 升级取消 */
#if SUPPORT_EEPROM
    /* 日志功能 */
//    {CMD_REC_GET,       record_branch_cmd},   /* 54 日志编号和内容获取 */
//    {CMD_REC_STOP,      record_branch_cmd},   /* 55 日志停止状态查询和设置 */
//    {CMD_REC_TIME,      record_branch_cmd},   /* 56 日志同步时间查询和设置 */
#endif
    /* 业务功能 */
};
const int cmd_base_tab_size = sizeof(CMD_BASE_TABLE) / sizeof(CMD_BASE_TABLE[0]);
/* clang-format on */

/*
 * 功能描述：这个函数处理设备基础功能命令
 * 入参说明：intf_type --- 接口类型
 *           header --- 处理数据包的回调函数
 *           data --- 指向包有效载荷
 *           len --- 有效载荷长度
 *           pkt --- 输出包缓冲
 * 返 回 值：输出包长度
 */
int sys_branch_cmd(comm_port_t *comm_port, proto_base_head_t *header, void *data, int len, void *pkt) {
    int cmd;
    int op;
    int ret = 0;

    cmd = header->cmd;
    op  = header->op_type;

    switch (cmd) {
        case CMD_VERSION: ret = cmd_version(op, data, len, pkt); break;
        case CMD_REBOOT: ret = sys_reboot(op, data, len, pkt); break;
        default: ret = 0; break;
    }

    return ret; /* out-packet length */
}

/*
 * 功能描述：这个函数处理以太网命令
 * 入参说明：intf_type --- 接口类型
 *           header --- 处理数据包的回调函数
 *           data --- 指向包有效载荷
 *           len --- 有效载荷长度
 *           pkt --- 输出包缓冲
 * 返 回 值：输出包长度
 */
int net_branch_cmd(comm_port_t *comm_port, proto_base_head_t *header, void *data, int len, void *pkt) {
    int cmd;
    int op;
    int ret = 0;

    cmd = header->cmd;
    op  = header->op_type;

    switch (cmd) {
        case CMD_NET_MAC: ret = cmd_net_mac(cmd, op, data, len, pkt); break;

        default: ret = 0; break;
    }

    return ret; /* out-packet length */
}

/*
 * 功能描述：这个函数为升级命令分支
 * 入参说明：intf --- 接口号，PROT_INTF_XXX
 *           head --- 指向prot_tp1_head_t结构体的指针
 *           data --- 指向报文数据负载指针
 *           len --- 报文数据负载长度
 *           pkt --- 返回报文缓存
 * 返 回 值：0:执行成功 非0：执行失败
 */
int upgrade_branch_cmd(parse_base_t *parse, char *outbuf, int *outlen) {
    int ret;

    switch (parse->header->cmd) {
        case CMD_UP_NOTIFY: ret = CMD_UP_notify(parse, outbuf, outlen); break;
        case CMD_UP_DATA: ret = CMD_UP_data(parse, outbuf, outlen); break;
        case CMD_UP_FINISH: ret = CMD_UP_finish(parse, outbuf, outlen); break;
        case CMD_UP_CANCEL: ret = CMD_UP_cancel(parse, outbuf, outlen); break;
        default: ret = 0; break;
    }

    return ret;
}
#if SUPPORT_EEPROM
/*
 * 功能描述：这个函数分支日志类命令
 * 入参说明：intf --- 接口编号，PROT_INTF_XXX
 *           head --- 指向数据包头的指针
 *           data --- 指向数据包负载的指针
 *           len --- 负载长度
 *           pkt --- 输出数据包缓存区
 * 返 回 值：输出数据包的长度
 */
int record_branch_cmd(comm_port_t *comm_port, proto_base_head_t *head, void *data, int len, void *pkt) {
    int cmd;
    int op;
    int ret;

    comm_port = comm_port;
    cmd       = head->cmd;
    op        = head->op_type;

    switch (cmd) {
        case CMD_REC_GET: ret = cmd_get_record(cmd, op, data, len, pkt); break;
        case CMD_REC_STOP: ret = cmd_stop_record(cmd, op, data, len, pkt); break;
        case CMD_REC_TIME: ret = cmd_time_record(cmd, op, data, len, pkt); break;
        default: ret = 0; break;
    }

    return ret; /* 应答包长度（字节） */
}
#endif
