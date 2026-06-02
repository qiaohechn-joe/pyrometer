/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：message.c
 * 文件描述：消息队列管理
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/12/02 陈军  消息配置移出和结构体定义优化
 */

#include "message.h"

typedef struct {
    uint32_t *start;
    uint32_t *end;
    uint32_t *head; /* 读指针 */
    uint32_t *tail; /* 写指针 */
    int       count;
} msg_queue_t;

static uint32_t    msg_data[MSG_QUEUE_SIZE]; /* 消息数据 */
static msg_queue_t msg_queue;                /* 队列 */

/*
 * 功能描述：此函数初始化消息队列
 * 入参说明：无
 * 返 回 值：无
 */
void msg_init(void) {
    msg_queue.start = &msg_data[0];
    msg_queue.end   = msg_queue.start + MSG_QUEUE_SIZE;
    msg_queue.head  = msg_queue.start;
    msg_queue.tail  = msg_queue.start;
    msg_queue.count = 0;
}

/*
 * 功能描述：此函数清空消息队列
 * 入参说明：无
 * 返 回 值：无
 */
void msg_flush_queue(void) {
    ENTER_CRITICAL();
    msg_queue.head  = msg_queue.start;
    msg_queue.tail  = msg_queue.start;
    msg_queue.count = 0;
    EXIT_CRITICAL();
}

/*
 * 功能描述：此函数获取消息队列中消息个数
 * 入参说明：无
 * 返 回 值：个数
 */
int msg_count(void) {
    return msg_queue.count;
}

/*
 * 功能描述：此函数入队一个消息
 * 入参说明：type --- 消息类型
 *           value --- 消息值
 * 返 回 值：无
 */
void msg_enqueue(uint32_t type, uint32_t value) {
    ENTER_CRITICAL();
    *msg_queue.tail = msg_make(type, value);
    msg_queue.tail++;
    if (msg_queue.tail == msg_queue.end) msg_queue.tail = msg_queue.start;
    if (msg_queue.count < MSG_QUEUE_SIZE) msg_queue.count++;
    EXIT_CRITICAL();
}

/*
 * 功能描述：此函数出队一个消息
 * 入参说明：无
 * 返 回 值：队列头的消息
 */
uint32_t msg_dequeue(void) {
    uint32_t msg;

    ENTER_CRITICAL();

    if (msg_queue_empty()) /* empty */
    {
        EXIT_CRITICAL();
        return MSG_NONE;
    }

    msg = *msg_queue.head;
    msg_queue.head++;
    if (msg_queue.head == msg_queue.end) msg_queue.head = msg_queue.start;
    msg_queue.count--;

    EXIT_CRITICAL();

    return msg;
}
