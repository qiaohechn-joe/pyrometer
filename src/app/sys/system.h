/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：system.h
 * 文件描述：系统层管理业务代码（初始化，系统维护，关键业务等）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本

 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "platform.h"
#include "para.h"
#include "sys_cfg.h"
#include "gd32_ll.h"

/* 密码仓库 */
#define PWD_CNT 2
extern uint32_t PWD_TAB[PWD_CNT];

/* 设备自检信息 */
#define DEV_ST_OK                          '0' /* 自检通过 */
#define DEV_ST_ERR                         '1' /* 自检错误 */

#define DAC_CAMERA_MUTEX_TIMEOUT           (configTICK_RATE_HZ / 2)

/* 系统操作 */
#define SYS_NONE                           0
#define SYS_UPGRADE                        1
#define SYS_BACKUP_ARG                     2
#define SYS_RESTORE_ARG                    3

/* 密码校验 */
#define SYS_PW_MASK                        0xfffff000
#define SYS_OP_MASK                        (~SYS_PW_MASK)
#define SYS_PW_VALUE                       0x80296

/* 版本操作 */
#define FW_BUILT                           "built at " __TIME__ " " __DATE__
#define make_version(major, minor, serial) (((uint32_t)major << 24) | ((uint32_t)minor << 16) | ((uint32_t)serial << 8) | 0)

#define version_major(ver)                 (((ver) >> 24) & 0x0ff)
#define version_minor(ver)                 (((ver) >> 16) & 0x0ff)
#define version_serial(ver)                (((ver) >> 8) & 0x0ff)

/* 版本信息定义 */
typedef hq_packed struct {
    uint32_t boot; /* boot版本 */
    uint32_t hw;   /* 硬件版本 */
    uint32_t fw;   /* 软件版本 */
} sys_version_t;

/* 固件信息定义（存储到bin文件指定偏移地址） */
typedef hq_packed struct {
    sys_version_t ver;
    char          describe[20];
} firmware_info_t;

extern const sys_version_t SYS_VERSION; /* 版本信息 */

void hq_sys_init(void);
int  dac_camera_lock(void);
int  dac_camera_unlock(void);
void reset_reason(void);

int  cmd_version(int op, void *data, int len, void *pkt);
int  sys_reboot(int op, void *data, int len, void *pkt);
void dev_info_init(sys_state_t *para);
bool is_valid_baudrate(int baudrate);

#endif /* _SYSTEM_H_ */
