/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：lwip.h
 * 文件描述：LWIP模块顶层代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#ifndef _LWIP_H_
#define _LWIP_H_

#include "lwipopts.h"
#include "lwip/netif.h"
#include "lwip/api.h"
#include "lwip/ip.h"

/* DHCP服务器最大重试次数 */
#define LWIP_MAX_DHCP_TRIES 3

int lwip_stack_init(void);

#endif /* _LWIP_H_ */
