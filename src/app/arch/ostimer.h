/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ostimer.h
 * 文件描述：操作系统定时器管理
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/12/04 陈军  统一结构体定义方式
 *           V1.2 2024/12/24 李兆越  加入设置定时器优先级
 */

#ifndef _OSTIMER_H_
#define _OSTIMER_H_

#include "platform.h"

typedef void (*ostimer_func_t)(void *arg); /* 定时器函数指针 */

/* 定时器列表项 */
typedef struct {
    ostimer_func_t func;   /* 函数 */
    void          *arg;    /* 参数 */
    int            prio;   /* 代理优先级，AGENT_PRIO_xxx */
    uint32_t       start;  /* 起始时刻：OS_TICKS */
    uint32_t       period; /* 周期时刻：OS_TICKS */
    int            enable; /* 1=启用 0=禁用 */
} ostimer_list_item_t;

int  ostimer_init(void);
void ostimer_register(int id, ostimer_func_t func, void *arg, int prio, int period);
void ostimer_open(int id, int now);
void ostimer_close(int id);
void ostimer_set_period(int id, uint32_t period);
void ostimer_restart(int id);
void ostimer_set_prio(int id, int prio);
#endif /* _OSTIMER_H_ */
