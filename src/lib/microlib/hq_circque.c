/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：circque.c
 * 文件描述：循环队列管理库
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：V1.0 2024/01/26 陈军  初始版本
 */

#include "stdio.h"
#include "circque.h"
#include "generic.h"

/*
 * 功能描述：此函数初始化循环队列
 * 入参说明：无
 * 返 回 值：循环队列结构体
 */
struct circ_queue_t *circque_init(void) {
    struct circ_queue_t *q = NULL;

    q->front = 0;
    q->rear  = 0;

    mem_set(q->data, sizeof(q->data), 0);

    return q;
}

/*
 * 功能描述：此函数判断循环队列是否已满
 * 入参说明：q --- 循环队列
 * 返 回 值：0 --- 失败，1 --- 成功
 */
int circque_full(struct circ_queue_t *q) {
    if (q == NULL) {
        return 0;
    }

    return (q->rear + 1) % QUEUE_MAX == q->front;
}

/*
 * 功能描述：此函数判断循环队列是否为空
 * 入参说明：q --- 循环队列
 * 返 回 值：0 --- 失败，1 --- 成功
 */
int circque_empty(struct circ_queue_t *q) {
    if (q == NULL) {
        return 0;
    }

    return q->front == q->rear;
}

/*
 * 功能描述：这个函数计算循环队列的当前长度
 * 入参说明：q --- 循环队列
 * 返 回 值：循环队列中当前存储的元素数量
 */
int circque_length(struct circ_queue_t *q) {
    if (q == NULL) {
        return 0;
    }

    return (q->rear - q->front + QUEUE_MAX) % QUEUE_MAX;
}

/*
 * 功能描述：这个函数进行入队操作
 * 入参说明：q --- 循环队列
 *           en_data --- 入队数据
 * 返 回 值：0 --- 失败，1 --- 成功
 */
int circque_write(struct circ_queue_t *q, unsigned char en_data) {
    /* 队列满 */
    if (queue_full(q)) {
        return 0;
    }

    /* 入队 */
    q->data[q->rear] = en_data;

    /* 更新队尾指针 */
    q->rear = (q->rear + 1) % QUEUE_MAX;

    return 1;
}

/*
 * 功能描述：这个函数进行出队操作
 * 入参说明：q --- 循环队列
 *           de_data --- 出队数据
 * 返 回 值：0 --- 失败，1 --- 成功
 */
int circque_read(struct circ_queue_t *q, unsigned char *de_data) {
    /* 队列满 */
    if (queue_empty(q)) {
        return 0;
    }

    /* 出队 */
    *de_data = q->data[q->front];

    /* 更新队首指针 */
    q->front = (q->front + 1) % QUEUE_MAX;

    return 1;
}
