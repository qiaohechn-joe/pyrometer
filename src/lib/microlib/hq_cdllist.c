/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：hq_cdllist.c
 * 文件描述：循环双向链表管理库（circular double list）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#include "hq_cdllist.h"

/*
 * 功能描述：此函数初始化循环双链表的头部
 * 入参说明：head --- 指向链表头部的指针
 * 返 回 值：无
 */
void list_init(struct list_head_t *head) {
    head->next = head;
    head->prev = head;
}

/*
 * 功能描述：此函数向链表中添加一个项目
 * 入参说明：item --- 要添加的链表项目的指针
 *           head --- 指向链表头部的指针
 * 返 回 值：无
 * 备    注：项目被插入到链表头之后，用于构建栈（后进先出）
 */
void list_add(struct list_head_t *item, struct list_head_t *head) {
    head->next->prev = item;
    item->next       = head->next;
    item->prev       = head;
    head->next       = item;
}

/*
 * 功能描述：此函数向链表中添加一个项目
 * 入参说明：item --- 要添加的链表项目的指针
 *           head --- 指向链表头部的指针
 * 返 回 值：无
 * 备    注：项目被插入到链表头之前，用于构建队列（先进先出）
 */
void list_add_tail(struct list_head_t *item, struct list_head_t *head) {
    item->prev       = head->prev;
    head->prev->next = item;

    item->next = head;
    head->prev = item;
}

/*
 * 功能描述：此函数从链表中移除一个项目
 * 入参说明：item --- 要移除的链表项目的指针
 *           head --- 指向链表头部的指针
 * 返 回 值：无
 */
void list_del(struct list_head_t *item) {
    item->next->prev = item->prev;
    item->prev->next = item->next;

    item->next = item;
    item->prev = item;
}
