/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：proto_char.h
 * 文件描述：高温计字符协议驱动代码（UPP）
 * 作    者：和其光电嵌软团队
 * 备    注：
 *           1）多点指令不支持错误码回复
 *           2）支持一次多包发送
 *           3）支持广播
 *           4）多点指令只支持单组收发
 * 更新记录：
 *           V1.0 2024/01/26 陈军    初始版本
 *           V1.1 2025/01/05 李兆越  协议层、传输层解耦
 *           V1.2 2025/02/08 李兆越  加入数据检查函数
 *           V1.3 2025/02/25 李兆越  优化 LIM_TYPE_COMMA_SEP_NUM 限制
 *           V1.4 2025/03/26 李兆越  多包多次应答,加入命令枚举
 *           V1.5 2025/04/03 李兆越  支持至多四字符命令
 */

#ifndef _PROTO_CHAR_H_
#define _PROTO_CHAR_H_

#include <ctype.h>
#include "serial.h"
#include "para.h"
#include "proto_char_cfg.h"

/* UPP操作模式 */
#define UPP_SINGLE         0 /* 单点指令 */
#define UPP_MULTI          1 /* 多点指令 */

/* UPP操作符 */
#define UPP_OP_GET         '?' /* 表示指令用于查询 */
#define UPP_OP_SET         '=' /* 表示指令用于设置 */

/* UPP命令字符个数 */
#define UPP_CMD_1          1 /* 单字符命令 */
#define UPP_CMD_2          2 /* 双字符命令 */
#define UPP_CMD_3          3 /* 三字符命令 */
#define UPP_CMD_4          4 /* 四字符命令 */

/* 协议数据包长度 */
#define PROT_MIN_PKT_LEN   1
#define PROT_MAX_PKT_LEN   1056

#define CMD_NUM            10

/* 多点指令地址 */
#define UPP_MULTI_ADDR_LEN 3

extern const char *TEMP_CMD_TBL[];
extern const char *AdjRange_CMD_TBL[];
/* 命令处理单元 */
void     upp_pkt_process(char *inbuf, int inlen, char *outbuf, int *outlen);
int      assign_with_format(parse_t *pkt_parse, void *value, int format_index);
int      format_value_to_buf(const char *fmt, void *arg, char *outbuf, int buflen);
int      cmd_found(const cmd_item_t *cmd_table, const char *cmd, uint8_t cmd_len);
uint16_t data_to_string(char *data, fmt_type_e fmt, char *outbuf);
bool     is_need_conver_unit(char *cmd);
bool     is_need_limitRange(char *cmd);

#endif /* _PROTO_CHAR_H_ */
