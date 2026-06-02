/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：tiag.h
 * 文件描述：跨阻放大器增益档位控制驱动代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/03/20 陈军  初始版本
 */

#ifndef _TIAG_H_
#define _TIAG_H_

#include "platform.h"
#include "tiag_cfg.h"

#define TIAG_AUTO_MODE 0 /* 跨阻档位自动切换模式 */
#define TIAG_CMD_MODE  1 /* 跨阻档位命令切换模式 */

#define TIAG_NB_TYPE   0 /* 窄带跨阻类型 */
#define TIAG_WB_TYPE   1 /* 宽带跨阻类型 */

/* 跨阻档位（L M H） */
#define GAIN_L         0
#define GAIN_M         1
#define GAIN_H         2
#define GAIN_CNT       3

/* 切换挡位功能函数 */
extern void tiag_init(void);
extern void tiag_switch(uint8_t ch_spec, uint32_t volt, uint8_t gain);

#endif /* _TIAG_H_ */
