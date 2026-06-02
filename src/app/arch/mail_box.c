/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：mail_box.c
 * 文件描述：邮箱管理
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/12/05 陈军  修改结构体定义格式
 */

#include "mail_box.h"

static int mbox_count;

/*
 * 功能描述：此函数初始化邮箱模块
 * 入参说明：无
 * 返 回 值：无
 */
void mbox_init(void) {
    mbox_count = 0;
}

/*
 * 功能描述：此函数创建一个邮箱
 * 入参说明：q_size --- 邮箱队列大小
 * 返 回 值：指向邮箱，NULL代表创建失败
 */
mbox_t *mbox_create(int q_size) {
    mbox_t *mb = NULL;

    if (mbox_count == MBOX_MAX_COUNT) {
        return NULL;
    }

    /* 分配邮箱 */
    mb = (mbox_t *)pvPortMalloc(sizeof(mbox_t));
    if (mb == NULL) {
        return NULL;
    }

    /* 创建队列 */
    if (q_size > MBOX_CAPACITY) {
        q_size = MBOX_CAPACITY;
    }
    mb->q = xQueueCreate(q_size, sizeof(void *));
    if (mb->q == NULL) {
        vPortFree(mb);
        return NULL;
    }

    mbox_count++;

    return mb;
}

/*
 * 功能描述：此函数删除一个邮箱
 * 入参说明：mb --- 指向要删除的邮箱
 * 返 回 值：0 --- 成功，其它 --- 失败
 */
int mbox_delete(mbox_t *mb) {
    if (mb == NULL) {
        return -1;
    }

    /* 删除队列 */
    vQueueDelete(mb->q);

    /* 释放内存 */
    vPortFree(mb);

    if (mbox_count > 0) {
        mbox_count--;
    }

    return 0;
}

/*
 * 功能描述：此函数清除一个邮箱
 * 入参说明：mb --- 指向要清除的邮箱
 * 返 回 值：0 --- 成功，其它 --- 失败
 */
int mbox_clear(mbox_t *mb) {
    BaseType_t ret;

    if (mb == NULL) {
        return -1;
    }

    ret = xQueueReset(mb->q);
    if (ret != pdPASS) {
        return -1;
    }

    return 0;
}

/*
 * 功能描述：此函数向邮箱发送一条消息
 * 入参说明：mb --- 指向邮箱
 *           msg --- 发送的消息
 * 返 回 值：0 --- 成功，其它 --- 失败
 */
int mbox_post(mbox_t *mb, void *msg) {
    BaseType_t ret;

    if (mb == NULL) {
        return -1;
    }

    ret = xQueueSend(mb->q, (void *)&msg, 0);
    if (ret != pdPASS) {
        return -1;
    }

    return 0;
}

/*
 * 功能描述：此函数从邮箱中获取一条消息
 * 入参说明：mb --- 指向邮箱
 *           timeo --- 等待超时（OS_TICKS）
 *           err --- 指向错误的指针（0=成功，其他=失败）
 * 返 回 值：从邮箱中获取的消息
 */
void *mbox_fetch(mbox_t *mb, uint32_t timeo, int *err) {
    void      *temp = NULL;
    BaseType_t ret;

    if (mb == NULL) {
        *err = 1;
        return NULL;
    }

    ret  = xQueueReceive(mb->q, &temp, timeo);
    *err = (ret != pdPASS) ? 1 : 0;

    return temp;
}
