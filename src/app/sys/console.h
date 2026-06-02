/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：console.h
 * 文件描述：控制台管理代码
 * 作    者：和其光电嵌软团队
 * 备    注：使用时注意打开OS的配置项configUSE_IDLE_HOOK
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "platform.h"

/* 提示符 */
#define CON_PROMPT        "\r$"

/* 命令缓存区 */
#define CON_CMD_BUFF_SIZE 256
#define CON_CMD_HIS_CNT   8

int console_init(void);

#endif /* _CONSOLE_H_ */
