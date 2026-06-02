/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：sys_cfg.h
 * 文件描述：系统参数配置接口代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/12/02 李兆越 初始版本
 */

#ifndef _SYS_CFG_H_
#define _SYS_CFG_H_

#include "platform.h"

/* 设备启动状态 */
#define DEV_STA_COOL_START 0 /* 冷启动 */
#define DEV_STA_WARM_START 1 /* 热启动 */
#define DEV_STA_CNT        2

/* 测试引脚配置 */
#define TEST_CLOCK         RCU_GPIOA
#define TEST_PORT          GPIOA
#define TEST_PIN           GPIO_PIN_11
#define test_pin_init()                                                                                                                              \
    {                                                                                                                                                \
        rcu_periph_clock_enable(TEST_CLOCK);                                                                                                         \
        gpio_mode_set(TEST_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, TEST_PIN);                                                                        \
        gpio_output_options_set(TEST_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, TEST_PIN);                                                               \
    }

#define test_pin_on()     gpio_bit_set(TEST_PORT, TEST_PIN)
#define test_pin_off()    gpio_bit_reset(TEST_PORT, TEST_PIN)
#define test_pin_toggle() gpio_bit_toggle(TEST_PORT, TEST_PIN)
#define AAA               test_pin_on();
#define BBB               test_pin_off();

/* 设备自检错误枚举 */
typedef enum {
    DEV_ERR_EEPROM = 0,   /* EEPROM（AT24Cxx）故障，开机检测 1*/
    DEV_ERR_RTC,          /* RTC故障，开机检测 */
    DEV_ERR_ANA_OUTRANGE, /* 模拟输出超范围，运行检测 1*/
    DEV_ERR_RECORD,       /* 日志单元索引加载失败，开机检测 */
    DEV_ERR_PSRAM,        /* PSRAM(VTI7064x)故障，开机检测 1*/
    DEV_ERR_CAMERA,       /* CAMERA故障，运行检测 1 原先为DEV_ERR_OV5640*/
    DEV_ERR_AD7124,       /* AD7124故障，开机检测 1*/
    DEV_ERR_LTC2606,      /* LTC2606 故障，运行检测 1*/
    DEV_ERR_TM1639,       /* TM1639 故障，运行检测 */
    DEV_ERR_CNT
} dev_err_t;

typedef struct {
    uint8_t valid; // 标识有系统命令待处理
    uint8_t reset;
    uint8_t recover; // 1：恢复出厂设置 11：写入工厂参数 12：恢复默认参数
} sys_op_t;

/* 系统相关参数定义(全局，不保存） */
typedef struct {
    /* 系统相关 */
    float    long_cycles; /* perf_counter当前记录时间 */
    uint8_t  unlock;
    uint8_t  develop;
    int      time_sync;                         /* 时间是否已与服务器同步？ */
    uint32_t sys_sec;                           /* 系统时间（从1970年1月1日00:00:00起的秒数）UNIX时间戳 */
    int      run_sec;                           /* 运行时间（从上电开始的秒数） */
    int      dev_state;                         /* 设备状态信息 */
    uint32_t state_sec[DEV_STA_CNT];            /* 设备冷热启动时间 */
    int      dev_desc[32];                      /* 设备信息 */
    int      init_state;                        /* =0出厂参数, =1非出厂参数 */
    uint32_t para_err_status[STORAGE_TYPE_CNT]; /* 参数错误状态 最高位置1表示当前参数组经过校验，0表示没走校验流程*/

    /* 网络相关 */
    uint32_t net_connected;         /* 网络连接标志 */
    int      camera_flag_open_send; /* 图像发送标志 */
    int      camera_client_num;     /* 图像发送标志 */

    char    macAddr[12];  /* 字符串_MAC地址 */
    uint8_t remote_ip[4]; /* 远端ip地址 */
    uint8_t ip_addr[4];   /* 本地ip地址 */
    uint8_t net_mask[4];  /* 子网掩码 */
    uint8_t gateway[4];   /* 网关 */

    /* 算法相关 */
    uint8_t algo_str[1024];
    uint8_t algo_network_str_w1[1024];
    uint8_t algo_network_str_w2[1024];
    uint8_t algo_network_str_b[1024];

    /* 日志相关 */
    uint32_t index_clear;        /* 索引清除 */
    int      record_open;        /* 日志打开标志，默认1打开 */
    int      record_max_cnt[20]; /* 黑盒、状态日志最大存储数量 */

    /* 跨阻切档相关 */
    char     tiag_switch_mode;           /* 跨阻切换模式 */
    uint32_t now_gain[SENS_CH_SPEC_CNT]; /* 跨阻当前档位，[0]窄带，[1]宽带 */

    /* 测温相关 */
    float temp_target_before_limit_set; /* 限幅前的目标温度设置T= */
    float temp_target_before_limit;     /* 限幅前的目标温度查询?T */

    int temp_out_range[3]; /* 测量温度越界标志位 -1低于下限，0正常，1高于上限*/

    float    target_temp;                    /* 目标比色温度，单位：摄氏度，模拟输出和显示使用 */
    float    ratio_temp;                     /* 目标比色温度，单位：摄氏度，模拟输出和显示使用 */
    float    ratio_temp_nerve;               /* 目标比色温度，单位：摄氏度，模拟输出和显示使用 */
    float    single_temp[SENS_CH_SPEC_CNT];  /* 目标单色温度，单位：摄氏度  */
    float    Measured_attenuation;           /* 衰减率，单位：%  */
    float    internal_temp[SENS_CH_NTC_CNT]; /* 目标比色温度，单位：摄氏度*/
    int      target_volt[SENS_CH_SPEC_CNT];  /* 目标电压，单位：微伏  */
    uint32_t output_volt[SENS_CH_SPEC_CNT];  /* 目标电压，单位：微伏  */
    float    temp_calc;                      /* 查看计算后的未处理温度值 */
    int      temp_state;                     /* 0: 正常, 1: 超过上限, -1: 低于下限 */
    int      flag_discard;                   /* 切档丢弃标志 */
    int      volt_discard_num;               /* 电压丢弃次数 */
    float    temp_calibration;               /* 温度校准 */
    float    temp_calib_low_set;             /* 温度参考低设定值 */
    float    temp_calib_low_real;            /* 温度参考低实际值 */
    float    temp_calib_high_set;            /* 温度参考高设定值 */
    float    temp_calib_high_real;           /* 温度参考高实际值 */
    float    calib_low_set[3];               /* 低温参考设定值 [窄带, 宽带, 比色] */
    float    calib_low_real[3];              /* 低温实际测量值 [窄带, 宽带, 比色] */
    float    calib_high_set[3];              /* 高温参考设定值 [窄带, 宽带, 比色] */
    float    calib_high_real[3];             /* 高温实际测量值 [窄带, 宽带, 比色] */
    uint8_t  two_calib_open;                 /* 温度两点校准使能 */
    /* video */
    char video_relative_reticle_diameter[12]; /* 视频相对十字线直径 */

    /* 模拟输出 */
    uint32_t ana_curt_value;         /* 模拟电流输出设置，默认60000000自动控制输出，否则表示手动控制输出实际电流值*/
    uint32_t ana_volt_value;         /* 模拟电压输出设置，默认60000000自动控制输出，否则表示手动控制输出实际电压值*/
    uint8_t  ana_calib_flag;         /* 模拟输出校准后测试标志，0：运行状态，1：测试状态 */
    uint32_t ana_calib_value;        /* 模拟输出校准后测试电压/电流值 */
    uint8_t  ana_temp_convert_flag;  /* 模拟输出给定温度值测试标志，0：运行状态，1：测试状态*/
    float    ana_temp_convert_value; /* 给定温度值，测试输出电流/电压（校准后） */
                                     /* 突发参数 */
    char burst_fmt[30];              /* 上报数据条目内容 */

    /* UI相关 */
    char panel_lock; /* 面板锁定 */

    /* 设备相关 */
    char     dev_sn[DEV_ID_SIZE];     /* 设备SN码 */
    char     dev_info[DEV_INFO_SIZE]; /* 存储 ?XU 返回的设备型号/配置信息 */
    char     dev_error[DEV_ERR_CNT];  /* 设备自检错误信息 */
    sys_op_t sys_op;
} sys_state_t;

extern sys_state_t sys_state; /* 全局临时参数 */

#endif /* _SYS_CFG_H_ */
