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

#ifndef _MAIL_BOX_H_
#define _MAIL_BOX_H_

#include "platform.h"

typedef struct {
    QueueHandle_t q;
} mbox_t;

void    mbox_init(void);
mbox_t *mbox_create(int q_size);
int     mbox_delete(mbox_t *mb);
int     mbox_clear(mbox_t *mb);
int     mbox_post(mbox_t *mb, void *msg);
void   *mbox_fetch(mbox_t *mb, uint32_t timeo, int *err);

#define mbox_valid(mb)         (mb != NULL)
#define mbox_set_invalid(mb)   mb = NULL
#define mbox_tryfetch(mb, err) mbox_fetch(mb, 1, err)

#endif /* _MAIL_BOX_H_ */
