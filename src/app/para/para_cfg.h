/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：para_cfg.h
 * 文件描述：系统参数配置接口代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈  军 初始版本
 *           V1.2 2024/04/10 李兆越 增加参数备份逻辑
 *           V1.3 2024/08/15 段政宏 梳理参数保存机制，简化参数操作逻辑
 *           V1.4 2025/02/10 李兆越 修改为固定大小的结构体，避免新增参数导致大小变化
 */

#ifndef _PARA_CFG_H_
#define _PARA_CFG_H_

#include "heater.h"
#include "proto_char.h"
#include "system.h"
#include "proto_coef.h"
#include "burst.h"
#include "ad7124.h"
#include "lib_cfg.h"

/* C99 兼容的静态断言宏 */
#define STATIC_ASSERT(CONDITION, MSG) typedef char static_assertion_failed_##MSG[(CONDITION) ? 1 : -1]

/* 参数属性表定义 */
typedef struct {
    char          *desc;   /* 参数描述 */
    const uint32_t offset; /* 参数组偏移地址 */
    const uint32_t size;   /* 参数组长度 */
} para_item_t;

/* 参数类型定义 */
typedef enum {
    PARA_DEV,    /* 0_device */
    PARA_PORT,   /* 1_port */
    PARA_GAIN,   /* 2_gain */
    PARA_ANA,    /* 3_analogout */
    PARA_ADC,    /* 4_adc */
    PARA_TEMP,   /* 5_temperature */
    PARA_NET,    /* 6_network */
    PARA_RECORD, /* 7_record */
    PARA_ALGO,   /* 8_algorithm */
    PARA_BURST,  /* 9_burst */
    PARA_ALARM,  /* 10_alarm */
    PARA_VIDEO,  /* 11_video */
    PARA_FILTER, /* 12_filter */
    PARA_HEATER, /* 13_heater */

    PARA_TYPE_CNT, /* 参数组总数（必须 <= 30） */
} para_type_t;

// 静态断言：确保组数不超过30（留给bit31做magic标志）
// 将 _Static_assert 替换为我们自定义的宏
STATIC_ASSERT(PARA_TYPE_CNT <= 30, PARA_TYPE_CNT_must_be_less_than_or_equal_to_30);

/* 0_device */
typedef hq_packed struct {
    uint8_t addr;                    /* 设备地址（1~DEV_MAX_ADDR） */
    char    name[DEV_NAME_SIZE + 1]; /* 设备名字 */
    char    sn[DEV_ID_SIZE + 1];     /* 设备SN码 */
    char    factory_set;             /*  工厂参数设置标志 */
    char    panel_lock;              /*  面板锁 */
} dev_para_t;

/* 1_port */
typedef hq_packed struct {
    char     mode;          /* 全双工/半双工 */
    char     check_fmt[4];  /* 协议通信校验方式 */
    uint32_t baudrate;      /* 协议通信波特率 */
    char     dbg_level[32]; /* DEBUG等级, todo待调整 */
    uint8_t  dbg_port;
} port_para_t;

/* 2_gain */
typedef hq_packed struct {
    uint32_t upper[SENS_CH_SPEC_CNT];   /* 跨阻切换电压上限 */
    uint32_t lowerml[SENS_CH_SPEC_CNT]; /* 跨阻切换电压下限M->L */
    uint32_t lowerhm[SENS_CH_SPEC_CNT]; /* 跨阻切换电压下限H->M */
    double   k_lm[SENS_CH_SPEC_CNT];    /* L档/M档所需系数 */
    double   k_mh[SENS_CH_SPEC_CNT];    /* M档/H档所需系数 */
    uint32_t cnt_thod;                  /* 切挡阈值检测区间的检测个数 */
    uint32_t volt_discard;              /* 切档后扔掉电压个数 */
    uint32_t gl_to_gm;                  /* 切挡步进上切次数 */
    uint32_t gm_to_gh;                  /* 切挡步进上切次数 */
    uint32_t gh_to_gm;                  /* 切挡步进下切次数 */
    uint32_t gm_to_gl;                  /* 切挡步进下切次数 */
} gain_para_t;

/* 3_analogout */
typedef hq_packed struct {
    char  type;  /* 模拟输出类型 ，0-20mA或4-20mA */
    float upper; /* 模拟输出温度上限（子温度），存储单位：摄氏度（小数点后1位）*/
    float lower; /* 模拟输出温度下限（子温度），存储单位：摄氏度（小数点后1位）*/

    float exceed_min_range; /* 模拟输出低于最小测温温度输出*/
    float exceed_max_range; /* 模拟输出高于最大测温温度输出*/

    float    lower_alarm_out; /* 低于下限报警输出值，存储单位：纳安 */
    float    upper_alarm_out; /* 高于上限报警输出值，存储单位：纳安 */
    uint32_t real_curt_2ma;   /* 理论给2mA实际输出的电流，测试得出，存储单位：纳安 */
    uint32_t real_curt_20ma;  /* 理论给20mA实际输出的电流，测试得出，存储单位：纳安 */
    uint32_t real_volt_1v;    /* 理论给1V实际输出的电压，测试得出，存储单位：微伏 */
    uint32_t real_volt_10v;   /* 理论给10V实际输出的电压，测试得出，存储单位：微伏 */
    uint16_t expos_time;      /* 响应时间（模拟输出速率），存储单位：毫秒 */
} ana_para_t;

/* 4_adc */

/* 5_temptature */
typedef hq_packed struct {
    char     unit;                             /* 温度单位（摄氏度/华氏度） */
    uint32_t volt_filter_len;                  /* 电压滤波长度 */
    uint32_t volt_change_range;                /* 同档位连续两次电压差值范围（毫伏） */
    float    emissivity;                       /* 发射率 */
    float    trans[SENS_CH_SPEC_CNT];          /* 透射率 */
    float    trans_interior[SENS_CH_SPEC_CNT]; /* 透射率（不对外开放） */
    float    dev_temp_min;                     /* 设备温度量程下限值，单位：依据系统参数单位设置 */
    float    dev_temp_max;                     /* 设备温度量程上限值 ，单位：依据系统参数单位设置 */
    double   temp_seg_volt_value;              /* 切换高低温算法系数的分界值 */
    float    volt_zero_l[SENS_CH_SPEC_CNT];    /* 零点电压偏置L */
    float    volt_zero_m;                      /* 零点电压偏置M */
    float    volt_zero_h;                      /* 零点电压偏置H */
    int      hyster_lower;                     /* 温度下限滞后值 */
    int      hyster_upper;                     /* 温度上限滞后值 */
    uint32_t hyster_upload;                    /* 切档后温度滞后上报时间 */
    float    temp_change_range;                /* [存储]同挡位连续两次电压差值范围 */
    uint32_t text_volt_for_sf;                 /* 电压-算法测试值 */
    uint32_t core_text;                        /* 算法测试值 */
    uint32_t temp_mode;                        /* 温度模式:0窄带，1宽带，2比色 */
    float    slope;                            /* 坡度 */
    float    slope_interior;                   /* 坡度（不对外开放） */
    uint32_t valid_vol;                        /* 有效电压阈值 */
    uint8_t  ambient_comp_ctl;                 /* 环温补偿使能 */
    float    temp_calib_gain[3];               /* 温度校准增益 */
    float    temp_calib_offset[3];             /* 温度校准偏置 */
} temp_para_t;

/* 6_network */
typedef hq_packed struct {
    uint32_t interval; /* 网络连接间隔，默认3s，单位为秒 */
    uint32_t rev_time; /* 接收超时时间，默认5s，单位为秒 */
    uint32_t dhcp;     /* 启用DHCP */

    uint32_t port;          /* 网络端口号 */
    char     ip_addr[16];   /* 字符串_本地IP地址 */
    char     remote_ip[16]; /* 字符串_远端IP地址 */
    char     net_mask[16];  /* 字符串_子网掩码 */
    char     gateway[16];   /* 字符串_网关地址 */
} net_para_t;

/* 7_record */
typedef hq_packed struct {
    uint32_t interval; /* 黑盒日志保存间隔，默认10，,单位s */
} record_para_t;

/* 9_burst */
typedef hq_packed struct {
    uint16_t speed;                     /* 突发速度（单位：毫秒） */
    char     fmt[3];                    /* 突发格式，第二个是key，value的分隔符， 第一个是命令分隔符， */
    char     cmd_list[BURST_LIST_SIZE]; /* 突发字符串格式 */
    char     mode;                      /* 轮询或突发模式 */
} burst_para_t;

/* 10_alarm */
typedef hq_packed struct {
    uint32_t relay_alarm_mode;        /* 继电器报警模式 */
    float    target_temp_alarm_on;    /* 目标温度报警触发阈值（升温超出此值报警） */
    float    target_temp_alarm_off;   /* 目标温度报警恢复阈值（降温低于此值恢复） */
    float    internal_temp_alarm_on;  /* 内部温度报警触发阈值（升温超出此值报警） */
    float    internal_temp_alarm_off; /* 内部温度报警恢复阈值（降温低于此值恢复） */
    uint32_t decay_rate;              /* 衰减率（兼容老逻辑） */
    uint32_t decay_fail_safe;         /* 衰减安全保护 */
    float    attenuation_alarm_off;   /* 衰减恢复阈值（大于此值恢复正常） */
    float    attenuation_alarm_on;    /* 衰减报警触发阈值（小于此值报警） */
} alarm_para_t;

/* 11_video */
typedef hq_packed struct {
    float    aiming_x;       /* 瞄准坐标X */
    float    aiming_y;       /* 瞄准坐标Y */
    uint8_t  exposure_mode;  /* EV曝光模式 */
    float    exposure_level; /* EV曝光等级 */
    float    brightness_val; /* 亮度调节 */
    uint8_t  backlight_val;
    uint8_t  gain_val;
    int      ctl_state;
    int      ctl_val;
    int      video_control;          /* 视频控制 */
    char     video_protocol_control; /* 视频协议控制 */
    uint8_t  video_send_fre;
    uint16_t test_delay;
    uint32_t send_size_test;
} video_para_t;

/* 12_filter */
typedef hq_packed struct {
    uint16_t temp_filter_buf_size;      /* 传感器温度滤波buf大小*/
    float    advance_hold_threshold;    /* 高级保持门限 */
    float    valley_holding_time;       /* 谷值保持时间 */
    float    advance_hold_average_time; /* 高级保持平均时间 */
    float    peak_holding_time;         /* 峰值保持时间 */
    uint8_t  mode;                      /* 滤波模式选择 */
} filter_para_t;

/* 13_heater */
typedef hq_packed struct {
    float  set_temp; /* 加热器目标温度 */
    double kp;       /* 加热器PID比例 */
    double ki;       /* 加热器PID积分 */
    double kd;       /* 加热器PID微分 */
} heater_para_t;

/* 系统参数表定义 */
typedef hq_packed struct {
    uint32_t      magic;  /* 魔数，确保参数有效性 */
    dev_para_t    dev;    /* 设备参数 */
    port_para_t   port;   /* 接口参数 */
    gain_para_t   gain;   /* 跨阻参数 */
    ana_para_t    ana;    /* 模拟输出参数 */
    ad7124_para_t adc;    /* AD7124参数 */
    temp_para_t   temp;   /* 测温参数 */
    net_para_t    net;    /* 网络参数 */
    record_para_t record; /* 日志参数 */
    algo_para_t   algo;   /* 温度转换算法参数 */
    burst_para_t  burst;  /* 突发 */
    alarm_para_t  alarm;  /* 报警 */
    video_para_t  video;  /* 视频瞄准 */
    filter_para_t filter; /* 滤波 */
    heater_para_t heater; /* 加热器 */
    uint16_t      crc[PARA_TYPE_CNT];
} sys_para_t;

/* 系统相关参数定义(全局，不保存） */
typedef ALIGN(4) struct {
    int   flag_temp_internal;
    float temp_internal_n;
    float temp_internal_w;

    float target_temp;
    float ratio_temp;
    float ratio_temp_nerve;

    double integral; // 误差累积值

    uint8_t nb_gain;
    uint8_t wb_gain;
} sys_quick_save_t;

/* 安全复位 */
void safe_reset(void);

#if SUPPORT_EEPROM
extern sys_para_t sys_para_bsp; /* 备份参数 */
#endif

extern sys_para_t        sys_para;        /* 系统参数 */
extern const sys_para_t  SYS_DEF_PARA;    /* 默认参数 */
extern const para_item_t PARA_INFO_TBL[]; /* 参数属性表 */
extern sys_quick_save_t  sys_quick_save;  /* Things that require quick saving */

int  powerup_uart_para_get(port_para_t *para);
void powerup_quick_save_get(void);
void hot_start_quick_save_get(void);
void hot_start_quick_save_set(void);

#endif /* _PARA_CFG_H_ */
