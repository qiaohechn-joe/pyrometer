/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：upgrade.c
 * 文件描述：固件升级驱动代码（基于协议类型1）
 * 作    者：和其光电嵌软团队
 * 备    注：考虑到没有EEPROM的项目不兼容，决定统一存储至内部FLASH
 * 更新记录：
 *           V1.0 2024/12/09 陈  军  初始版本
 *           V1.1 2024/01/03 李兆越  优化命令与存储
 *           V1.2 2024/02/19 李兆越  任务启停、接口收发代码解耦
 *           V1.3 2024/03/04 李兆越  dbg_info打印信息优化
 *           V1.4 2024/03/06 李兆越  升级文件支持大于128KB
 *           V1.5 2024/04/22 李兆越  任务启停逻辑规范解耦
 *           V1.6 2024/05/06 李兆越  统一页擦写
 */

#include "agent.h"
#include "mail_box.h"
#include "storage.h"
#include "serial.h"
#include "system.h"
#include "upgrade.h"
#include "ostimer.h"

static mbox_t *xfer_mb; /* 邮箱用于固件传输 */
fw_info_t      fw_info;

/*
 * 功能描述：这个函数固件升级单元
 * 入参说明：无
 * 返 回 值：0 -- 成功，其它 -- 失败
 */
int upgrade_init(void) {
    xfer_mb = mbox_create(2);
    if (xfer_mb == NULL) {
        dbg_upgrade("Upgrade : xfer_mb create failed!\r\n");
        return -1;
    }

    /* 使能CRC外设时钟 */
    mcu_periph_crc_enable();

    return 0;
}

/*
 * 功能描述：这个函数从远端升级固件（投递方式）
 * 入参说明：arg --- 入参
 * 返 回 值：无
 */
static void upgrade_task(void *arg) {
    int           err;
    int           msg;
    fw_data_req_t req;

    uint32_t        timeout;
    static uint32_t retry = 0;

    upgrade_init();
    comm_port_t *comm_port = (comm_port_t *)arg;

    ENTER_CRITICAL();
    fw_info.status = UG_STATE_TRANS;
    EXIT_CRITICAL();

    req.major_ver  = FW_VER_MAJOR;
    req.minor_ver  = FW_VER_MINOR;
    req.serial_ver = FW_VER_SERIAL;
    req.seg_count  = fw_info.seg_count;

    timeout = FW_DATA_TIMEOUT_PC;
    mbox_clear(xfer_mb); /* 清除邮箱 */

    /* 擦除固件数据区 */
    gdf_erase_sector(IMG_DATA_SECT_1);
    gdf_erase_sector(IMG_DATA_SECT_2);

    while (fw_info.seg_idx < fw_info.seg_count) {
        /* 中途取消升级：防止取消后请求 */
        if (fw_info.status == UG_STATE_CANCELED) {
            goto over;
        }

        /* 保存段号, todo 考虑删除，只在退出升级时写入即可,包括异常情况 */
        //        ENTER_CRITICAL();
        //        fw_info.crc16   = crc16_RTU(&fw_info, sizeof(fw_info) - 2);
        //        EXIT_CRITICAL();
        //        gdf_write_data_by_page(IMG_INFO_ADDR, sizeof(fw_info), (uint8_t *)&fw_info);

        /* 构建请求包 */
        req.seg_idx = fw_info.seg_idx;

        /* 发送请求，等待应答 */
        for (retry = FW_DATA_RETRY; retry != 0; retry--) {
            parse_base_t parse       = {0};
            parse.header->cmd        = CMD_UP_DATA;
            proto_base_head_t header = {
                .sop      = '#',               // 起始字符
                .dev_id   = sys_para.dev.addr, // 设备地址，例如 1
                .cmd      = CMD_UP_DATA,       // 命令码，例如 10
                .op_type  = PROT_OP_WR,        // 操作类型，比如读操作
                .data_len = 0x0007,            // 数据长度为 4 字节
            };
            parse.header    = &header;
            parse.data      = (char *)&req;
            int  outlen     = 0;
            char outbuf[20] = {0}; // 目前长度用到18
            proto_base_build_pkt(&parse, outbuf, &outlen);
            /* 中途取消升级：防止取消后请求 */
            if (fw_info.status == UG_STATE_CANCELED) {
                goto over;
            }
            //            if(outlen < 18 || retry < 3) {
            //                int a = 1;
            //            }
            /* 应答命令 */
            if (outlen > 0) {
                comm_send_chunk(comm_port->group_idx, comm_port->channel_idx, outbuf, outlen);
            }

            msg = (int)mbox_fetch(xfer_mb, timeout, &err); /* 等待data */
            if (msg == MSG_RECV_DATA) {
                break;
            }
        }

        if (retry == 0) {
            goto over; /* 超时 */
        }
    }
over:
    ENTER_CRITICAL();
    fw_info.status = UG_STATE_FINISHED;
    EXIT_CRITICAL();

    dbg_upgrade("Upgrade : Over!\r\n");
}

/*
 * 功能描述：这个函数处理升级通知命令 CMD_UP_NOTIFY
 * 入参说明：intf --- 接口号，PROT_INTF_XXX
 *           op --- 操作类型, PROT_OP_xxx
 *           data --- 指向报文数据负载指针
 *           len --- 报文数据负载长度
 *           pkt --- 返回报文缓存
 * 返 回 值：0:执行成功 非0：执行失败
 */
int CMD_UP_notify(parse_base_t *parse, char *outbuf, int *outlen) {
    fw_notify_t *notify = (fw_notify_t *)parse->data;
    int          ret    = 0;

    if ((parse->header->op_type == PROT_OP_WR) && (parse->header->data_len == sizeof(fw_notify_t))) {
#if VERSION_CHECK
        if ((notify->minor_ver != version_minor(SYS_VERSION.fw)) ||
            ((notify->major_ver < version_major(SYS_VERSION.fw)) && (notify->serial_ver < version_serial(SYS_VERSION.fw)))) {
            parse->data             = NULL;
            parse->header->data_len = 0;
            ret                     = proto_base_build_pkt(parse, outbuf, outlen);
            return ret;
        }
#endif
        ENTER_CRITICAL();

        /* 版本正确 */
        mem_copy((void *)&fw_info, (const void *)notify, sizeof(fw_notify_t));
        fw_info.source  = parse->port;
        fw_info.status  = UG_STATE_START;
        fw_info.seg_idx = 0;

        //    if (fw_info.status != UG_STATE_START) { /* 非传输中断情况 */
        //        fw_info.seg_idx = 0;                /* 清除当前段号 */
        //    }
        // fw_info.crc16 = crc16_RTU(&fw_info, sizeof(fw_info) - 2);
        EXIT_CRITICAL();

        /* 擦除固件信息区 */
        gdf_write_data_by_page(IMG_INFO_ADDR, sizeof(fw_info), (uint8_t *)&fw_info);

        /* 任务启停 */
        for (int i = 1; i < OSTIMER_COUNT; i++) {
            ostimer_close(i);
            // TODO 关闭突发串
        }

        agent_post(NOT_FROM_ISR, AGENT_PRIO_MD, upgrade_task, (void *)parse->port); /* 启动升级任务 */
        ret = proto_base_build_pkt(parse, outbuf, outlen);                          // todo 移出到外部封装
    }

    return ret;
}

/*
 * 功能描述：这个函数处理升级数据命令 CMD_UP_DATA
 * 入参说明：intf --- 接口号，PROT_INTF_XXX
 *           op --- 操作类型, PROT_OP_xxx
 *           data --- 指向报文数据负载指针
 *           len --- 报文数据负载长度
 *           pkt --- 返回报文缓存
 * 返 回 值：返回报文长度
 * 备    注：
 *           7+数据分包：软件版本号+包序号+包大小+数据分包
 *           128和256分包的时候，进中断太紧凑导致没喂狗，短暂延时即可
 */
int CMD_UP_data(parse_base_t *parse, char *outbuf, int *outlen) {
    fw_data_reply_t *reply = (fw_data_reply_t *)parse->data;

    if (parse->header->op_type == PROT_OP_WR) {
        dbg_upgrade("ver=%d%d%d seg_num=%d fw.ver=%d%d%d fw.seg_num=%d\r\n", reply->major_ver, reply->minor_ver, reply->serial_ver, reply->seg_idx,
                    fw_info.major_ver, fw_info.minor_ver, fw_info.serial_ver, fw_info.seg_idx);
        if (!mem_comp(reply, &fw_info, 3) && (reply->seg_idx == fw_info.seg_idx)) /* 版本号和当前段号都匹配 */
        {
            fw_info.source = parse->port;

            /* 保存固件数据到存储区 */
            uint32_t addr = IMG_DATA_ADDR + fw_info.seg_idx * fw_info.seg_size;

            vTaskSuspendAll();
            gdf_write_data_by_word(addr, reply->seg_len, parse->data + sizeof(fw_data_reply_t));
            xTaskResumeAll();
            dbg_upgrade("Upgrade : write %d bytes @0x%08X\r\n", reply->seg_len, addr);
            addr += reply->seg_len;
            fw_info.seg_idx++; /* 下一段 */

            /* 小单包，还有取消升级，慢慢来50->100 */
            vTaskDelay(100);

            mbox_post(xfer_mb, (void *)MSG_RECV_DATA); /* 投递已收到数据信号到邮箱 */
        }
    }

    return 0; /* 不应答 */
}

/*
 * 功能描述：这个函数处理升级完成确认命令 CMD_UP_FINISH
 * 入参说明：intf --- 接口号，PROT_INTF_XXX
 *           op --- 操作类型, PROT_OP_xxx
 *           data --- 指向报文数据负载指针
 *           len --- 报文数据负载长度
 *           pkt --- 返回报文缓存
 * 返 回 值：返回报文长度
 */
int CMD_UP_finish(parse_base_t *parse, char *outbuf, int *outlen) {
    if ((parse->header->op_type == PROT_OP_RD) && (parse->header->data_len == 5)) {

        up_finish_data_t *data = (up_finish_data_t *)parse->data;
        if (mem_comp(data, &fw_info, 3) || data->seg_count != fw_info.seg_count) {
            parse->error = 1;
        } else if (fw_info.seg_idx != fw_info.seg_count) {
            parse->error = 2;
        } else {
            fw_info.status = UG_STATE_TRANS_COMPLETE;
        }
        if (fw_info.status == UG_STATE_TRANS_COMPLETE) {
            /* 固件校验 */
            int img_crc32 = calculate_crc32((uint8_t *)FW_DATA_ADDR, fw_info.img_size, ENDIAN_BIG);
            if (fw_info.img_crc32 != img_crc32) {
                fw_info.status = UG_STATE_CHECK_FAIL;
                parse->error   = 3;
            } else if (mem_comp((const void *)(FW_DATA_ADDR + 0x400 + 12), SPECAL_PRODUCT_NUM, sizeof(SPECAL_PRODUCT_NUM) - 1) != 0) {
                fw_info.status = UG_STATE_CHECK_FAIL;
                parse->error   = 4;
            } else {
                fw_info.status = UG_STATE_CHECK_PASS;
            }
        }

        /* 保存固件信息到存储区 */
        ENTER_CRITICAL();
        // int tmp       = sizeof(fw_info_t);
        fw_info.crc32 = calculate_crc32((uint8_t *)&fw_info, sizeof(fw_info_t) - 4, ENDIAN_LITTLE);
        EXIT_CRITICAL();
        gdf_write_data_by_page(IMG_INFO_ADDR, sizeof(fw_info), (uint8_t *)&fw_info);

        parse->data             = NULL;
        parse->header->data_len = 0;
        parse->header->op_type  = PROT_OP_ACK;
        proto_base_build_pkt(parse, outbuf, outlen); // todo 移出到外部封装

        if (fw_info.status == UG_STATE_CHECK_PASS) {
            ENTER_CRITICAL();
            sys_state.sys_op.valid = true;
            sys_state.sys_op.reset = true;
            EXIT_CRITICAL();
        }
    }
    return 0;
}

/*
 * 功能描述：这个函数处理升级取消命令CMD_UP_CANCEL
 * 入参说明：intf --- 接口号，PROT_INTF_XXX
 *           op --- 操作类型, PROT_OP_xxx
 *           data --- 指向报文数据负载指针
 *           len --- 报文数据负载长度
 *           pkt --- 返回报文缓存
 * 返 回 值：返回报文长度
 */
int CMD_UP_cancel(parse_base_t *parse, char *outbuf, int *outlen) {
    int ret = 0;

    if ((parse->header->op_type == PROT_OP_RD) && (parse->header->data_len == 2)) { /* 数据包总数为2字节 */
        mbox_post(xfer_mb, (void *)MSG_STOP_UG);                                    /* 投递停止信号 */

        /* 取消升级标志置位 */
        ENTER_CRITICAL();
        fw_info.status = UG_STATE_CANCELED;
        EXIT_CRITICAL();

        parse->data             = NULL;
        parse->header->data_len = 0;
        parse->header->op_type  = PROT_OP_ACK;
        ret                     = proto_base_build_pkt(parse, outbuf, outlen);

        /* 任务启停 */
        for (int i = 1; i < OSTIMER_COUNT; i++) {
            ostimer_open(i, 1);
        }
    }

    return ret;
}
