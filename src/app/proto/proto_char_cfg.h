/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：proto_char_cfg.h
 * 文件描述：通用高温计协议驱动代码（UPP）
 * 作    者：和其光电嵌软团队
 * 备    注：1）多点指令不支持错误码回复
 *           2）支持一次多包发送
 *           3）支持广播
 *           4）多点指令只支持单组收发
 * 更新记录：
 *           V1.0 2024/01/26 陈军    初始版本
 *           V1.1 2025/02/08 李兆越  命令表优化
 *           V1.2 2025/03/18 李兆越  加入CF算法写入命令，加入18号错误
 *           V1.3 2025/05/19 李兆越  表驱动 + 函数指针实现通用格式解析
 */

#ifndef _PROTO_CHAR_CFG_H_
#define _PROTO_CHAR_CFG_H_

#include "para_cfg.h"
#include "system.h"

/* 解析函数统一类型 */
typedef int (*parse_func_t)(const char *src, void *dst);

/* 结构体定义 */
typedef struct {
    const char  *name;   /* 格式简写，比如 "d", "u", "f" */
    const char  *fmt;    /* sscanf 格式字符串 */
    int          size;   /* 类型大小，方便后续自动分配 */
    parse_func_t parser; /* 对应的解析函数 */
} format_entry_t;

/* 宏定义简化封装函数生成 */
#define DEFINE_PARSE_WRAPPER(name, ctype, fmt_str)                                                                                                   \
    static int parse_##name(const char *src, void *dst) {                                                                                            \
        return sscanf(src, fmt_str, (ctype *)dst);                                                                                                   \
    }

typedef enum {
    LIM_TYPE_NONE,          /* 无限制 */
    LIM_TYPE_MAX_LEN,       /* 最大长度限制 (字符串) */
    LIM_TYPE_RANGE,         /* 范围限制(low_th, up_th) */
    LIM_TYPE_SPECIAL_VAL,   /* 特定值限制 */
    LIM_TYPE_COMMA_SEP_NUM, /* 逗号分隔数字限制 */
    LIM_TYPE_COMMA_SEP_STR, /* 逗号分隔大写字符限制 */
} limit_type_t;

typedef enum { FMT_U_8 = 0, FMT_U_16, FMT_U_32, FMT_LLD, FMT_LF, FMT_F6, FMT_F, FMT_S, FMT_X, FMT_C } fmt_type_e;

/* 命令枚举 */
typedef enum {
    CMD_DA,
    CMD_ID,
    CMD_DN,
    CMD_DV,
    CMD_RS,
    CMD_PR,
    CMD_SC,
    CMD_UL,
    CMD_J,
    CMD_DEV,
    CMD_DB,
    CMD_DP,
    CMD_BR,
    CMD_CF,
    CMD_PM,
    CMD_NM,
    CMD_NH,
    CMD_WM,
    CMD_WH,
    CMD_NUP,
    CMD_WUP,
    CMD_NLM,
    CMD_WLM,
    CMD_NLH,
    CMD_WLH,
    CMD_GDN,
    CMD_GCN,
    CMD_AM,
    CMD_AL,
    CMD_AH,
    CMD_OAL,
    CMD_OAH,
    CMD_OI,
    CMD_IL,
    CMD_IH,
    CMD_AT,
    CMD_OV,
    CMD_VL,
    CMD_VH,
    CMD_AC,
    CMD_DM,
    CMD_GA,
    CMD_OFN,
    CMD_OFW,
    CMD_NV,
    CMD_WV,
    CMD_ONV,
    CMD_OWV,
    CMD_SN,
    CMD_SW,
    CMD_SM,
    CMD_U,
    CMD_DL,
    CMD_DH,
    CMD_M,
    CMD_NC,
    CMD_WC,
    CMD_E,
    CMD_NS,
    CMD_WS,
    CMD_S,
    CMD_SI,
    CMD_ON,
    CMD_OW,
    CMD_VV,
    CMD_ATC,
    CMD_TTCO,
    CMD_CA,
    CMD_TCLS,
    CMD_TCLR,
    CMD_TCHS,
    CMD_TCHR,
    CMD_WTGA,
    CMD_WTOF,
    CMD_NTGA,
    CMD_NTOF,
    CMD_RTGA,
    CMD_RTOF,
    CMD_T,
    CMD_RT,
    CMD_ST,
    CMD_I,
    CMD_NT,
    CMD_WT,
    CMD_B,
    CMD_TN,
    CMD_NI,
    CMD_NR,
    CMD_DHCP,
    CMD_IP,
    CMD_NY,
    CMD_GW,
    CMD_MA,
    CMD_MAC,
    CMD_PO,
    CMD_RI,
    CMD_RQ,
    CMD_RO,
    CMD_RN,
    CMD_AP,
    CMD_NWA,
    CMD_NWB,
    CMD_NB,
    CMD_V,
    CMD_BS,
    CMD_BC,
    CMD_BF,
    CMD_RA,
    CMD_TON,
    CMD_TOF,
    CMD_ION,
    CMD_IOF,
    CMD_DR,
    CMD_AON,
    CMD_AOF,
    CMD_EM,
    CMD_EV,
    CMD_BN,
    CMD_BL,
    CMD_GN,
    CMD_VI,
    CMD_VM,
    CMD_VF,
    CMD_VT,
    CMD_VN,
    CMD_AX,
    CMD_AY,
    CMD_RC,
    CMD_AN,
    CMD_C,
    CMD_F,
    CMD_AA,
    CMD_P,
    CMD_FM,
    CMD_CT,
    CMD_KP,
    CMD_KI,
    CMD_KD,
    CMD_CNT,
} cmd_id_e;

typedef enum {
    P = 1 << 0, // 轮询属性
    B = 1 << 1, // 突发属性
    S = 1 << 2, // 设置属性
    N = 1 << 3, // 通知属性
    M = 1 << 4, // 保存属性
    I = 1 << 5, // 内部属性，1表示内部使用，0表示公开
    L = 1 << 6, // lock，需要解锁才能执行
} prop_e;

typedef enum {
    CMD_FLK = 0,
    CMD_AE,
} cmd_type_e;

/* UPP操作模式 */
#define UPP_ADDR_MODE_SINGLE 0 /* 单点指令 */
#define UPP_ADDR_MODE_MULTI  1 /* 多点指令 */

#define PARSE_DATA_MAX_LEN   1024
#define ADDR_LEN             3
#define BROADCAST_ADDR       99

/* 限制结构体定义 */
typedef struct {
    limit_type_t type; /* 限制类型 */

    union {
        float max_len; /* 最大长度限制，适用于字符串 */
        struct {
            float low_th; /* 范围限制的最小值 */
            float up_th;  /* 范围限制的最大值 */
        } range;          /* 范围限制 */

        struct {
            int   count;  /* 数组包含的元素数量 */
            void *values; /* 指向特定值数组的指针 */
        } special;        /* 特定值限制 */
    } value;              /* 限制值信息 */
} limit_t;

typedef bool (*str_chk_f)(const char *);

typedef struct {

    fmt_type_e type;
    int        min_len;
    int        max_len;
    union {
        struct {
            uint32_t min;
            uint32_t max;
        } u_range;
        struct {
            long min;
            long max;
        } ll_range;
        struct {
            float min;
            float max;
        } f_range;
        struct {
            double min;
            double max;
        } lf_range;
        struct {
            const char str[10]; // 假设字符种类不超过10个
            int        count;
        } c_range;
        struct {
            str_chk_f str_chk; // 函数指针，用于字符串校验
        } s_range;
    } data;
} chk_rule_t;

typedef struct {
    uint8_t group; // 参数所属的参数组
    uint8_t props; // 属性信息
} att_t;

/* UPP命令表元素定义 */
typedef struct {
    char      *cmd;    /* 命令 */
    cmd_id_e   cmd_id; /* id */
    att_t      att;    /* 属性 */
    chk_rule_t rule;   /* 校验规则 */
    void      *arg;    /* 参数 */
} cmd_item_t;

/* UPP解析结构体 */
typedef struct {
    uint16_t   cmd_len;                       /* 单包长度 */
    uint8_t    addr_mode;                     /* 地址模式 */
    uint8_t    type;                          /* 命令对应的类型 */
    uint8_t    save;                          /* 设置命令是否保存，0不保存，1保存 */
    uint8_t    ack;                           /* 设置命令是否应答，0不应答，1应答 */
    char      *ptr;                           /* 当前处理的位置*/
    uint8_t    cmd_word_len;                  /* 命令码字符数 */
    uint8_t    delim_len;                     /* 命令码字符数 */
    uint8_t    mode;                          /* 操作模式 */
    uint16_t   att;                           /* 命令属性 */
    uint32_t   multi_addr;                    /* 多点指令地址 */
    char       op;                            /* 操作符 */
    uint16_t   cmd_idx;                       /* 位于命令表的序号 */
    char       cmd_word[5];                   /* 命令码 */
    uint8_t    data_out_en;                   /* 输出data有效 */
    uint16_t   data_len;                      /* 收有效载荷长度 */
    char       data[PARSE_DATA_MAX_LEN + 20]; /* 收有效载荷 */
    uint8_t    err;                           /* 指令错误码 */
    cmd_type_e cmd_type;                      /* 命令类型 */
} parse_t;

/* 定义 para_limit 宏，根据 type 选择适当的初始化 */
#define LIM_NONE()                    {.type = LIM_TYPE_NONE, .value = {0}}
#define LIM_MAX_LEN(max_length_value) {.type = LIM_TYPE_MAX_LEN, .value.max_len = (max_length_value)}
#define LIM_RANGE(low_threshold, up_threshold)                                                                                                       \
    {                                                                                                                                                \
        .type = LIM_TYPE_RANGE, .value.range = { low_threshold, up_threshold }                                                                       \
    }
#define LIM_SPEC(special_cnt, special_values)                                                                                                        \
    {                                                                                                                                                \
        .type = LIM_TYPE_SPECIAL_VAL, .value.special = {.count = (special_cnt), .values = (void *)(special_values) }                                 \
    }
#define LIM_COMMA_SEP_NUM() {.type = LIM_TYPE_COMMA_SEP_NUM, .value = {0}}
#define LIM_COMMA_SEP_STR() {.type = LIM_TYPE_COMMA_SEP_STR, .value = {0}}

/* 支持UPP错误码输出 */
#define SUPPORT_UPP_ERR     1

/*代码内部温度表示单位*/
#define TEMP_UNIT_INTERNAL  'C'
/* 错误信息结构体 */
typedef struct {
    char        num;
    const char *desc;
} err_info_t;

/* 错误类型枚举 */
typedef enum {
    UPP_OK = 0,               /* 协议正确 */
                              /* 即时错误无需记录日志，直接返回到串口 */
    UPP_ERR_LEN,              /* 长度错误 */
    UPP_ERR_NO_CARRIAGE,      /* 命令无回车\r错误 */
    UPP_ERR_OP_TYPE,          /* 命令属性错误 */
    UPP_ERR_NO_OPERATOR,      /* 无操作符错误 */
    UPP_ERR_ADDR_ILLEGAL,     /* 地址不全是数字错误 */
    UPP_ERR_ADDR_OUT_RANGE,   /* 地址超限错误 */
    UPP_ERR_MODE,             /* 使用错误操作模式 */
    UPP_ERR_NO_SUPPORT_CMD,   /* 暂不支持此命令 */
    UPP_ERR_NO_SET_DATA,      /* 设置命令无数据 */
    UPP_ERR_NO_UPPERCASE,     /* 命令不全是大写字母 */
    UPP_ERR_NO_SUP_BROADCAST, /* 轮询命令不支持多点广播命令 */
    UPP_ERR_DATA_ILLEGAL,     /* 有效载荷数据格式不合法错误 */
    UPP_ERR_DATA_LEN,         /* 有效载荷长度错误 */
    UPP_ERR_DATA_RANGE,       /* 有效载荷值域错误 */
    UPP_ERR_MULT_OPERATOR,    /* 多操作符错误 */
    UPP_ERR_BUILD_PACKET,     /* 构建包错误 */
    UPP_ERR_DATA_LIMIT,       /* 参数限制错误 */
    UPP_ERR_DATA_CRC,         /* 参数校验错误 */
    UPP_ERR_CMD_LEN,          /* 命令字长度错误 */
    UPP_ERR_CMD_CONFLICT,     /* 执行冲突 */
    UPP_ERR_CMD_LOCKED,       /* 命令被锁定，无法执行 */
    UPP_ERR_OPERATOR_ILLEGAL, /* 非法操作符 */
    /* 此处以下错误需要记录到日志中 */
} error_type_e;

/* 数据类型枚举 */
typedef enum {
    TYPE_UINT8,
    TYPE_UINT32,
    TYPE_FLOAT,
} data_type_e;

extern const int            FORMAT_TAB_CNT; /* 计数-格式表 */
extern const format_entry_t format_table[];
extern const cmd_item_t     CMD_TBL[];
extern const err_info_t     ERR_INFO_TBL[];

int  upp_set_callback(parse_t *parse, cmd_id_e cmd_id);
int  upp_get_callback(parse_t *parse, cmd_id_e cmd_id);
void str_extract_data(parse_t *pkt_parse, void *buff, int type);

#endif /* _PROTO_CHAR_CFG_H_ */
