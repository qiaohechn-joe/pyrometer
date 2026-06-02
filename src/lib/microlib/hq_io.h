/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：hq_io.h
 * 文件描述：输入输出库
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/06/05 陈军  调试等级迁移到lib_cfg.h
 *           V1.2 2025/03/13 陈军  调试口串口速率配置
 */

#ifndef _HQ_IO_H_
#define _HQ_IO_H_

#include <stdio.h>

#define DBG_LEN_MAX   32

#define DBG_IO_RTT    1

#define DBG_PORT_DIFF 2

void  dbg_init(const char *level, unsigned int rate, const char *fmt);
char *dbg_get_level(void);
void  dbg_set_level(const char *str);
int   dbg_printf(const char *level_str, const char *fmt, ...);
int   dbg_level_enabled(const char *level_str);
int   dbg_print_buffer(const char *level_char, const char *title, const void *buff, int len, int line_len);
int   getch(void);
int   getch_timeout(int *ch, unsigned long timeo);

int putch(int ch);
int puts(const char *str);
int putd(unsigned long val);
int putx(unsigned long val, int digits);
int putf(float val);
int print_buffer(const char *title, const void *buff, int len, int line_len);

#endif /* _HQ_IO_H_ */
