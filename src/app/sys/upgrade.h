/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：upgrade.h
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

#ifndef _UPGRADE_H_
#define _UPGRADE_H_

#include "platform.h"

/* 根据不同产品配置 */
#define SPECAL_PRODUCT_NUM      "LED102-v3" /* 产品号确认防止异常升级 */
#define VERSION_CHECK           0           /* 开启版本校验 */

/* 升级源设备 */
#define UG_SRC_RS232            RS232_COM      /* 串口RS232 */
#define UG_SRC_RS485            RS485_COM      /* 串口RS485 */
#define UG_SRC_DTU              20             /* DTU */
#define UG_SRC_LORA             21             /* LoRa */
#define UG_SRC_BLE              22             /* 蓝牙 */
#define UG_SRC_UDISK            23             /* U盘 */
#define UG_SRC_USB              24             /* USB协议 */
#define UG_SRC_SDCARD           25             /* SD卡 */
#define UG_SRC_NET              NET_CLIENT_COM /* 以太网 */

/* 存储划分 */
#define IMG_INFO_ADDR           FW_INFO_ADDR /* 固件信息存储区--GDFLASH */
#define IMG_DATA_ADDR           FW_DATA_ADDR /* 固件数据存储区--GDFLASH */

#define IMG_DATA_SECT_1         FW_DATA_SECT_1 /* 固件数据存储区1--GDFLASH */
#define IMG_DATA_SECT_2         FW_DATA_SECT_2 /* 固件数据存储区2--GDFLASH */

#define FW_SEG_SIZE             1024 /* 段大小 */

/* 消息 */
#define MSG_STOP_UG             0x01
#define MSG_RECV_DATA           0x02

/* 升级状态定义 */
#define UG_STATE_START          1
#define UG_STATE_TRANS          2
#define UG_STATE_CANCELED       3
#define UG_STATE_TRANS_COMPLETE 4
#define UG_STATE_CHECK_FAIL     5
#define UG_STATE_CHECK_PASS     6
#define UG_STATE_FINISHED       7

/* 升级标志 */
#define UG_FLAG_START           1 /* 升级开始 (APP --> BL) */
#define UG_FLAG_OVER            2 /* 升级完成 (BL) */
#define UG_FLAG_FINISH          3 /* 升级确认 (BL --> APP) */

/* U盘升级 */
#define FW_IMG_NAME             "HQ*.bin"
#define FW_IMG_MAX_SIZE         (BSP_FLASH_SIZE - 0x3000UL) /* Bootloader大小：0x3000 */
#define FW_IMG_VER_OFFSET       0x0400

/* 远程升级参数 */
#define FW_DATA_RETRY           3                         /* 数据请求重试次数 */
#define FW_DATA_TIMEOUT_EXT     (configTICK_RATE_HZ * 10) /* 数据回复超时（DTU）-- 10s * 3 */
#define FW_DATA_TIMEOUT_PC      (configTICK_RATE_HZ * 1)  /* 数据回复超时（PC）  -- 5s  * 3 */

#define mcu_soft_reset()                                                                                                                             \
    do {                                                                                                                                             \
        __set_FAULTMASK(1);                                                                                                                          \
        NVIC_SystemReset();                                                                                                                          \
    } while (0)

/* 固件信息 */
typedef hq_packed struct { /* 28字节 */
    uint8_t  major_ver;    /* 主版本 */
    uint8_t  minor_ver;    /* 次版本 */
    uint8_t  serial_ver;   /* 修正版本 */
    uint32_t img_size;     /* 固件总字节 */
    uint16_t seg_size;     /* 分包大小（字节） */
    uint16_t seg_count;    /* 分包总数 */
    uint32_t img_crc32;    /* 固件镜像的CRC32 */
    uint8_t  status;       /* 升级状态 */
    uint32_t img_addr;     /* 固件地址 */

    comm_port_t *source;   /* 升级源，UG_SRC_xxx */
    uint16_t     seg_idx;  /* 当前分包序号，从0计数 */
    uint8_t      rsvd1[2]; /* 填充字段1 */
    uint32_t     crc32;    /* 固件信息结构的CRC16 */
} fw_info_t;               /* 存下来代码用到的 */

/* 固件通知 */
typedef hq_packed struct {
    uint8_t  major_ver;  /* 主版本 */
    uint8_t  minor_ver;  /* 次版本 */
    uint8_t  serial_ver; /* 修正版本 */
    uint32_t img_size;   /* 固件总字节大小 */
    uint16_t seg_size;   /* 段大小（字节） */
    uint16_t seg_count;  /* 段总数 */
    uint32_t img_crc32;  /* 固件镜像的CRC32（bin） */
} fw_notify_t;

/* 固件数据请求（设备） */
typedef hq_packed struct {
    uint8_t  major_ver;  /* 主版本 */
    uint8_t  minor_ver;  /* 次版本 */
    uint8_t  serial_ver; /* 修正版本 */
    uint16_t seg_idx;    /* 当前段号 */
    uint16_t seg_count;  /* 段总数 */
} fw_data_req_t;

/* 固件数据回复头（上位机） */
typedef hq_packed struct {
    uint8_t  major_ver;  /* 主版本 */
    uint8_t  minor_ver;  /* 次版本 */
    uint8_t  serial_ver; /* 修正版本 */
    uint16_t seg_idx;    /* 分包序号 */
    uint16_t seg_len;    /* 此段的数据长度 */
} fw_data_reply_t;

/* 升级完成请求（上位机） */
typedef hq_packed struct {
    uint8_t  major_ver;  /* 主版本 */
    uint8_t  minor_ver;  /* 次版本 */
    uint8_t  serial_ver; /* 修正版本 */
    uint16_t seg_count;  /* 段总数 */
} up_finish_data_t;

extern fw_info_t fw_info;

int upgrade_init(void);
int upgrade_udisk(void);
// 读取固件信息表
void upgrade_clr_accept(void);
int  CMD_UP_notify(parse_base_t *parse, char *outbuf, int *outlen);
int  CMD_UP_data(parse_base_t *parse, char *outbuf, int *outlen);
int  CMD_UP_finish(parse_base_t *parse, char *outbuf, int *outlen);
int  CMD_UP_cancel(parse_base_t *parse, char *outbuf, int *outlen);
void upgrade_task(void *arg);

#endif /* _UPGRADE_H_ */
