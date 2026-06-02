/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：pc.h
 * 文件描述：PC接口业务代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#ifndef _PC_H_
#define _PC_H_

#include "serial.h"

#define PC_COM        COM2
#define PC_BAUD_RATE  115200u
#define PC_RX_TIMEOUT 20 /* 毫秒 */

/* RS232接收数据包长度上下限（字节数） */
#define PC_MIN_LEN    2
#define PC_MAX_LEN    1056

int  pc_init(void);
void pc_send(int port, const void *data, int len);

#endif /* _PC_H_ */
