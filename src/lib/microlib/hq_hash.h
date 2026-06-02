/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：hq_hash.h
 * 文件描述：自定义哈希表
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/04/24 李兆越   初始版本
 */

#ifndef HQ_HASH_H
#define HQ_HASH_H

#include "platform.h"

/* 哈希表大小 (最好选择质数) */
#define HASH_TABLE_SIZE 101

/* 命令哈希表项结构 */
typedef struct cmd_hash_entry {
    char                  *cmd_str; /* 命令字符串 */
    uint16_t               cmd_id;  /* 命令ID */
    struct cmd_hash_entry *next;    /* 链表指针 */
} cmd_hash_entry_t;

/* 哈希表结构 */
typedef struct {
    cmd_hash_entry_t *table[HASH_TABLE_SIZE]; /* 哈希桶数组 */
    SemaphoreHandle_t lock;                   /* 互斥锁(线程安全) */
} cmd_hash_table_t;

void              cmd_hash_init(cmd_hash_table_t *ht);
void              cmd_hash_add(cmd_hash_table_t *ht, const char *cmd, uint16_t cmd_id);
cmd_hash_entry_t *cmd_hash_find(cmd_hash_table_t *ht, const char *cmd);
void              cmd_hash_remove(cmd_hash_table_t *ht, const char *cmd);
void              cmd_hash_clear(cmd_hash_table_t *ht);

#endif /* HQ_HASH_H */
