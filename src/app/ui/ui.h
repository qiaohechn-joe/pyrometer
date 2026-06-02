/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ui.h
 * 文件描述：ui菜单界面管理
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/03/19 李兆越 初始版本
 *           V1.1 2025/09/04 张赛   重做ui控制操作逻辑
 */

#ifndef _UI_H_
#define _UI_H_

#include "platform.h"

#define KEY_SCAN_RATE_HZ (configTICK_RATE_HZ / 20) /* 50ms */
#define UI_TASK_RATE_HZ  (configTICK_RATE_HZ / 5)  /* 200ms */

#define EDIT_MODE        0 /* 编辑模式 */
#define GUIDE_MODE       1 /* 导航模式 */

typedef struct {
    int     main_page_id; /* 主菜单索引 */
    int     sub_page_id;  /* 子菜单索引 */
    int32_t dis_value;    /* 显示值 */
    int8_t  set_value;    /* 设定值增量 */
    int     current_para_index;
    int     current_para_num;
    bool    cfg_states;         /* 参数配置标志 */
    bool    set_states;         /* 设定设定值标志 */
    uint8_t ip_cfg_disvalue[4]; /* IP地址显示值(4段) */
    bool    lock_status;        /* 界面锁定状态 */
    bool    is_blinking;
} ui_t;

extern ui_t ui;

void ui_init(void);
void ui_task(void *arg);

#endif /* _UI_H_ */
