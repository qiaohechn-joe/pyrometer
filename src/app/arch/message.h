/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：message.h
 * 文件描述：消息队列管理
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/12/02 陈军  消息配置移出和结构体定义优化
 */

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "platform.h"

void msg_init(void);
void msg_flush_queue(void);
int  msg_count(void);
#define msg_queue_empty()     (msg_count() == 0)
#define msg_queue_full()      (msg_count() == MSG_QUEUE_SIZE)

/* 消息：unsigned int，bit[31:8]=值, bit[7:0]=类型 */
#define msg_type(msg)         ((msg >> 24) & 0x000f)
#define msg_value(msg)        (msg & 0x0fff)
#define msg_make(type, value) ((type << 24) | value)

void     msg_enqueue(uint32_t type, uint32_t value);
uint32_t msg_dequeue(void);

#endif /* _MESSAGE_H_ */
