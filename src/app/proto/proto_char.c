/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：proto_char.c
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

#include "platform.h"
#include "proto_char.h"

static parse_t upp_pkt_parse;

/*
 * 功能描述：该函数用于记录并输出UPP操作错误信息。
 * 入参说明：err   --- UPP错误码；
 *           outbuf --- 输出缓冲区地址；
 *           outlen --- 当前输出缓冲区长度（输入/输出参数）。
 * 返 回 值：无。
 * 备注说明：注意多点指令组网传输是混乱的，没有加主从机制；
 *           输出格式为："ERROR: [XX] - 错误描述\r\n"。
 */
static void upp_pkt_error(uint8_t err, char *outbuf, int *outlen) {
#if SUPPORT_UPP_ERR
    if (err == UPP_OK) {
        return;
    }
    *outlen += sprintf(outbuf, "ERROR: [%02d] - %s\r\n", err, ERR_INFO_TBL[err].desc);
#endif /* SUPPORT_UPP_ERR */
}

/*
 * 功能描述：该函数解析FLUKE产品帧格式的UPP包。
 * 入参说明：pkt_parse --- UPP包解析结构体。
 *       buff --- 输入UPP包指针。
 *       len --- 输入UPP包长度。
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
int parse_cmd_flk(parse_t *pkt_parse, char *buff) {
    parse_t *parse = pkt_parse;
    char    *pkt   = buff;

    /* 命令类型&长度解析 */
    if (*(parse->ptr) == UPP_OP_GET) {
        parse->op = UPP_OP_GET;
        parse->ptr++;
        uint8_t cmd_word_len = parse->cmd_len - (parse->ptr - pkt) - parse->delim_len;
        if (cmd_word_len < 1 || cmd_word_len > 4) {
            parse->err = UPP_ERR_CMD_LEN;
            return -1;
        }
        parse->cmd_word_len = cmd_word_len;

    } else {
        char *carr_pos = mem_search(pkt, pkt_parse->cmd_len, UPP_OP_SET);
        if (carr_pos == NULL) {
            parse->err = UPP_ERR_OPERATOR_ILLEGAL;
            return -1;
        }
        parse->op            = UPP_OP_SET;
        uint8_t cmd_word_len = carr_pos - parse->ptr;
        if (cmd_word_len < 1 || cmd_word_len > 4) {
            parse->err = UPP_ERR_CMD_LEN;
            return -1;
        }
        parse->cmd_word_len = cmd_word_len;
    }

    if (!check_uppercase(parse->ptr, parse->cmd_word_len)) {
        mem_copy(parse->cmd_word, parse->ptr, parse->cmd_word_len);
        parse->ptr += parse->cmd_word_len;
    } else {
        parse->err = UPP_ERR_NO_UPPERCASE;
        return -1;
    }

    /* 以上处理完成，ptr指向命令字的后一个字符 */

    /* 设置命令的数据解析 */
    if (parse->op == UPP_OP_SET) {
        parse->ptr++;
        if (parse->cmd_len - (parse->ptr - pkt) <= parse->delim_len) {
            parse->err = UPP_ERR_NO_SET_DATA;
            return -1;
        }
        parse->data_len = parse->cmd_len - (parse->ptr - pkt) - parse->delim_len;
        mem_copy(parse->data, parse->ptr, parse->data_len);
    }

    /* 广播地址只支持查询 */
    //    if (pkt_parse->multi_addr == BROADCAST_ADDR) {
    //        pkt_parse->op = UPP_OP_GET;
    //    }

    return 0;
}

/*
 * 功能描述：解析指令的目标地址
 * 入参说明：pkt --- UPP包包解析结构体。
 *       buff --- 输入UPP包指针。
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
static int get_multi_addr(parse_t *pkt_parse, char *pkt) {
    if (!is_pure_digit_n(pkt, ADDR_LEN)) {
        pkt_parse->addr_mode  = UPP_ADDR_MODE_SINGLE;
        pkt_parse->multi_addr = 0;
        pkt_parse->ptr        = pkt;
    } else if (!string_to_dec_n(&pkt_parse->multi_addr, pkt, ADDR_LEN)) {
        pkt_parse->addr_mode = UPP_ADDR_MODE_MULTI;
        pkt_parse->ptr       = pkt + ADDR_LEN;
    } else {
        pkt_parse->err = UPP_ERR_ADDR_ILLEGAL;
        return -1;
    }

    if (pkt_parse->multi_addr != sys_para.dev.addr && pkt_parse->multi_addr != BROADCAST_ADDR) {
        pkt_parse->err = UPP_OK; // 地址不匹配，无返回
        return -1;
    }

    if (pkt_parse->multi_addr > DEV_MAX_ADDR && pkt_parse->multi_addr != BROADCAST_ADDR) {
        pkt_parse->err = UPP_ERR_ADDR_OUT_RANGE;
        return -1;
    }

    return 0;
}

/*
 * 功能描述：该函数解析UPP包。
 * 入参说明：pkt_parse --- UPP包解析结构体。
 *           buff --- 输入UPP包指针。
 *           len --- 输入UPP包长度。
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
static int upp_parse_cmd(parse_t *pkt_parse, char *buff) {
    parse_t *parse = pkt_parse;
    char    *pkt   = buff;

    /* 地址解析 */
    if (get_multi_addr(parse, pkt)) {
        return -1;
    }
    parse->cmd_type = CMD_FLK;
    return parse_cmd_flk(parse, buff);
}

/*
 * 功能描述：该函数用于检索命令码是否存在于命令码表中
 * 入参说明：cmd_table --- 命令码表的地址
 *           cmd       --- 要检索的命令码缓存地址
 *           cmd_len   --- 命令码的长度
 * 返 回 值：命令列表中的位置，如果命令不存在则返回-1
 */
int cmd_found(const cmd_item_t *cmd_table, const char *cmd, uint8_t cmd_len) {
    for (int i = 0; i < CMD_CNT; i++) {
        if (strlen(cmd_table[i].cmd) == cmd_len && string_comp_n(cmd_table[i].cmd, cmd, cmd_len) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * 功能描述：检查数据格式
 * 入参说明：parse      --- 协议解析结构体
 *           rule       --- 数据校验规则
 *           data_bkp   --- 数据备份地址
 *           bkp_len    --- 数据备份长度
 * 返 回 值：命令列表中的位置，如果命令不存在则返回-1
 */
static bool cmd_data_chk(parse_t *parse, chk_rule_t rule, char *data_bkp, uint16_t *bkp_len) {
    void     *target_param = CMD_TBL[parse->cmd_idx].arg;
    uint32_t  u_value;
    long long ll_value;
    float     f_value;
    double    lf_value;
    char      c_value;

    /* 长度校验 */
    if ((parse->data_len < rule.min_len || parse->data_len > rule.max_len) && !sys_state.develop) {
        parse->err = UPP_ERR_DATA_LEN;
        return false;
    }
    /* 值校验 */
    switch (rule.type) {
        case FMT_U_8:
            *bkp_len = sizeof(uint8_t);
            memcpy(data_bkp, (char *)target_param, *bkp_len);
            /*纯数字设置增加所有字符全是数字的校验*/
            if (is_dec_number(parse->data, parse->data_len) != 1) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if (sscanf(parse->data, "%u", &u_value) < 1) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if ((u_value > rule.data.u_range.max || u_value < rule.data.u_range.min) && !sys_state.develop) {
                parse->err = UPP_ERR_DATA_RANGE;
                return false;
            }
            *(uint8_t *)target_param = (uint8_t)u_value;
            break;
        case FMT_U_16:
            *bkp_len = sizeof(uint16_t);
            memcpy(data_bkp, (char *)target_param, *bkp_len);
            if (is_dec_number(parse->data, parse->data_len) != 1) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if (sscanf(parse->data, "%u", &u_value) < 1) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if ((u_value > rule.data.u_range.max || u_value < rule.data.u_range.min) && !sys_state.develop) {
                parse->err = UPP_ERR_DATA_RANGE;
                return false;
            }
            memcpy(target_param, &u_value, sizeof(uint16_t));
            break;
        case FMT_U_32:
            *bkp_len = sizeof(uint32_t);
            memcpy(data_bkp, (char *)target_param, *bkp_len);
            if (is_dec_number(parse->data, parse->data_len) != 1) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if (sscanf(parse->data, "%u", &u_value) < 1) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if ((u_value > rule.data.u_range.max || u_value < rule.data.u_range.min) && !sys_state.develop) {
                parse->err = UPP_ERR_DATA_RANGE;
                return false;
            }
            memcpy(target_param, &u_value, sizeof(u_value));
            break;
        case FMT_LLD:
            *bkp_len = sizeof(long long);
            memcpy(data_bkp, (char *)target_param, *bkp_len);
            if (is_dec_number(parse->data, parse->data_len) != 1) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if (sscanf(parse->data, "%lld", &ll_value) < 1) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if (ll_value > rule.data.ll_range.max || ll_value < rule.data.ll_range.min) {
                parse->err = UPP_ERR_DATA_RANGE;
                return false;
            }
            memcpy(target_param, &ll_value, sizeof(ll_value));
            break;
        case FMT_LF:
            *bkp_len = sizeof(double);
            memcpy(data_bkp, (char *)target_param, *bkp_len);
            if (is_dec_number(parse->data, parse->data_len) == 0) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if (sscanf(parse->data, "%lf", &lf_value) < 1) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if (lf_value > rule.data.lf_range.max || lf_value < rule.data.lf_range.min) {
                parse->err = UPP_ERR_DATA_RANGE;
                return false;
            }
            memcpy(target_param, &lf_value, sizeof(lf_value));
            break;

        case FMT_F6:
            *bkp_len = sizeof(float);
            memcpy(data_bkp, (char *)target_param, *bkp_len);
            if (is_dec_number(parse->data, parse->data_len) == 0) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if (sscanf(parse->data, "%f", &f_value) < 1) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }

            if (is_need_conver_unit(CMD_TBL[parse->cmd_idx].cmd)) {
                float temp = 0;
                memcpy(&temp, &f_value, sizeof(f_value));
                /*进行单位转换*/
                f_value = convert_uint(temp, sys_para.temp.unit, TEMP_UNIT_INTERNAL);
            }
            if (f_value > rule.data.f_range.max || f_value < rule.data.f_range.min) {
                parse->err = UPP_ERR_DATA_RANGE;
                return false;
            }

            memcpy(target_param, &f_value, sizeof(f_value));
            break;
        case FMT_F:
            *bkp_len = sizeof(float);
            memcpy(data_bkp, (char *)target_param, *bkp_len);
            if (is_dec_number(parse->data, parse->data_len) == 0) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if (sscanf(parse->data, "%f", &f_value) < 1) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }

            if (is_need_conver_unit(CMD_TBL[parse->cmd_idx].cmd)) {
                float temp = 0;
                memcpy(&temp, &f_value, sizeof(f_value));
                /*进行单位转换*/
                f_value = convert_uint(temp, sys_para.temp.unit, TEMP_UNIT_INTERNAL);
            }

            if (f_value > rule.data.f_range.max || f_value < rule.data.f_range.min) {
                parse->err = UPP_ERR_DATA_RANGE;
                return false;
            }
            if (is_need_limitRange(CMD_TBL[parse->cmd_idx].cmd)) {
                if (f_value > sys_para.temp.dev_temp_max || f_value < sys_para.temp.dev_temp_min) {
                    parse->err = UPP_ERR_DATA_RANGE;
                    return false;
                }
            }

            memcpy(target_param, &f_value, sizeof(f_value));
            break;

        case FMT_S:
            *bkp_len = strlen((char *)parse->data);
            if (rule.data.s_range.str_chk) {
                memcpy(data_bkp, (char *)target_param, *bkp_len);
                if (!rule.data.s_range.str_chk(parse->data) && !sys_state.develop) {
                    parse->err = UPP_ERR_DATA_RANGE;
                    return false;
                }
            }
            strncpy(target_param, parse->data, *bkp_len);
            ((char *)target_param)[*bkp_len] = '\0'; // 安全做法：用最后一个字节放 \0
            break;
        case FMT_X:
            *bkp_len = sizeof(uint32_t);
            memcpy(data_bkp, (char *)target_param, *bkp_len);
            if (sscanf(parse->data, "%x", &u_value) < 1) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if (u_value > rule.data.u_range.max || u_value < rule.data.u_range.min) {
                parse->err = UPP_ERR_DATA_RANGE;
                return false;
            }
            memcpy(target_param, &u_value, sizeof(u_value));
            break;
        case FMT_C:
            *bkp_len = sizeof(char);
            memcpy(data_bkp, (char *)target_param, *bkp_len);
            if (sscanf(parse->data, "%c", &c_value) < 1) {
                parse->err = UPP_ERR_DATA_ILLEGAL;
                return false;
            }
            if (!strchr(rule.data.c_range.str, c_value) && !sys_state.develop) {
                parse->err = UPP_ERR_DATA_RANGE;
                return false;
            }
            *(char *)target_param = c_value;
            break;
        default:
            // 校验规则不完备错误
            break;
    }
    return true;
}

/*
 * 功能描述：该函数执行UPP包。
 * 入参说明：pkt_parse --- UPP包解析结构体。
 *       outbuf --- 输出UPP包指针。
 *       outlen --- 输出UPP包长度。
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
int upp_execute_cmd(parse_t *pkt_parse, char *outbuf, int *outlen) {
    parse_t *parse                     = pkt_parse;
    char     data_bkp[BURST_LIST_SIZE] = {0};
    int16_t  cmd_idx;

    /* 命令识别 */
    cmd_idx = cmd_found(CMD_TBL, parse->cmd_word, parse->cmd_word_len);
    if (cmd_idx == -1) {
        parse->err = UPP_ERR_NO_SUPPORT_CMD;
        return -1;
    } else {
        parse->cmd_idx = cmd_idx;
    }

    /* 属性校验 */
    if (((parse->op == UPP_OP_SET) && !(CMD_TBL[cmd_idx].att.props & S)) || ((parse->op == UPP_OP_GET) && !(CMD_TBL[cmd_idx].att.props & P))) {
        parse->err = UPP_ERR_OP_TYPE;
        return -1;
    }

    if ((CMD_TBL[cmd_idx].att.props & L) && (parse->op == UPP_OP_SET) && !sys_state.unlock) {
        parse->err = UPP_ERR_CMD_LOCKED;
        return -1;
    }

    /* 数据校验 */
    uint16_t bkp_len = 0;
    if (parse->op == UPP_OP_SET) {
        if (!cmd_data_chk(parse, CMD_TBL[parse->cmd_idx].rule, data_bkp, &bkp_len)) {
            return -1;
        } else if (upp_set_callback(parse, CMD_TBL[cmd_idx].cmd_id)) {
            /* 命令执行失败，恢复参数 */
            memcpy((char *)CMD_TBL[cmd_idx].arg, data_bkp, bkp_len);
            parse->err = UPP_ERR_DATA_ILLEGAL;
            return -1;
        }
    } else if (parse->op == UPP_OP_GET) {
        if (upp_get_callback(parse, CMD_TBL[cmd_idx].cmd_id)) {
            return -1;
        }
    }
    return 0;
}

/*
 * 功能描述：该函数是FLUCK轮询和设置命令回复帧的生成器。
 * 入参说明：pkt_parse --- UPP包解析结构体。
 *       para --- 回复帧有效载荷。
 *       outbuf --- 输出UPP包指针。
 * 返 回 值：回复帧长度（字节数）。
 */
int upp_rsp_build_flk(parse_t *pkt_parse, char *outbuf) {
    parse_t *parse  = pkt_parse;
    int      outlen = 0;

    if (parse->op == UPP_OP_SET && !(CMD_TBL[parse->cmd_idx].att.props & N)) {
        return 0;
    }

    /* 生成多点帧首 */
    if (parse->addr_mode == UPP_ADDR_MODE_MULTI) {
        if (pkt_parse->multi_addr == BROADCAST_ADDR) {
            outlen += sprintf(outbuf + outlen, "%0*d", ADDR_LEN, sys_para.dev.addr);
        } else {
            outlen += sprintf(outbuf + outlen, "%0*d", ADDR_LEN, parse->multi_addr);
        }
    }

    /* 生成命令码 */
    outlen += sprintf(outbuf + outlen, "!%s", CMD_TBL[parse->cmd_idx].cmd);

    /* 生成载荷 */
    float temp = 0;
    if (is_need_conver_unit(CMD_TBL[parse->cmd_idx].cmd)) {
        if (CMD_TBL[parse->cmd_idx].rule.type == FMT_F || CMD_TBL[parse->cmd_idx].rule.type == FMT_F6) {
            memcpy(&temp, CMD_TBL[parse->cmd_idx].arg, sizeof(float));
        }
        /*进行单位转换*/
        temp = convert_uint(temp, TEMP_UNIT_INTERNAL, sys_para.temp.unit);
        outlen += data_to_string((void *)&temp, CMD_TBL[parse->cmd_idx].rule.type, outbuf + outlen);
    } else {
        if (pkt_parse->data_out_en) {
            outlen += sprintf(outbuf + outlen, "%s", pkt_parse->data);
        } else {
            outlen += data_to_string(CMD_TBL[parse->cmd_idx].arg, CMD_TBL[parse->cmd_idx].rule.type, outbuf + outlen);
        }
    }

    /* 添加结束符 */
    outlen += sprintf(outbuf + outlen, "%s", "\r\n");

    return outlen;
}

/*
 * 功能描述：这个函数处理UPP单包
 * 入参说明：pkt_parse --- 输入UPP包指针
 *           buf --- 输出UPP包指针
 *           len --- 输出UPP包长度指针
 *           arg --- 入参
 * 返 回 值：无
 */
static void upp_cmd_handler(parse_t *parse, void *inbuf, void *outbuf, int *outlen) {
    /* 解析协议包 */
    if (upp_parse_cmd(parse, inbuf)) {
        upp_pkt_error(parse->err, outbuf, outlen);
        return;
    }

    /* 执行命令 */
    if (upp_execute_cmd(parse, (char *)outbuf, outlen)) {
        upp_pkt_error(parse->err, outbuf, outlen);
        return;
    }

    /* 构造应答 */
    *outlen += upp_rsp_build_flk(parse, outbuf);
}

/*
 * 功能描述：该函数初步检查UPP包。
 * 入参说明：pkt_parse --- UPP包解析结构体。
 *           buff --- 输入UPP包指针。
 *           len --- 输入UPP包长度。
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
static int check_packet(parse_t *pkt_parse, void *inbuf, int inlen) {
    char *pkt = (char *)inbuf;
    char *carr_pos; // 包尾/r位置

    /* 检查UPP长度 */
    if (inlen < 3) { /* 3为协议最小长度：?V\r */
        pkt_parse->err = UPP_ERR_LEN;
        printf("Err len = %d \r\n", inlen);
        return -1;
    }

    /* 检查UPP包尾 */
    carr_pos = mem_search(pkt, inlen, '\r');
    if (carr_pos == NULL) {
        pkt_parse->err = UPP_ERR_NO_CARRIAGE;
        return -1;
    }

    /* 计算单包长度 */
    pkt_parse->delim_len = (*(carr_pos + 1) == '\n') ? 2 : 1;
    pkt_parse->cmd_len   = (carr_pos - pkt) + pkt_parse->delim_len;

    return 0;
}

/*
 * 功能描述：该函数处理来自传输层的UPP包（支持连包）。
 * 入参说明：buf --- 输入UPP包指针。
 *           len --- 输入UPP包长度。
 *           pkt_handler --- 接收单包处理回调。
 *           arg --- 回调的入参。
 * 返 回 值：无。
 */
void upp_pkt_process(char *inbuf, int inlen, char *outbuf, int *outlen) {
    static parse_t *const pkt_parse = &upp_pkt_parse;

    while (inlen > 0) {
        mem_set(&upp_pkt_parse, sizeof(upp_pkt_parse), 0x00);

        /* 协议格式检查 */
        if (check_packet(pkt_parse, (void *)inbuf, inlen)) {
            printf("inbuf = %s\r\n", inbuf);
            upp_pkt_error(pkt_parse->err, outbuf, outlen);
            break;
        }

        /* 包处理 */
        upp_cmd_handler(pkt_parse, (void *)inbuf, outbuf, outlen);

        /* 下一包 */
        inlen -= pkt_parse->cmd_len;
        inbuf += pkt_parse->cmd_len;
        outbuf += (*outlen);
    }
}

/*
 * 功能描述：输出数据转换为字符串
 * 入参说明：data --- 待转换数据。
 *           fmt --- 转换格式。
 *           outbuf --- 转换输出地址。
 * 返 回 值：转换字符串的长度。
 */
uint16_t data_to_string(char *data, fmt_type_e fmt, char *outbuf) {
    float     f_value  = 0.0f;
    double    lf_value = 0.0;
    uint32_t  u_value  = 0;
    long long ll_value = 0;
    int       outlen   = 0;

    /* 生成载荷 */
    switch (fmt) {
        case FMT_U_32:
            memcpy(&u_value, data, sizeof(uint32_t));
            outlen = sprintf(outbuf, "%u", u_value);
            break;
        case FMT_U_16:
            memcpy(&u_value, data, sizeof(uint16_t));
            outlen = sprintf(outbuf, "%u", u_value);
            break;
        case FMT_U_8:
            memcpy(&u_value, data, sizeof(uint8_t));
            outlen = sprintf(outbuf, "%u", u_value);
            break;
        case FMT_LLD:
            memcpy(&ll_value, data, sizeof(long long));
            outlen = sprintf(outbuf, "%lld", ll_value);
            break;
        case FMT_LF:
            memcpy(&lf_value, data, sizeof(double));
            outlen = sprintf(outbuf, "%lf", lf_value);
            break;
        case FMT_F6:
            memcpy(&f_value, data, sizeof(float));
            outlen = sprintf(outbuf, "%.6f", f_value);
            break;
        case FMT_F:
            memcpy(&f_value, data, sizeof(float));
            outlen = sprintf(outbuf, "%.2f", f_value);
            break;
        case FMT_S: outlen = sprintf(outbuf, "%s", data); break;
        case FMT_X:
            memcpy(&u_value, data, sizeof(uint32_t));
            outlen = sprintf(outbuf, "%x", u_value);
            break;
        case FMT_C: outlen = sprintf(outbuf, "%c", *data); break;
        default: break;
    }

    return outlen;
}

/*
 * 功能描述：参数设置时判断是否需要进行单位转换
 * 入参说明：char *cmd      --- 传入的指令字符串
 * 返 回 值：true表示与要进行单位转换， false表示不需要进行单位转换
 */
bool is_need_conver_unit(char *cmd) {
    int index = 0;
    for (index = 0; TEMP_CMD_TBL[index] != NULL; index++) {
        if (strcmp(TEMP_CMD_TBL[index], cmd) == 0) {
            return true;
        }
    }
    return false;
}
/*
 * 功能描述：参数设置时判断是否需要再次对设置值范围进行限制
 * 入参说明：char *cmd      --- 传入的指令字符串
 * 返 回 值：true表示与要进行限制， false表示不需要进行限制
 */
bool is_need_limitRange(char *cmd) {
    int index = 0;
    for (index = 0; AdjRange_CMD_TBL[index] != NULL; index++) {
        if (strcmp(AdjRange_CMD_TBL[index], cmd) == 0) {
            return true;
        }
    }
    return false;
}
