/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ad7124_cfg.h
 * 文件描述：ad7124配置程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/11/18  张晓博、乔鹤 初始版本
 *           V1.1 2025/03/08  陈军 整理
 */

#ifndef _AD7124_CFG_H_
#define _AD7124_CFG_H_

#include "ad7124.h"

#define AD7124_CHANNEL_COUNT 8

extern ad7124_reg_t AD7124_DEF_REG_CFG0[AD7124_REG_CNT];

#endif /* _AD7124_CFG_H_ */
