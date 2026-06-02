/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：record_cfg.h
 * 文件描述：日志管理配置代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/12/20 陈军    初始版本
 *           V1.1 2025/03/07 李兆越  优化记录数量宏定义
 *           V1.2 2025/04/22 李兆越  加入日志异常处理
 */

#ifndef _RECORD_CFG_H_
#define _RECORD_CFG_H_

#define REC_FS_OK           0 /* 是否支持FatFS文件系统 */

#define REC_INTVAL_MIN      300      /* 最小300ms，防止太快EEPROM无法获取互斥量 */
#define REC_INTVAL_MAX      86400000 /* 最大24小时 */

/* 日志类型 */
#define REC_TYPE_BBOX       0 /* 黑盒日志 */
#define REC_TYPE_STATE      1 /* 工作状态日志 */
#define REC_TYPE_CNT        2

/* EEPROM每个区域的大小 */
#define REC_BBOX_SIZE       (EEPROM_ADDR_REC_BBOX_OFF - EEPROM_ADDR_REC_BBOX_ON)   /* 61.875 KB = 63,360 字节 */
#define REC_STATE_SIZE      (EEPROM_ADDR_REC_STATE_OFF - EEPROM_ADDR_REC_STATE_ON) /* 61.875 KB = 63,360 字节 */

/* 日志记录数量 */
#define REC_BBOX_CNT        (REC_BBOX_SIZE / sizeof(bbox_record_t))
#define REC_STATE_CNT       (REC_STATE_SIZE / sizeof(state_record_t))

/* 日志类型名称 */
#define REC_TYPE_NAME_BBOX  "BlackBox"  /* 黑盒日志名称 */
#define REC_TYPE_NAME_STATE "WorkState" /* 工作状态日志名称 */

/* 每次请求的最大日志记录数 */
#define REC_REQ_MAX_COUNT   20

/* 日志索引存储地址 */
#define REC_INDEX_ADDR      EEPROM_ADDR_REC_INDEX

/* 日志存储地址 */
#define REC_BBOX_ADDR       EEPROM_ADDR_REC_BBOX_ON
#define REC_STATE_ADDR      EEPROM_ADDR_REC_STATE_ON

/* 黑盒日志标题 */
#define REC_TITLE_BBOX      "| num | time |dev_state | reboot_cnt | \r\n"

/* 工作状态日志标题 */
#define REC_TITLE_STATE     "| num | time_sec | start_state | env_temp | \r\n"
/* 黑盒日志结构体（和黑盒标题项对应） */
typedef hq_packed struct {               /* 定时触发，记录关键事件，频率较低 */
    rtc_time_t time;                     /* RTC时间 */
    uint16_t   dev_state;                /* 设备状态，0X00正常 */
    uint32_t   net_status;               /* 网络连接状态（如：已连接、断开等） */
    uint32_t   xTotalHeapSize;           /* 系统总堆内存大小（字节） */
    uint32_t   xFreeHeapSize;            /* 当前空闲堆内存大小（字节） */
    uint32_t   xMinimumEverFreeHeapSize; /* 历史最小空闲堆内存值（字节） */
    //    uint8_t cpu_usage;         /* CPU占用率（0-100%） */

    //    uint8_t battery_level;        /* 电池电量 */
    /* 错误记录 */
    //    uint16_t spi_error_code;       /* SPI通信错误码 */
    //    uint16_t timeout_count;        /* SPI通信超时次数 */
    //    uint16_t checksum_error_count; /* 校验错误次数 */
    // 备用电池电量
} bbox_record_t;

/* 状态日志结构体（和状态标题项对应） */
typedef hq_packed struct { /* 错误触发时，记录时间戳和那一刻的状况 */
    uint32_t time_sec;     /* RTC时间戳 */
    uint8_t  start_state;  /* 0转为冷启动，1转为热启动 */
    //    uint16_t reboot_count; /* 设备重启次数 */
    //    float env_temp; /* 环境温度（单位：摄氏度） */

    //    uint8_t relay_alarm_state; /* 继电器报警状态（如：超限报警等） */
    //    uint8_t power_voltage;     /* 电源电压状态（单位：V，低电压报警等） */
    //    /* 网络相关 */
    //    uint8_t  net_status;  /* 网络连接状态（如：1已连接、0断开等） */
    //    uint16_t net_latency; /* 网络延迟（单位：ms） */

    //    /* 紧急日志存储区域todo：后续放入内部flash，避免低电压读写 */
    //    uint8_t emergency_log_flag;         /* 紧急日志标志位（0: 无更新，1: 有更新） */
    //    uint8_t emergency_log_memory_error; /* 内存异常标志 */
    //    uint8_t emergency_log_bus_error;    /* 总线错误标志 */

    //    /* 外部设备健康状态 */
    //    uint8_t external_device_health; /* 外部设备健康状态（如：正常、故障等） */

} state_record_t;

extern void  bbox_record_task(void *reg);
extern void  state_record_task(void *reg);
extern char *bbox_rec_format(char *text, uint32_t num, void *buff);
extern char *state_rec_format(char *text, uint32_t num, void *buff);

extern const char *const REC_TYPE_NAME[];
extern const char *const REC_TITLE[];
extern const uint32_t    REC_SIZE[];
extern const uint32_t    REC_CAPACITY[];
extern const uint32_t    REC_START_ADDR[];
extern const uint32_t    REC_END_ADDR[];

#endif /* _RECORD_CFG_H_ */
