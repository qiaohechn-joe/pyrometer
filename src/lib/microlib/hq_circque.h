/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：circque.h
 * 文件描述：循环队列管理库
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：V1.0 2024/01/26 陈军  初始版本
 */

#ifndef _CIRCQUEUE_H_
#define _CIRCQUEUE_H_

#define QUEUE_MAX 20 /* 循环队列的最大大小 */

/* 循环队列结构体 */
struct circ_queue_t {
    unsigned char front;           /* 头*/
    unsigned char rear;            /* 尾 */
    unsigned char data[QUEUE_MAX]; /* 数据 */
};

struct circ_queue_t *circque_init(void);
int                  circque_full(struct circ_queue_t *q);
int                  circque_empty(struct circ_queue_t *q);
int                  circque_length(struct circ_queue_t *q);
int                  circque_write(struct circ_queue_t *q, unsigned char en_data);
int                  circque_read(struct circ_queue_t *q, unsigned char *de_data);

#endif /* _CIRCQUEUE_H_ */
