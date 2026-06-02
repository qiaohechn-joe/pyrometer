/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：proto_coef.c
 * 文件描述：校准系数配置
 * 作    者：和其光电嵌软团队
 * 备    注：算法修改添加说明在文末
 * 更新记录：
 *           V1.0 2024/01/26 陈  军  初始版本
 *           V1.1 2024/11/07 张晓博  新增多项式参数写入与查询
 *           V1.2 2025/01/23 李  超  新增矩阵式参数写入与查询
 *           V1.3 2025/03/07 李兆越  校准系数协议框架优化
 *           V1.4 2025/03/18 李兆越  错误码及异常情况优化
 */

#include "platform.h"
#include "proto_coef.h"
#include "para.h"
#include "para_cfg.h"

/*
 * 功能描述：解析对数拟合算法配置
 * 入参说明：input --- 待解析字符串
 * 返 回 值：0：解析成功，其他：失败
 */
int parse_log_para(char *input) {
    char      *token;
    log_para_t log_para;

    /* 解析 ch_idx */
    if (!(token = strtok(input, ","))) return ALGO_PARSE_ERR_FORMAT;
    int ch_idx = atoi(token);
    if (ch_idx < 0 || ch_idx >= 4) return ALGO_PARSE_ERR_IDX;

    /* 解析 seg_idx */
    if (!(token = strtok(NULL, ","))) return ALGO_PARSE_ERR_FORMAT;
    int seg_idx = atoi(token);
    if (seg_idx < 0 || seg_idx >= 2) return ALGO_PARSE_ERR_IDX;

    /* 解析 threshold */
    token = strtok(NULL, ",");
    if (!token) return ALGO_PARSE_ERR_FORMAT;
    log_para.threshold = strtod(token, NULL);

    /* 解析 a.base 和 a.index */
    token = strtok(NULL, ",");
    if (!token) return ALGO_PARSE_ERR_FORMAT;
    log_para.coef[0].base = strtoll(token, NULL, 10);

    token = strtok(NULL, ",");
    if (!token) return ALGO_PARSE_ERR_FORMAT;
    log_para.coef[0].index = (int8_t)atoi(token);

    /* 解析 b.base 和 b.index */
    token = strtok(NULL, ",");
    if (!token) return ALGO_PARSE_ERR_FORMAT;
    log_para.coef[1].base = strtoll(token, NULL, 10);

    token = strtok(NULL, ",");
    if (!token) return ALGO_PARSE_ERR_FORMAT;
    log_para.coef[1].index = (int8_t)atoi(token);

    /* check,暂未使用 */
    token = strtok(NULL, ",");
    if (!token) return ALGO_PARSE_ERR_FORMAT;

    /* 检查是否有多余参数 */
    token = strtok(NULL, ",");
    if (token) return ALGO_PARSE_ERR_PARA_TOO_MUCH;

    memcpy(&sys_para.algo.log_para[ch_idx][seg_idx], &log_para, sizeof(log_para_t));

    return ALGO_PARSE_OK;
}

/*
 * 功能描述：解析多项式算法配置
 * 入参说明：input --- 待解析字符串
 * 返 回 值：0：解析成功，其他：失败
 */
int parse_poly_para(char *input) {
    char       *token;
    poly_para_t poly_para;

    /* 解析 seg_idx */
    if (!(token = strtok(input, ","))) return ALGO_PARSE_ERR_FORMAT;
    int seg_idx = atoi(token);
    if (seg_idx < 0 || seg_idx >= POLY_PARA_SEG_MAX) return ALGO_PARSE_ERR_IDX;

    /* 解析 poly_term */
    if (!(token = strtok(NULL, ","))) return ALGO_PARSE_ERR_FORMAT;
    int poly_term = atoi(token);
    if (poly_term < 0 || poly_term > POLY_PARA_TERM_MAX) return ALGO_PARSE_ERR_IDX;
    poly_para.poly_term = (uint8_t)poly_term;

    /* 解析 subtrahend */
    token = strtok(NULL, ",");
    if (!token) return ALGO_PARSE_ERR_FORMAT;
    poly_para.subtrahend = strtof(token, NULL);

    /* 解析系数 coef[0] ~ coef[poly_term] */
    for (int i = 0; i < poly_term; i++) {
        token = strtok(NULL, ",");
        if (!token) return ALGO_PARSE_ERR_FORMAT;
        poly_para.coef[i].base = strtoll(token, NULL, 10);

        token = strtok(NULL, ",");
        if (!token) return ALGO_PARSE_ERR_FORMAT;
        poly_para.coef[i].index = (int8_t)atoi(token);
    }

    /* check,暂未使用 */
    token = strtok(NULL, ",");
    if (!token) return ALGO_PARSE_ERR_FORMAT;

    /* 检查是否有多余参数 */
    token = strtok(NULL, ",");
    if (token) return ALGO_PARSE_ERR_PARA_TOO_MUCH;

    /* 将解析后的参数保存到全局变量 */
    memcpy(&sys_para.algo.poly_para[seg_idx], &(poly_para.poly_term), sizeof(poly_para_t) - (POLY_PARA_TERM_MAX - poly_term) * sizeof(coef_t));

    return ALGO_PARSE_OK;
}

/*
 * 功能描述：解析对数拟合算法配置
 * 入参说明：input --- 待解析字符串
 * 返 回 值：0：解析成功，其他：失败
 */
int parse_network_para(char *input, cmd_id_e cmd_idx) {
    char *token;
    char *data = input;
    union {
        uint32_t hex;
        float    data;
    } data_recev;

    network_para_t network_para;
    memcpy(&network_para, &sys_para.algo.network_para, sizeof(network_para_t));
    /*第一个token 本次包含多少个数据*/
    token = strtok(data, " ");
    if (!token) return ALGO_PARSE_ERR_FORMAT;
    int data_number = atoi(token);
    if (data_number < NETWORK_PARA_NEURON_NUM_W || data_number > (NETWORK_PARA_LAYER1_NEURON_NUM_B + NETWORK_PARA_LAYER2_NEURON_NUM_B)) {
        return ALGO_PARSE_ERR_FORMAT;
    }
    /*解析数据*/
    for (int i = 0; i < data_number; i++) {
        token = strtok(NULL, " ");
        if (!token) return ALGO_PARSE_ERR_FORMAT;
        data_recev.hex = (uint32_t)strtoul(token, NULL, 16);
        switch ((int)cmd_idx) {
            case (int)CMD_NWA: network_para.effective_w1[i] = data_recev.data; break;
            case (int)CMD_NWB: network_para.effective_w2[i] = data_recev.data; break;
            case (int)CMD_NB:
                if (i < NETWORK_PARA_LAYER1_NEURON_NUM_B) {
                    network_para.effective_b1[i] = data_recev.data;
                } else {
                    network_para.effective_b2[i - NETWORK_PARA_LAYER1_NEURON_NUM_B] = data_recev.data;
                }
                break;
            default: break;
        }
    }
    //    /*校验功能，预留*/
    //    token = strtok(NULL, " ");
    //    if (!token) return ALGO_PARSE_ERR_FORMAT;
    /*检查是否还有多余参数*/
    token = strtok(NULL, " ");
    if (token) return ALGO_PARSE_ERR_FORMAT; /*格式不对，参数多余，返回错误*/
    memcpy(&sys_para.algo.network_para, &network_para, sizeof(network_para_t));
    return ALGO_PARSE_OK;
}

/*
 * 功能描述：解析算法数据包，保存到全局变量
 * 入参说明：pkt_parse --- 包解析结构体指针
 * 返 回 值：0：加载算法成功，其他：失败
 */
int algo_load(parse_t *pkt_parse) {
    if (!pkt_parse || pkt_parse->data_len <= ALGO_HDR_SIZE) {
        return ALGO_PARSE_ERR_NULL;
    }

    char *data = pkt_parse->data;
    char *token;

    /* 解析 op, todo冗余字段，待删除 */
    if (!(token = strtok(data, ",")) || (*token != '0' && *token != '1')) {
        //        return ALGO_PARSE_ERR_FORMAT;
        //    char op = *token;
    }

    /* 解析 type */
    if (!(token = strtok(NULL, ",")) || *token < COEF_TYPE_LOG + '0' || *token > COEF_TYPE_MATRIX + '0') return ALGO_PARSE_ERR_FORMAT;
    coef_type_e type = (coef_type_e)atoi(token);

    /* 解析不同类型算法数据 */
    data += ALGO_HDR_SIZE;
    int ret = ALGO_PARSE_OK;
    switch (type) {
        case COEF_TYPE_LOG:
            ret = parse_log_para(data); /* 传入剩余字符串，让 parse_log 自己解析 */
            break;
        case COEF_TYPE_POLY: ret = parse_poly_para(data); break;
        case COEF_TYPE_MATRIX:
            // ret = parse_matrix_para(data);
            break;
        default: ret = ALGO_PARSE_ERR_TYPE; break;
    }

    if (ret == ALGO_PARSE_OK) {
    }
    return ret;
}

/*
 * 功能描述：获取算法配置数据
 * 入参说明：pkt_parse --- 包解析结构体指针
 * 返 回 值：0：获取成功，其他：获取失败
 */
int algo_get(parse_t *parse) {
    char    *outbuf = parse->data;
    uint16_t outlen = 0;
    /* 对数 */
    // outlen += sprintf(outbuf + outlen, "!AP");
    for (int ch = 0; ch < LOG_PARA_CH_MAX; ch++) {
        for (int seg = 0; seg < LOG_PARA_SEG_MAX; seg++) {
            log_para_t *ptr = &sys_para.algo.log_para[ch][seg];
            outlen += sprintf(outbuf + outlen, "1,0,%d,%d,%lf,", ch, seg, ptr->threshold);
            outlen += sprintf(outbuf + outlen, "%lld,%d,%lld,%d,0;", ptr->coef[0].base, ptr->coef[0].index, ptr->coef[1].base, ptr->coef[1].index);
            if (outlen > PARSE_DATA_MAX_LEN) return -1;
        }
    }

    /* 多项式 */
    for (int seg = 0; seg < POLY_PARA_SEG_MAX; seg++) {
        poly_para_t *ptr = &sys_para.algo.poly_para[seg];
        outlen += sprintf(outbuf + outlen, "1,1,%d,%d,%f,", seg, ptr->poly_term, ptr->subtrahend);
        for (int i = 0; i < ptr->poly_term; i++) {
            outlen += sprintf(outbuf + outlen, "%lld,%d,", ptr->coef[i].base, ptr->coef[i].index);
            if (outlen > PARSE_DATA_MAX_LEN) return -1;
        }
        if (seg == (POLY_PARA_SEG_MAX - 1)) {
            outlen += sprintf(outbuf + outlen, "0;");
        } else {
            outlen += sprintf(outbuf + outlen, "0\r\n");
        }
        if (outlen > PARSE_DATA_MAX_LEN) return -1;
    }

    parse->data_len    = outlen;
    parse->data_out_en = 1;
    return 0;
}

/*
 * 功能描述：解析神经网络算法数据包，保存到全局变量
 * 入参说明：pkt_parse --- 包解析结构体指针
 * 返 回 值：0：加载算法成功，其他：失败
 */
int algo_network_para_load(parse_t *pkt_parse, cmd_id_e cmd_idx) {
    if (!pkt_parse || pkt_parse->data_len <= ALGO_HDR_SIZE) {
        return ALGO_PARSE_ERR_NULL;
    }

    char *data = pkt_parse->data;

    /* 解析不同类型算法数据 */
    int ret = ALGO_PARSE_OK;
    ret     = parse_network_para(data, cmd_idx); /* 传入剩余字符串，让 parse_log 自己解析 */

    return ret;
}

/*
 * 功能描述：获取神经网络算法配置数据
 * 入参说明：pkt_parse --- 包解析结构体指针
 * 返 回 值：0：获取成功，其他：获取失败
 */
int algo_network_para_get(parse_t *parse, cmd_id_e cmd_idx) {
    char    *outbuf = parse->data;
    uint16_t outlen = 0;

    int data_len = 0;

    uint32_t *temp_para = NULL;
    switch (cmd_idx) {
        case CMD_NWA:
            outlen += sprintf(outbuf + outlen, "64");
            data_len  = NETWORK_PARA_NEURON_NUM_W;
            temp_para = (uint32_t *)&sys_para.algo.network_para.effective_w1;
            break;
        case CMD_NWB:
            outlen += sprintf(outbuf + outlen, "64");
            data_len  = NETWORK_PARA_NEURON_NUM_W;
            temp_para = (uint32_t *)&sys_para.algo.network_para.effective_w2;
            break;
        case CMD_NB:
            outlen += sprintf(outbuf + outlen, "65");
            data_len  = NETWORK_PARA_LAYER1_NEURON_NUM_B + NETWORK_PARA_LAYER2_NEURON_NUM_B;
            temp_para = (uint32_t *)&sys_para.algo.network_para.effective_b1;
            break;
        default: break;
    }
    if (temp_para == NULL) return -1;

    uint32_t hex;
    for (int ch = 0; ch < data_len; ch++) {
        if (ch == NETWORK_PARA_LAYER1_NEURON_NUM_B) {
            memcpy((void *)&hex, (const void *)sys_para.algo.network_para.effective_b2, sizeof(float));
            outlen += sprintf(outbuf + outlen, " %08X", hex);
        } else {
            hex = temp_para[ch];
            outlen += sprintf(outbuf + outlen, " %08X", hex);
        }
        if (outlen > PARSE_DATA_MAX_LEN) return -1;
    }

    parse->data_len    = outlen;
    parse->data_out_en = 1;
    return 0;
}
