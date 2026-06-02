/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：proto_coef.h
 * 文件描述：校准系数配置
 * 作    者：和其光电嵌软团队
 * 备    注：
 *           对数型系数内容解析（type = 0，函数模型：y=alnx+b）
 *           ------------------------------------------------------------------------------------------
 *           | 字段名     | 类型/范围                    | 描述                                        |
 *           ------------------------------------------------------------------------------------------
 *           | op         | 枚举 {0: 读, 1: 写}          | 操作类型                                    |
 *           | type       | 固定值 0                     | 系数类型：对数型                            |
 *           | ch_idx     | 整数 [0 ~ 4]                 | 通道索引                                    |
 *           | seg_idx    | 整数 [0 ~ 1]                 | 分段索引（最多支持2段）                     |
 *           | threshold  | 浮点数                       | 当前段的阈值，超过后不可使用该函数段转化温度|
 *           | coef[0]      | poly_coef_t { base, index }| 系数a                                       |
 *           | coef[1]      | poly_coef_t { base, index }| 系数b                                       |
 *           | check      | 校验位                       | 数据完整性校验（暂未启用）                  |
 *           ------------------------------------------------------------------------------------------
 *
 *           示例：
 *           1,0,0,0,2.482,-66399678540,-5,16016335870,-3,0
 *
 *           多项式型系数内容解析（type = 1）
 *           ------------------------------------------------------------------------------------------
 *           | 字段名       | 类型/范围                  | 描述                                        |
 *           ------------------------------------------------------------------------------------------
 *           | op           | 枚举 {0: 读, 1: 写}        | 操作类型                                    |
 *           | type         | 固定值 1                   | 系数类型：多项式型                          |
 *           | seg_idx      | 整数 [0 ~ 2]               | 多项式段数索引                              |
 *           | poly_term    | 整数 [1 ~ 15]              | 多项式项数                                  |
 *           | subtrahend   | 浮点数，默认 0.000         | t0 值（减数）                               |
 *           | coef[0]      | poly_coef_t { base, index }| 第0次项系数                                 |
 *           | ...          | ...                        | 中间各项系数（省略）                        |
 *           | coef[14]     | poly_coef_t { base, index }| 第14次项系数                                |
 *           | check        | 校验位                     | 数据完整性校验（暂未启用）                  |
 *           ------------------------------------------------------------------------------------------
 *           示例：
 *           1,1,0,14,0.000,-78611614501,-1,-78611614502,-2,...,-78611614501,8,check
 *
 * 更新记录：
 *           V1.0 2024/01/26 陈  军  初始版本
 *           V1.1 2024/11/07 张晓博  新增多项式参数写入与查询
 *           V1.2 2025/01/23 李  超  新增矩阵式参数写入与查询
 *           V1.3 2025/03/07 李兆越  校准系数协议框架优化
 *           V1.4 2025/03/18 李兆越  错误码及异常情况优化
 *           V1.5 2025/06/18 段政宏  重构算法参数处理模块
 */

#ifndef _PROTO_COEF_H_
#define _PROTO_COEF_H_

#include "proto_char_cfg.h"
#include "platform.h"

#define NOW_ALGO                         COEF_TYPE_MULTI /* 当前使用算法 */
#define ALGO_HDR_SIZE                    4

#define COEF_OP_SET                      '1' /* 设置系数 */
#define COEF_OP_GET                      '0' /* 查询系数 */

#define LOG_PARA_CH_MAX                  2 /* 单项式最大通道数 */
#define LOG_PARA_SEG_MAX                 2 /* 单项式最大段数 */

#define POLY_PARA_SEG_MAX                2  /* 多项式最大段数 */
#define POLY_PARA_TERM_MAX               15 /* 多项式最大项数 */

#define MATRIX_PARA_ROW_MAX              6 /* 矩阵式最大行数 */

#define NETWORK_PARA_NEURON_NUM_W        64 /*神经网络权重数量 */
#define NETWORK_PARA_LAYER1_NEURON_NUM_B 64 /*神经网络第一层bias数量 */
#define NETWORK_PARA_LAYER2_NEURON_NUM_B 1  /*神经网络第二层bias数量 */

/* 错误码定义 */
#define ALGO_PARSE_OK                    0  // 成功
#define ALGO_PARSE_ERR_NULL              -1 // 输入为 NULL
#define ALGO_PARSE_ERR_TYPE              -2 // 类型不支持
#define ALGO_PARSE_ERR_FORMAT            -3 // 数据格式错误
#define ALGO_PARSE_ERR_IDX               -4 // 索引越界
#define ALGO_PARSE_ERR_PARA_TOO_MUCH     -5

// 每种 type 对应不同的解析方式
typedef enum {
    COEF_TYPE_LOG    = 0,
    COEF_TYPE_POLY   = 1,
    COEF_TYPE_MATRIX = 2,
} coef_type_e;

/* 校准系数相关结构体 */
typedef hq_packed struct { /* 9字节 */
    long long int base;    /* 底数 */
    int8_t        index;   /* e的指数 */
} coef_t;

typedef hq_packed struct { /* 22字节 */
    double threshold;      /* 当前段的阈值，超过后不可使用该段函数转化温度 */
    coef_t coef[2];        /* 系数a, b */
} log_para_t;

typedef hq_packed struct {            /* 22字节 */
    uint8_t poly_term;                /*项数 */
    float   subtrahend;               /* t0 */
    coef_t  coef[POLY_PARA_TERM_MAX]; /* an */
} poly_para_t;

typedef hq_packed struct { /* 22字节 */
    uint16_t row_num;      /* 行数 */
    float    subtrahend;
    coef_t   coef[6]; /* 系数 9*6 = 54 54*6 = 324*/
} matrix_para_t;

typedef hq_packed struct {
    float effective_w1[NETWORK_PARA_NEURON_NUM_W];        /* 神经网络参数，第一层权重 */
    float effective_b1[NETWORK_PARA_LAYER1_NEURON_NUM_B]; /* 神经网络参数，第一层bias */
    float effective_w2[NETWORK_PARA_NEURON_NUM_W];        /* 神经网络参数，第二层权重 */
    float effective_b2[NETWORK_PARA_LAYER2_NEURON_NUM_B]; /* 神经网络参数，第二层bias */
} network_para_t;

typedef hq_packed struct {
    log_para_t  log_para[LOG_PARA_CH_MAX][LOG_PARA_SEG_MAX]; /* 单项式系数。通道数,段数 */
    poly_para_t poly_para[POLY_PARA_SEG_MAX];                /* 多项式系数 */
    // matrix_para_t matrix[MATRIX_PARA_ROW_MAX]; /* XKL 算法矩阵系数 9*6 = 54 54*6 = 324*/
    network_para_t network_para; /*神经网络温度转化算法参数*/
} algo_para_t;

int algo_load(parse_t *pkt_parse);
int algo_get(parse_t *pkt_parse);
int algo_network_para_load(parse_t *pkt_parse, cmd_id_e cmd_idx);
int algo_network_para_get(parse_t *pkt_parse, cmd_id_e cmd_idx);

#endif /* _PROTO_COEF_H_ */
