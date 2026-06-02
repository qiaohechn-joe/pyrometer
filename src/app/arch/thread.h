/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：thread.h
 * 文件描述：用户线程管理
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#ifndef _THREAD_H_
#define _THREAD_H_

#include "platform.h"

typedef void (*thread_entry_t)(void *arg);

int thread_init(void);
int thread_create(int num, thread_entry_t entry, void *arg, int stk_size);

#endif /* _THREAD_H_ */
