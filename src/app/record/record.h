/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：record.h
 * 文件描述：日志管理代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/12/10 陈军  初始版本
 *           V1.1 2025/02/13 李兆越  停止状态设置优化；复位停止信息规范处理；存储日志范围限制；
 *           V1.2 2025/04/18 李兆越  黑盒日志采用软定时统一管理
 */

#ifndef _RECORD_H_
#define _RECORD_H_

#include "platform.h"

/* 无效的日志记录编号 */
#define REC_NUM_NA (~0)

/* 日志索引（需保存，如EEPROM） */
typedef hq_packed struct {
    uint32_t head_num; /* 第一个日志编号：不循环 */
    uint32_t tail_num; /* （最后一个 + 1）日志编号：不循环 */
    uint32_t count;    /* 日志数量 */
    uint16_t rsvd;     /* 填充位 */
    uint16_t crc;      /* CRC16 */
} record_index_t;

/* 日志停止信息 */
typedef hq_packed struct {
    uint8_t  type;     /* 日志类型 */
    uint32_t start;    /* 开始时间戳（从1970-1-1 00:00:00起的秒数） */
    uint32_t duration; /* 持续时间（秒），0=不停止，~0=永久停止 */
} record_stop_t;

/* 日志信息 */
typedef struct {
    uint32_t    size;      /* 日志大小（字节） */
    uint32_t    capacity;  /* 日志容量 */
    const char *name;      /* 日志类型名称 */
    const char *title;     /* 日志标题 */
    uint32_t    first_num; /* 第一个日志编号：不循环 */
    uint32_t    last_num;  /* 最后一个日志编号：不循环 */
    uint32_t    count;     /* 日志数量 */
} record_info_t;

/* 日志编号 */
typedef hq_packed struct {
    uint8_t  type;  /* 日志类型 */
    uint32_t first; /* 第一个日志编号：从0开始，不循环 */
    uint32_t last;  /* 最后一个日志编号：从0开始，不循环 */
} record_num_t;

/* 日志请求信息 */
typedef hq_packed struct {
    uint8_t  type;  /* 日志类型，范围为[0,1] */
    uint32_t first; /* 第一个日志编号：从0开始，不循环。范围为(0,∞) */
    uint32_t count; /* 获取日志数量，范围为(0,20] */
} record_req_t;

extern record_index_t rec_index[REC_TYPE_CNT];
extern record_stop_t  rec_stop[REC_TYPE_CNT];

int  record_init(void);                                                      /* 初始化 */
void record_clear(void);                                                     /* 清除所有日志 */
void record_get_info(int type, record_info_t *info);                         /* 获取日志信息 */
int  record_is_stop(int type);                                               /* 检查日志是否停止 */
int  record_read(int type, unsigned long num, void *buff);                   /* 读取日志（不包括编号） */
int  record_load(int type, uint32_t start, int count, void *buff, int size); /* 加载日志 */
int  record_save(int type, void *buff);                                      /* 保存日志 */
void record_format(char *text, int type, uint32_t num, void *buff);          /* 格式化日志 */

int cmd_get_record(int cmd, int op, void *data, int len, void *pkt);
int cmd_stop_record(int cmd, int op, void *data, int len, void *pkt);
int cmd_time_record(int cmd, int op, void *data, int len, void *pkt);
int record_export_usb(void); /* 导出日志 */

#endif /* _RECORD_H_ */
