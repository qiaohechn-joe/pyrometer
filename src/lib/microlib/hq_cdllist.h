/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：hq_cdllist.h
 * 文件描述：循环双向链表管理库（circular double list）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#ifndef _HQ_CDLLIST_H_
#define _HQ_CDLLIST_H_

/* 循环双向链表 */
struct list_head_t {
    struct list_head_t *next;
    struct list_head_t *prev;
};

/* 初始化一个链表 */
void list_init(struct list_head_t *head);
/* 在头部之后添加一个项目（栈）：head---item---head->next */
void list_add(struct list_head_t *item, struct list_head_t *head);
/* 在头部之前添加一个项目（队列）：head->prev---item---head */
void list_add_tail(struct list_head_t *item, struct list_head_t *head);
/* 从其链表中删除项目 */
void list_del(struct list_head_t *item);

#define list_empty(head)              ((head)->next == (head))
#define list_entry(ptr, type, member) container_of(ptr, member, type)

#endif /* _HQ_CDLLIST_H_ */
