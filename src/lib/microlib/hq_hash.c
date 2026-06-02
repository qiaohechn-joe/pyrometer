/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：hq_hash.c
 * 文件描述：自定义哈希表
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/04/24 李兆越   初始版本
 */

#include "hq_hash.h"

static uint32_t hash_func(const char *str);

/*
 * 功能描述：初始化哈希表。
 * 入参说明：ht --- 指向要初始化的哈希表。
 * 返 回 值：无。
 */
void cmd_hash_init(cmd_hash_table_t *ht) {
    memset(ht->table, 0, sizeof(ht->table));
    ht->lock = xSemaphoreCreateMutex();
    if (ht->lock == NULL) {
        dbg_error("Error: Failed to create mutex for hash table.\n");
    }
}

/*
 * 功能描述：添加命令到哈希表。
 * 入参说明：ht --- 指向哈希表。
 *           cmd --- 要添加的命令字符串。
 *           cmd_id --- 命令对应的ID。
 * 返 回 值：无。
 */
void cmd_hash_add(cmd_hash_table_t *ht, const char *cmd, uint16_t cmd_id) {
    uint32_t          index = hash_func(cmd);
    cmd_hash_entry_t *entry;

    /* 检查命令是否已存在 */
    xSemaphoreTake(ht->lock, portMAX_DELAY);
    for (entry = ht->table[index]; entry != NULL; entry = entry->next) {
        if (strcmp(entry->cmd_str, cmd) == 0) {
            /* 命令已存在，更新ID */
            entry->cmd_id = cmd_id;
            xSemaphoreGive(ht->lock);
            return;
        }
    }

    /* 创建新条目 */
    entry = (cmd_hash_entry_t *)pvPortMalloc(sizeof(cmd_hash_entry_t));
    if (entry == NULL) {
        dbg_error("Error: Failed to allocate memory for new hash entry.\n");
        xSemaphoreGive(ht->lock);
        return; /* 内存分配失败 */
    }

    /* 复制命令字符串 */
    entry->cmd_str = (char *)pvPortMalloc(strlen(cmd) + 1);
    if (entry->cmd_str == NULL) {
        dbg_error("Error: Failed to allocate memory for command string.\n");
        vPortFree(entry);
        xSemaphoreGive(ht->lock);
        return; /* 内存分配失败 */
    }
    strcpy(entry->cmd_str, cmd);

    entry->cmd_id    = cmd_id;
    entry->next      = ht->table[index]; /* 插入到链表头部 */
    ht->table[index] = entry;

    xSemaphoreGive(ht->lock);
}

/*
 * 功能描述：查找命令。
 * 入参说明：ht --- 指向哈希表。
 *           cmd --- 要查找的命令字符串。
 * 返 回 值：找到的哈希表项指针，未找到返回NULL。
 */
cmd_hash_entry_t *cmd_hash_find(cmd_hash_table_t *ht, const char *cmd) {
    uint32_t          index = hash_func(cmd);
    cmd_hash_entry_t *entry;

    xSemaphoreTake(ht->lock, portMAX_DELAY);
    for (entry = ht->table[index]; entry != NULL; entry = entry->next) {
        if (strcmp(entry->cmd_str, cmd) == 0) {
            xSemaphoreGive(ht->lock);
            return entry;
        }
    }
    xSemaphoreGive(ht->lock);
    return NULL;
}

/*
 * 功能描述：删除命令。
 * 入参说明：ht --- 指向哈希表。
 *           cmd --- 要删除的命令字符串。
 * 返 回 值：无。
 */
void cmd_hash_remove(cmd_hash_table_t *ht, const char *cmd) {
    uint32_t          index = hash_func(cmd);
    cmd_hash_entry_t *entry, *prev = NULL;

    xSemaphoreTake(ht->lock, portMAX_DELAY);
    for (entry = ht->table[index]; entry != NULL; prev = entry, entry = entry->next) {
        if (strcmp(entry->cmd_str, cmd) == 0) {
            if (prev == NULL) {
                /* 删除链表头节点 */
                ht->table[index] = entry->next;
            } else {
                prev->next = entry->next;
            }

            /* 释放内存 */
            vPortFree(entry->cmd_str);
            vPortFree(entry);
            break;
        }
    }
    xSemaphoreGive(ht->lock);
}

/*
 * 功能描述：清空哈希表。
 * 入参说明：ht --- 指向要清空的哈希表。
 * 返 回 值：无。
 */
void cmd_hash_clear(cmd_hash_table_t *ht) {
    int               i;
    cmd_hash_entry_t *entry, *next;

    xSemaphoreTake(ht->lock, portMAX_DELAY);
    for (i = 0; i < HASH_TABLE_SIZE; i++) {
        entry = ht->table[i];
        while (entry != NULL) {
            next = entry->next;
            vPortFree(entry->cmd_str);
            vPortFree(entry);
            entry = next;
        }
        ht->table[i] = NULL;
    }
    xSemaphoreGive(ht->lock);
}

/*
 * 功能描述：简单的字符串哈希函数（djb2算法）。
 * 入参说明：str --- 要计算哈希值的字符串。
 * 返 回 值：计算得到的哈希值。
 */
static uint32_t hash_func(const char *str) {
    uint32_t hash = 5381;
    int      c;

    while ((c = *str++) != '\0') {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash % HASH_TABLE_SIZE;
}
