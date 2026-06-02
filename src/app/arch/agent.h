/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：agent.h
 * 文件描述：代理任务管理
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/12/05 陈军  修改结构体定义格式
 */

#ifndef _AGENT_H_
#define _AGENT_H_

#include "arch_cfg.h"
#include "platform.h"

typedef void (*agent_entry_t)(void *arg); /* 代理函数指针 */

/* 代理函数结构体 */
typedef struct {
    agent_entry_t entry; /* 入口 */
    void         *arg;   /* 参数 */
} agent_func_t;

/* 代理队列结构体  */
typedef struct {
    SemaphoreHandle_t sem;                     /* 同步信号量 */
    agent_func_t      array[AGENT_QUEUE_SIZE]; /* 函数数组 */
    agent_func_t     *head;                    /* 读指针 */
    agent_func_t     *tail;                    /* 写指针 */
    int               count;                   /* 计数 */
} agent_queue_t;

int  agent_init(void);
void agent_flush(int from_isr, int prio);
int  agent_post(int from_isr, int prio, agent_entry_t func, void *arg);

#endif /* _AGENT_H_ */
