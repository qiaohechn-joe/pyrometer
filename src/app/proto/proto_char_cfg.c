/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：proto_char_cfg.c
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

#include "platform.h"
#include "serial.h"
#include "proto_char.h"
#include "proto_char_cfg.h"
#include "system.h"
#include "storage.h"
#include "ostimer.h"
#include "para.h"
#include "record.h"
#include "lwip.h"
#include "proto_coef.h"
#include "burst.h"
#include "measure.h"
#include "usbh_video.h"
#include "analogout.h"
#include "burst.h"

/* 由 comm_cfg.c 设置的当前命令来源组（uart_group/tcp_group） */
extern int g_cmd_src_group;

/* 温度单位定义表 */
const char TEMP_UNIT_TAB[] = {'C', 'F', 'K'};
#define TEMP_UNIT_CNT (sizeof(TEMP_UNIT_TAB) / sizeof(TEMP_UNIT_TAB[0]))
extern gain_sw_data_t gain_sw_data[SENS_CH_SPEC_CNT];
extern uint8_t        put_port;

/* UPP命令表 */
/* clang-format off */ 
const cmd_item_t CMD_TBL[] = {
    /*cmd, cmd_id, {group,       props},     {类型,    最短,        最长,          {data值校验}},                               arg} */
    /* dev */
    {"DA",  CMD_DA,  {PARA_DEV,  P|B|S|N|M}, {FMT_U_8,  1,           2,           {DEV_MIN_ADDR, DEV_MAX_ADDR}},               (void *)&sys_para.dev.addr         }, /* 设备地址 */
    {"ID",  CMD_ID,  {PARA_DEV,  P|S|N|M|L|I}, {FMT_S,    DEV_ID_SIZE, DEV_ID_SIZE, .data.s_range = {.str_chk = is_pure_digit}}, (void *)&sys_para.dev.sn           }, /* 设备SN码 */
    {"DN",  CMD_DN,  {PARA_DEV,  P|N|I},       {FMT_S,    NULL,        NULL,        NULL},                                       (void *)&sys_para.dev.name         }, /* 设备信息*/
    {"DV",  CMD_DV,  {PARA_DEV,  P|N|I},       {FMT_S,    NULL,        NULL,        NULL},                                       (void *)&sys_state.dev_info        }, /* 设备软件版本 */
    {"RS",  CMD_RS,  {PARA_DEV,  S|N|I},       {FMT_C,    1,           1,           .data.c_range = {.str = "1"}},               (void *)&sys_state.sys_op.reset    }, /* 设备软复位 */
    {"PR",  CMD_PR,  {PARA_DEV,  S|N|L|I},     {FMT_U_8,  1,           1,           {1,4}},                                      (void *)&sys_state.sys_op.recover  }, /* 系统参数恢复工厂设置 */
    {"SC",  CMD_SC,  {PARA_DEV,  P|N|I},       {FMT_S,    1,           2,           .data = NULL},                               (void *)&sys_state.dev_error       }, /* 设备自检信息 */
    {"UL",  CMD_UL,  {PARA_DEV,  P|S|N|I},     {FMT_U_8,  1,           1,           {0,1}},                                      (void *)&sys_state.unlock          }, /* 工厂参数恢复 factory reset */
    {"J",   CMD_J,   {PARA_DEV,  P|S|N|I|M},     {FMT_C,  1,           1,           .data.c_range = {.str = "LU"}},                 (void *)&sys_para.dev.panel_lock   }, /* panel lock*/
    {"DEV", CMD_DEV, {PARA_DEV,  P|S|N|I},     {FMT_U_8,  1,           1,           {0,1}},                                      (void *)&sys_state.develop         }, /* 开发者模式开关 */
    /* port */                               
    {"BR",  CMD_BR,  {PARA_PORT, P|S|N|M},   {FMT_U_32, 4,           6,           {2400, 230400}},                                  (void *)&sys_para.port.baudrate },
    {"CF",  CMD_CF,  {PARA_PORT, P|S|N|M|I},   {FMT_S,    1,           3,           .data.s_range = {.str_chk = is_check_fmt_valid}}, (void *)&sys_para.port.check_fmt},
    {"PM",  CMD_PM,  {PARA_PORT, P|S|N|M|I},   {FMT_C,    1,           1,           .data.c_range = {.str = "24",}},                  (void *)&sys_para.port.mode     },
    {"DB",  CMD_DB,  {PARA_PORT, P|S|N|M|I}, {FMT_C,    1,           1,           .data.c_range = {.str = "01234567"}},             (void *)&sys_para.port.dbg_level},
    {"DP",  CMD_DP,  {PARA_PORT, P|S|N|M|I}, {FMT_U_8,  1,           1,           {0,3}},                                           (void *)&sys_para.port.dbg_port},           /* 调试指令，选择调试输出端口RTT/USART */
    
    /* gain参数 */
    {"NM",  CMD_NM,  {PARA_GAIN, P|B|S|N|M|I}, {FMT_LF,    1,           10,          .data.lf_range = {1.0f, 1000.0f}},        (void *)&sys_para.gain.k_lm[SENS_CH_SPEC_NB]},     /* 窄带跨阻比值（低/中）*/
    {"NH",  CMD_NH,  {PARA_GAIN, P|B|S|N|M|I}, {FMT_LF,    1,           10,          .data.lf_range = {1.0f, 1000.0f}},        (void *)&sys_para.gain.k_mh[SENS_CH_SPEC_NB]},     /* 宽带跨阻比值（中/高）*/
    {"WM",  CMD_WM,  {PARA_GAIN, P|B|S|N|M|I}, {FMT_LF,    1,           10,          .data.lf_range = {1.0f, 1000.0f}},        (void *)&sys_para.gain.k_lm[SENS_CH_SPEC_WB]},     /* 突变跨阻比值（低/中）*/
    {"WH",  CMD_WH,  {PARA_GAIN, P|B|S|N|M|I}, {FMT_LF,    1,           10,          .data.lf_range = {1.0f, 1000.0f}},        (void *)&sys_para.gain.k_mh[SENS_CH_SPEC_WB]},     /* 宽带跨阻比值（中/高）*/
    {"NUP", CMD_NUP, {PARA_GAIN, P|B|S|N|M|I}, {FMT_U_32,  1,           7,           {1, 3300000}},                            (void *)&sys_para.gain.upper[SENS_CH_SPEC_NB]},    /* 跨阻切换电压上限（窄带）*/
    {"WUP", CMD_WUP, {PARA_GAIN, P|B|S|N|M|I}, {FMT_U_32,  1,           7,           {1, 3300000}},                            (void *)&sys_para.gain.upper[SENS_CH_SPEC_WB]},    /* 跨阻切换电压上限（宽带）*/
    {"NLM", CMD_NLM, {PARA_GAIN, P|B|S|N|M|I}, {FMT_U_32,  1,           7,           {1, 3300000}},                            (void *)&sys_para.gain.lowerml[SENS_CH_SPEC_NB]},  /* 跨阻切换电压下限M->L（窄带）*/
    {"WLM", CMD_WLM, {PARA_GAIN, P|B|S|N|M|I}, {FMT_U_32,  1,           7,           {1, 3300000}},                            (void *)&sys_para.gain.lowerml[SENS_CH_SPEC_WB]},  /* 跨阻切换电压下限M->L（宽带） */
    {"NLH", CMD_NLH, {PARA_GAIN, P|B|S|N|M|I}, {FMT_U_32,  1,           7,           {1, 3300000}},                            (void *)&sys_para.gain.lowerhm[SENS_CH_SPEC_NB]},  /* 跨阻切换电压下限H->M（窄带） */
    {"WLH", CMD_WLH, {PARA_GAIN, P|B|S|N|M|I}, {FMT_U_32,  1,           7,           {1, 3300000}},                            (void *)&sys_para.gain.lowerhm[SENS_CH_SPEC_WB]},  /* 跨阻切换电压下限H->M（宽带） */
    {"GDN", CMD_GDN, {PARA_GAIN, P|B|S|N|M|I}, {FMT_U_32,  1,           3,           {1, 500}},                                (void *)&sys_para.gain.volt_discard},              /* 切档丢弃电压个数 */
    {"GCN", CMD_GCN, {PARA_GAIN, P|B|S|N|M|I}, {FMT_U_32,  1,           3,           {1, 500}},                                (void *)&sys_para.gain.cnt_thod},                  /* 切挡阈值检测区间的检测个数 */
    
    /* analogout */
    {"AM", CMD_AM, {PARA_ANA, P|S|N|M},   {FMT_C,    1,          1,           .data.c_range = {.str = "04",}},             (void *)&sys_para.ana.type},                   /* 模拟输出模式 */
    {"AL", CMD_AL, {PARA_ANA, P|B|S|N|M}, {FMT_F,    3,          7,           .data.f_range = {TEMP_LOWER, TEMP_UPPER}},   (void *)&sys_para.ana.lower},                  /* 模拟输出温度下限 */
    {"AH", CMD_AH, {PARA_ANA, P|B|S|N|M}, {FMT_F,    3,          7,           .data.f_range = {TEMP_LOWER, TEMP_UPPER}},   (void *)&sys_para.ana.upper},                  /* 模拟输出温度上限 */
    {"OAL", CMD_OAL, {PARA_ANA, P|B|S|N|M}, {FMT_F,    1,          7,           .data.f_range = {0, 4}},   (void *)&sys_para.ana.exceed_min_range},                  /* 模拟输出超下限输出值 */
    {"OAH", CMD_OAH, {PARA_ANA, P|B|S|N|M}, {FMT_F,    1,          7,           .data.f_range = {20, 24}},   (void *)&sys_para.ana.exceed_max_range},                  /* 模拟输出超上限输出值 */
    {"OI", CMD_OI, {PARA_ANA, P|S|N|M},   {FMT_U_32, 1,          8,           {0, 60000000}},                              (void *)&sys_state.ana_curt_value},            /* 模拟电流输出控制 */
    {"IL", CMD_IL, {PARA_ANA, P|S|N|M|I},   {FMT_U_32, 6,          7,           {900000,  1500000}},                         (void *)&sys_para.ana.real_curt_2ma},          /* 电流输出校准下限值 */
    {"IH", CMD_IH, {PARA_ANA, P|S|N|M|I},   {FMT_U_32, 8,          8,           {18500000, 24000000}},                       (void *)&sys_para.ana.real_curt_20ma},         /* 电流输出校准上限值 */
    {"AT", CMD_AT, {PARA_ANA, P|S|N|I},     {FMT_F,    3,          6,           .data.f_range = {TEMP_LOWER, TEMP_UPPER}},          (void *)&sys_state.ana_temp_convert_value},    /* 模拟输出虚拟温度 */
    /* adc */
    {"DM",  CMD_DM,  {PARA_ADC, P|S|N|M|I},   {FMT_U_8,  1, 1, {0, 3}}, (void *)&sys_para.adc.ctrl_pwr_mode},               /* ADC_CTRL  */
    {"GA",  CMD_GA,  {PARA_ADC, P|S|N|M|I},   {FMT_U_32, 1, 1, {0, 0}}, (void *)&sys_para.adc.pga},                         /* 可编程增益:PGA1 */
    {"OFN", CMD_OFN, {PARA_ADC, P|S|N|M|I},   {FMT_U_32, 1, 8, {0, 10000000}}, (void *)&sys_para.adc.offs0},                /* 0通道偏置宽带 */
    {"OFW", CMD_OFW, {PARA_ADC, P|S|N|M|I},   {FMT_U_32, 1, 8, {0, 10000000}}, (void *)&sys_para.adc.offs2},                /* 2通道偏置窄带 */
    {"NV",  CMD_NV,  {PARA_ADC, P|B|N|I},     {FMT_U_32, 1, 1, {0, 0}}, (void *)&sys_state.target_volt[SENS_CH_SPEC_NB]},   /* 窄带原始电压 */
    {"WV",  CMD_WV,  {PARA_ADC, P|B|N|I},     {FMT_U_32, 1, 1, {0, 0}}, (void *)&sys_state.target_volt[SENS_CH_SPEC_WB]},   /* 宽带原始电压 */
    {"ONV", CMD_ONV, {PARA_ADC, P|B|N|I},     {FMT_U_32, 1, 1, {0, 0}}, (void *)&sys_state.output_volt[SENS_CH_SPEC_NB]},   /* 窄带补偿后电压 */
    {"OWV", CMD_OWV, {PARA_ADC, P|B|N|I},     {FMT_U_32, 1, 1, {0, 0}}, (void *)&sys_state.output_volt[SENS_CH_SPEC_WB]},   /* 宽带补偿后电压 */
    {"SN",  CMD_SN,  {PARA_ADC, P|B|S|N|M|I}, {FMT_U_32, 1, 1, {0, 2}}, (void *)&sys_state.now_gain[SENS_CH_SPEC_NB]},      /* 窄带挡位 */
    {"SW",  CMD_SW,  {PARA_ADC, P|B|S|N|M|I}, {FMT_U_32, 1, 1, {0, 2}}, (void *)&sys_state.now_gain[SENS_CH_SPEC_WB]},      /* 宽带挡位 */
    {"SM",  CMD_SM,  {PARA_ADC, P|B|S|N|M|I}, {FMT_U_32, 1, 1, {0, 1}}, (void *)&sys_state.tiag_switch_mode},               /* 挡位模式（手动/自动） */
    
    /* temperature（输入） */
    {"U",    CMD_U,    {PARA_TEMP, P|B|S|N|M}, {FMT_C,   1, 1, .data.c_range = {.str = "CFK"}},          (void *)&sys_para.temp.unit},                              /* 温度单位 */
    {"DL",   CMD_DL,   {PARA_TEMP, P|S|N|M|I},   {FMT_F,   3, 7, .data.f_range = {TEMP_LOWER, TEMP_UPPER}},(void *)&sys_para.temp.dev_temp_min},                      /* 输出温度下限 */
    {"DH",   CMD_DH,   {PARA_TEMP, P|S|N|M|I},   {FMT_F,   3, 7, .data.f_range = {TEMP_LOWER, TEMP_UPPER}},(void *)&sys_para.temp.dev_temp_max},                      /* 输出温度上限 */
    {"M",    CMD_M,    {PARA_TEMP, P|B|S|N|M}, {FMT_U_8, 1, 1, {0, 2}},                                  (void *)&sys_para.temp.temp_mode},                         /* 测温模式 */
    {"NC",   CMD_NC,   {PARA_TEMP, P|B|S|N|M|I}, {FMT_F6,  1, 6, .data.f_range = {0.100f, 1.150f}},        (void *)&sys_para.temp.trans_interior[SENS_CH_SPEC_NB]},   /* 窄带透射率内部 */
    {"WC",   CMD_WC,   {PARA_TEMP, P|B|S|N|M|I}, {FMT_F6,  1, 6, .data.f_range = {0.100f, 1.150f}},        (void *)&sys_para.temp.trans_interior[SENS_CH_SPEC_WB]},   /* 宽带透射率内部 */
    {"E",    CMD_E,    {PARA_TEMP, P|B|S|N|M}, {FMT_F6,  1, 6, .data.f_range = {0.100f, 1.150f}},        (void *)&sys_para.temp.emissivity},                        /* 窄带透射率 */
    {"NS",   CMD_NS,   {PARA_TEMP, P|B|S|N|M}, {FMT_F6,  1, 6, .data.f_range = {0.100f, 10.150f}},       (void *)&sys_para.temp.trans[SENS_CH_SPEC_NB]},            /* 窄带透射率 */
    {"WS",   CMD_WS,   {PARA_TEMP, P|B|S|N|M}, {FMT_F6,  1, 6, .data.f_range = {0.100f, 10.150f}},       (void *)&sys_para.temp.trans[SENS_CH_SPEC_WB]},            /* 宽带发射率 */
    {"S",    CMD_S,    {PARA_TEMP, P|B|S|N|M}, {FMT_F6,  1, 6, .data.f_range = {0.100f, 10.150f}},        (void *)&sys_para.temp.slope},                             /* 比色坡度 */
    {"SI",   CMD_SI,   {PARA_TEMP, P|B|S|N|M}, {FMT_F6,  1, 6, .data.f_range = {0.100f, 10.150f}},        (void *)&sys_para.temp.slope_interior},                    /* 比色坡度 （不对外开放）*/
    {"ON",   CMD_ON,   {PARA_TEMP, P|B|S|N|M|I}, {FMT_U_32,1, 7, {0, 1000000}},                            (void *)&sys_para.temp.volt_zero_l[SENS_CH_SPEC_NB]},      /* 窄带暗电压偏置uV */
    {"OW",   CMD_OW,   {PARA_TEMP, P|B|S|N|M|I}, {FMT_U_32,1, 7, {0, 1000000}},                            (void *)&sys_para.temp.volt_zero_l[SENS_CH_SPEC_WB]},      /* 宽带暗电压偏置uV */
    {"VV",   CMD_VV,   {PARA_TEMP, P|S|N|M|I},   {FMT_U_32,1, 7, {0, 3220000}},                            (void *)&sys_para.temp.valid_vol},                         /* 有效电压 */
    {"ATC",  CMD_ATC,  {PARA_TEMP, P|S|N|M|I},   {FMT_U_8, 1, 1, {0, 1}},                                  (void *)&sys_para.temp.ambient_comp_ctl},                 /* 环温补偿控制 */
    {"TTCO", CMD_TTCO, {PARA_TEMP, P|S|N|I},     {FMT_U_8, 1, 1, {0, 1}},                                  (void *)&sys_state.two_calib_open},                        /* 温度两点校准使能 */
    {"CA",   CMD_CA,   {PARA_TEMP, S|N|I},       {FMT_F,   3, 7, .data.f_range = {TEMP_LOWER, TEMP_UPPER}},(void *)&sys_state.temp_calibration},                      /* 温度校准 */
    {"TCLS", CMD_TCLS, {PARA_TEMP, S|N|I},       {FMT_F,   3, 7, .data.f_range = {TEMP_LOWER, TEMP_UPPER}},(void *)&sys_state.temp_calib_low_set},                    /* 温度参考设定低值 */
    {"TCLR", CMD_TCLR, {PARA_TEMP, S|N|I},       {FMT_F,   3, 7, .data.f_range = {TEMP_LOWER, TEMP_UPPER}},(void *)&sys_state.temp_calib_low_real},                   /* 温度参考实际低值 */
    {"TCHS", CMD_TCHS, {PARA_TEMP, S|N|I},       {FMT_F,   3, 7, .data.f_range = {TEMP_LOWER, TEMP_UPPER}},(void *)&sys_state.temp_calib_high_set},                   /* 温度参考设定高值 */
    {"TCHR", CMD_TCHR, {PARA_TEMP, S|N|I},       {FMT_F,   3, 7, .data.f_range = {TEMP_LOWER, TEMP_UPPER}},(void *)&sys_state.temp_calib_high_real},                  /* 温度参考实际高值 */
    {"WTGA", CMD_WTGA, {PARA_TEMP, P|B|S|N|M|I}, {FMT_F,   1, 8, .data.f_range = {0.800000f, 1.200000f}},  (void *)&sys_para.temp.temp_calib_gain[SENS_CH_SPEC_WB]},  /* 宽带温度校准增益 */
    {"WTOF", CMD_WTOF, {PARA_TEMP, P|B|S|N|M|I}, {FMT_F,   1, 6, .data.f_range = {-500.0f, 500.0f}},       (void *)&sys_para.temp.temp_calib_offset[SENS_CH_SPEC_WB]},/* 宽带温度校准偏置 */
    {"NTGA", CMD_NTGA, {PARA_TEMP, P|B|S|N|M|I}, {FMT_F,   1, 8, .data.f_range = {0.800000f, 1.200000f}},  (void *)&sys_para.temp.temp_calib_gain[SENS_CH_SPEC_NB]},  /* 窄带温度校准增益 */
    {"NTOF", CMD_NTOF, {PARA_TEMP, P|B|S|N|M|I}, {FMT_F,   1, 6, .data.f_range = {-500.0f, 500.0f}},       (void *)&sys_para.temp.temp_calib_offset[SENS_CH_SPEC_NB]},/* 窄带温度校准偏置 */
    {"RTGA", CMD_RTGA, {PARA_TEMP, P|B|S|N|M|I}, {FMT_F,   1, 8, .data.f_range = {0.800000f, 1.200000f}},  (void *)&sys_para.temp.temp_calib_gain[2]},                /* 比色温度校准增益 */
    {"RTOF", CMD_RTOF, {PARA_TEMP, P|B|S|N|M|I}, {FMT_F,   1, 6, .data.f_range = {-500.0f, 500.0f}},       (void *)&sys_para.temp.temp_calib_offset[2]},              /* 比色温度校准偏置 */
    
    /* temperature（输出） */
    {"T",  CMD_T,  {PARA_TEMP, P|B|N}, {FMT_F, 1, 1, .data.f_range = {0.1f, 10000.0f}}, (void *)&sys_state.target_temp},                               /* 目标温度 */
    {"RT", CMD_RT, {PARA_TEMP, P|B|N|I}, {FMT_F, 1, 1, .data.f_range = {0.0f, 0.0f}},     (void *)&sys_state.ratio_temp},                                /* 比色温度 */
    {"ST", CMD_ST, {PARA_TEMP, P|B|N|I}, {FMT_F, 1, 1, .data.f_range = {0.0f, 0.0f}},     (void *)&sys_state.internal_temp[SENS_CH_NTC_SPEC]},            /* 内部sensor温度 */
    {"I",  CMD_I,  {PARA_TEMP, P|B|N}, {FMT_F, 1, 1, .data.f_range = {0.0f, 0.0f}},     (void *)&sys_state.internal_temp[SENS_CH_NTC_TIA]},            /* 内部board温度 */
    {"NT", CMD_NT, {PARA_TEMP, P|B|N}, {FMT_F, 1, 1, .data.f_range = {0.1f, 10000.0f}}, (void *)&sys_state.single_temp[SENS_CH_SPEC_NB]},              /* 窄带单色目标温度 */
    {"WT", CMD_WT, {PARA_TEMP, P|B|N}, {FMT_F, 1, 1, .data.f_range = {0.1f, 10000.0f}}, (void *)&sys_state.single_temp[SENS_CH_SPEC_WB]},              /* 宽带单色目标温度 */
    {"B",  CMD_B,  {PARA_TEMP, P|B|N}, {FMT_F, 1, 1, .data.f_range = {0.0f, 0.0f}},     (void *)&sys_state.Measured_attenuation},                      /* 被测衰减率 */
    {"TN", CMD_TN, {PARA_TEMP, P|B|N|I}, {FMT_F, 1, 1, .data.f_range = {0.0f, 0.0f}},     (void *)&sys_state.ratio_temp_nerve},                          /* 比色神经网络算法温度 */

    /* network */
    {"NI",   CMD_NI,  {PARA_NET, P|S|N|M|I}, {FMT_U_32, 1, 2, {1, 10}},       (void *)&sys_para.net.interval},  /* 网络重连间隔 */
    {"NR",   CMD_NR,  {PARA_NET, P|S|N|M|I}, {FMT_U_32, 1, 2, {1, 50}},       (void *)&sys_para.net.rev_time},  /* 网络接收超时时间 */
    {"DHCP", CMD_DHCP,{PARA_NET, P|S|N|M|I}, {FMT_U_8,  1, 1, {0, 1}},        (void *)&sys_para.net.dhcp},      /* 使用DHCP */
    {"IP",   CMD_IP,  {PARA_NET, P|S|N|M},   {FMT_S,    7, 15, .data.s_range = {.str_chk = is_valid_ip}}, (void *)&sys_para.net.ip_addr},   /* 设备IP地址 */
    {"NY",   CMD_NY,  {PARA_NET, P|S|N|M|I}, {FMT_S,    7, 15, .data.s_range = {.str_chk = is_valid_ip}}, (void *)&sys_para.net.remote_ip}, /* 远端IP地址 */
    {"GW",   CMD_GW,  {PARA_NET, P|S|N|M},   {FMT_S,    7, 15, .data.s_range = {.str_chk = is_valid_ip}}, (void *)&sys_para.net.gateway},   /* 默认网关地址 */
    {"MA",   CMD_MA,  {PARA_NET, P|S|N|M},   {FMT_S,    7, 15, .data.s_range = {.str_chk = is_valid_ip}}, (void *)&sys_para.net.net_mask},  /* 子网掩码 */
    {"MAC",  CMD_MAC, {PARA_NET, P},         {FMT_S,    12, 12, .data = NULL}, (void *)&sys_state.macAddr},   /* 设备MAC地址 */
    {"PO",   CMD_PO,  {PARA_NET, P|S|N|M},   {FMT_U_16, 1, 5, {0, 65535}},    (void *)&sys_para.net.port},      /* 网络端口号 */
    
    /* record */
    {"RI", CMD_RI, {PARA_RECORD, P|S|N|M|I}, {FMT_U_32, 3, 8, {REC_INTVAL_MIN, REC_INTVAL_MAX}}, (void *)&sys_para.record.interval}, /* 黑盒日志存储间隔 interval */
    {"RQ", CMD_RQ, {PARA_RECORD, S|N|I},     {FMT_U_8,  1, 1, {0,                PWD_CNT}     }, (void *)&sys_state.index_clear},    /* 索引清除 clear 80295 */
    {"RO", CMD_RO, {PARA_RECORD, P|S|N|I},   {FMT_U_8,  1, 1, {0,                1}           }, (void *)&sys_state.record_open},    /* 所有日志打开记录 open 1 */
    {"RN", CMD_RN, {PARA_RECORD, P|N|I},     {FMT_S,    1, BURST_LIST_SIZE, .data = NULL      }, (void *)&sys_state.record_max_cnt}, /* 查询日志索引数 */
    
    /* algorithm */
    {"AP", CMD_AP, {PARA_ALGO,  P|S|N|M|I}, {FMT_S,    1, PARSE_DATA_MAX_LEN, .data = NULL    }, (void *)&sys_state.algo_str},       /* 算法系数 */
    {"NWA", CMD_NWA, {PARA_ALGO,  P|S|N|M|I}, {FMT_S,    1, PARSE_DATA_MAX_LEN, .data = NULL    }, (void *)&sys_state.algo_network_str_w1},       /* 神经网络算法权重设置 */
    {"NWB", CMD_NWB, {PARA_ALGO,  P|S|N|M|I}, {FMT_S,    1, PARSE_DATA_MAX_LEN, .data = NULL    }, (void *)&sys_state.algo_network_str_w2},       /* 神经网络算法权重设置 */
    {"NB", CMD_NB, {PARA_ALGO,  P|S|N|M|I}, {FMT_S,    1, PARSE_DATA_MAX_LEN, .data = NULL    }, (void *)&sys_state.algo_network_str_b},       /* 神经网络算法偏置设置 */
    
    /* burst */
    {"V",  CMD_V,  {PARA_BURST, P|S|N|M}, {FMT_C,    1, 1,               .data.c_range = {.str = "BP"}    },           (void *)&sys_para.burst.mode},        /* 打开上报 */
    {"BS", CMD_BS, {PARA_BURST, P|S|N|M}, {FMT_U_16, 2, 5,               {30, 10000}          },                       (void *)&sys_para.burst.speed},       /* 突发速度 */
    {"BC", CMD_BC, {PARA_BURST, P|S|N|M}, {FMT_S,    1, BURST_LIST_SIZE, .data.s_range = {.str_chk = valid_cmd_list}}, (void *)&sys_para.burst.cmd_list},    /* 上报类型 */
    {"BF", CMD_BF, {PARA_BURST, P|S|N|M}, {FMT_S,    2, 2,               .data = NULL       },                         (void *)&sys_para.burst.fmt},         /* 上传格式指令 */
    
    /* alarm */
    {"RA",  CMD_RA,  {PARA_ALARM, P|S|N|M}, {FMT_U_8,  1, 1, {0,  9}                          }, (void *)&sys_para.alarm.relay_alarm_mode},       /* 继电器报警模式 */
    {"TON", CMD_TON, {PARA_ALARM, P|S|N|M}, {FMT_F,    1, 6, .data.f_range = {TEMP_LOWER, TEMP_UPPER}}, (void *)&sys_para.alarm.target_temp_alarm_on},   /* 目标温度报警触发阈值 */ 
    {"TOF", CMD_TOF, {PARA_ALARM, P|S|N|M}, {FMT_F,    1, 5, .data.f_range = {0.0f, 150.0f}}, (void *)&sys_para.alarm.target_temp_alarm_off},  /* 目标温度报警恢复死区 */
    {"ION", CMD_ION, {PARA_ALARM, P|S|N|M|I}, {FMT_F,    1, 5, .data.f_range = {-40.0f, 150.0f} }, (void *)&sys_para.alarm.internal_temp_alarm_on}, /* 内部温度报警触发阈值 */
    {"IOF", CMD_IOF, {PARA_ALARM, P|S|N|M|I}, {FMT_F,    1, 5, .data.f_range = {0.0f, 150.0f} },   (void *)&sys_para.alarm.internal_temp_alarm_off},/* 内部温度报警恢复死区 */
    {"DR",  CMD_DR,  {PARA_ALARM, P|S|N|M|I}, {FMT_U_32, 1, 3, {0,  100}                        }, (void *)&sys_para.alarm.decay_rate},             /* 衰减报警阈值 */
    {"AON", CMD_AON, {PARA_ALARM, P|S|N|M}, {FMT_F,    1, 4, .data.f_range = {0.00f,  1.00f}  }, (void *)&sys_para.alarm.attenuation_alarm_on},   /* 衰减报警触发阈值 */
    {"AOF", CMD_AOF, {PARA_ALARM, P|S|N|M|I}, {FMT_F,    1, 4, .data.f_range = {0.00f,  1.00f}  }, (void *)&sys_para.alarm.attenuation_alarm_off},  /* 衰减报警恢复死区 */
    
    /* video */
    {"EM", CMD_EM, {PARA_VIDEO,  P|S|N|M|I}, {FMT_U_8, 1, 1, {1, 8}                             }, (void *)&sys_para.video.exposure_mode},                    /* EV曝光模式 */
    {"EV", CMD_EV, {PARA_VIDEO,  P|S|N|M|I}, {FMT_F,   2,5, .data.f_range = {-13.0f, -2.0f}     }, (void *)&sys_para.video.exposure_level},                   /* 曝光等级 */
    {"BN", CMD_BN, {PARA_VIDEO,  P|S|N|M|I}, {FMT_F,   1,5, .data.f_range = {- 64.0f, 64.0f}    }, (void *)&sys_para.video.brightness_val},                   /* 亮度等级 */
    {"BL", CMD_BL, {PARA_VIDEO,  P|S|N|M|I}, {FMT_U_8, 1,1,  {0, 5}                             }, (void *)&sys_para.video.backlight_val},                    /* 逆光对比度 */
    {"GN", CMD_GN, {PARA_VIDEO,  P|S|N|M|I}, {FMT_U_8, 1,1,  {1, 7}                             }, (void *)&sys_para.video.gain_val},                         /* 增益值 */
    {"VI", CMD_VI, {PARA_VIDEO,  P|S|N|M}, {FMT_U_8, 1, 1, {0, 1}                             }, (void *)&sys_para.video.video_control},                    /* 视频控制 */
    {"VM", CMD_VM, {PARA_VIDEO,  P|S|N|M|I}, {FMT_U_8, 1, 1, {0, 1}                             }, (void *)&sys_para.video.video_protocol_control},           /* 视频协议控制 */
    {"VF", CMD_VF, {PARA_VIDEO,  P|S|N|M|I}, {FMT_U_8, 1, 1, {0, 8}                             }, (void *)&sys_para.video.video_send_fre},           /* 视频协议控制 */
    {"VT", CMD_VT, {PARA_VIDEO,  P|S|N|M|I}, {FMT_U_16, 1, 10, {0, 1000}                             }, (void *)&sys_para.video.test_delay},           /* 视频协议控制 */    
    {"VN", CMD_VN, {PARA_VIDEO,  P|S|N|M|I}, {FMT_U_32, 1, 10, {0, 4000}                             }, (void *)&sys_para.video.send_size_test},           /* 视频协议控制 */    
    {"AX", CMD_AX, {PARA_VIDEO,  P|S|N|M|I}, {FMT_F,   1, 7, .data.f_range = {-1000.0f, 1000.0f}}, (void *)&sys_para.video.aiming_x},                         /* 视频瞄准坐标X轴 */
    {"AY", CMD_AY, {PARA_VIDEO,  P|S|N|M|I}, {FMT_F,   1, 7, .data.f_range = {-1000.0f, 1000.0f}}, (void *)&sys_para.video.aiming_y},                         /* 视频瞄准坐标Y轴 */
    {"RC", CMD_RC, {PARA_VIDEO,  P|N|I},     {FMT_F,   1, 4, .data.f_range = {0.00f,    1.00f}  }, (void *)&sys_state.video_relative_reticle_diameter},       /* 视频相对十字线直径 */
    
    /* filter */
    {"AN", CMD_AN, {PARA_FILTER, P|S|N|M},   {FMT_U_16, 1, 3, {1, FILTER_LEN_MAX}              },  (void *)&sys_para.filter.temp_filter_buf_size},     /* 传感器温度滤波buf大小 */
    {"C",  CMD_C,  {PARA_FILTER, P|S|N|M|I},   {FMT_F,    1, 1, .data.f_range = {0.0f, 0.0f}     },  (void *)&sys_para.filter.advance_hold_threshold},   /* 高级保持门限 */
    {"F",  CMD_F,  {PARA_FILTER, P|B|S|N|M|I}, {FMT_F,    1, 5, .data.f_range = {HOLDING_TIME_LOWER, HOLDING_TIME_UPPER}   },  (void *)&sys_para.filter.valley_holding_time},      /* 谷值保持时间 */
    {"AA", CMD_AA, {PARA_FILTER, P|S|N|M|I},   {FMT_F,    1, 1, .data.f_range = {0.0f, 0.0f}     },  (void *)&sys_para.filter.advance_hold_average_time},/* 高级保持平均时间 */
    {"P",  CMD_P,  {PARA_FILTER, P|B|S|N|M|I}, {FMT_F,    1, 5, .data.f_range = {HOLDING_TIME_LOWER, HOLDING_TIME_UPPER}   },  (void *)&sys_para.filter.peak_holding_time},        /* 峰值保持时间 */
    {"FM", CMD_FM,  {PARA_FILTER, P|S|N|M|I},  {FMT_U_8,  1, 1, {0, 2}                           },  (void *)&sys_para.filter.mode},        /* 滤波模式 */
    /* heater */
    {"CT", CMD_CT, {PARA_HEATER, P|S|N|M|I}, {FMT_F6, 1, 5,  .data.f_range = {0.0f,  100.0f} },  (void *)&sys_para.heater.set_temp},        /* 加热器温度设定 */
    {"KP", CMD_KP, {PARA_HEATER, P|S|N|M|I}, {FMT_LF, 1, 12, .data.lf_range = {0.0f, 50000.0f}}, (void *)&sys_para.heater.kp},               /* 加热器PID比例系数 */
    {"KI", CMD_KI, {PARA_HEATER, P|S|N|M|I}, {FMT_LF, 1, 12, .data.lf_range = {0.0f, 50000.0f}}, (void *)&sys_para.heater.ki},               /* 加热器PID积分系数 */
    {"KD", CMD_KD, {PARA_HEATER, P|S|N|M|I}, {FMT_LF, 1, 12, .data.lf_range = {0.0f, 50000.0f}}, (void *)&sys_para.heater.kd},               /* 加热器PID微分系数 */
};
/* clang-format on */

const err_info_t ERR_INFO_TBL[] = {
    /* 命令交互中的错误，直接返回到串口 */
    [UPP_OK]                   = {.desc = "upp ok"},
    [UPP_ERR_LEN]              = {.desc = "upp err len"},
    [UPP_ERR_NO_CARRIAGE]      = {.desc = "upp err no carriage"},
    [UPP_ERR_OP_TYPE]          = {.desc = "upp err op type"},
    [UPP_ERR_NO_OPERATOR]      = {.desc = "upp err no operator"},
    [UPP_ERR_ADDR_ILLEGAL]     = {.desc = "upp err addr illegal"},
    [UPP_ERR_ADDR_OUT_RANGE]   = {.desc = "upp addr over range"},
    [UPP_ERR_MODE]             = {.desc = "upp err mode"},
    [UPP_ERR_NO_SUPPORT_CMD]   = {.desc = "upp err no support cmd"},
    [UPP_ERR_NO_SET_DATA]      = {.desc = "upp err no set data"},
    [UPP_ERR_NO_UPPERCASE]     = {.desc = "upp err no uppercase"},
    [UPP_ERR_NO_SUP_BROADCAST] = {.desc = "upp err no sup broadcast"},
    [UPP_ERR_DATA_ILLEGAL]     = {.desc = "upp err data illegal"},
    [UPP_ERR_DATA_LEN]         = {.desc = "upp err data len"},
    [UPP_ERR_DATA_RANGE]       = {.desc = "upp err data range"},
    [UPP_ERR_MULT_OPERATOR]    = {.desc = "upp err mult operator"},
    [UPP_ERR_BUILD_PACKET]     = {.desc = "upp err build packet"},
    [UPP_ERR_DATA_LIMIT]       = {.desc = "upp err data limit"},
    [UPP_ERR_DATA_CRC]         = {.desc = "upp err data crc/calibration"},
    [UPP_ERR_CMD_LEN]          = {.desc = "upp err cmd length"},
    [UPP_ERR_CMD_CONFLICT]     = {.desc = "upp err cmd conflict"},
    [UPP_ERR_CMD_LOCKED]       = {.desc = "upp err device is locked"},
    [UPP_ERR_OPERATOR_ILLEGAL] = {.desc = "upp err operator illegal"},
    /* 运行中的错误，记录在日志中 */
};
/*需要单位转换的指令*/
const char *TEMP_CMD_TBL[] = {"DL", "DH", "TCLS", "TCLR", "TCHS", "TCHR", "T",  "RT", "ST", "I",  "NT",
                              "WT", "TN", "TON",  "TOF",  "ION",  "IOF",  "CT", "AL", "AH", "AT", NULL};
/*FMT_F格式的校验,对设置范围二次限制到DL，DH所设的范围内*/
const char *AdjRange_CMD_TBL[] = {"AL", "AH", "AT", "CA", "TCLS", "TCLR", "TCHS", "TCHR", "TON", NULL};
/*
 * 功能描述：该函数配置除直接写参数外的其他操作
 * 入参说明：pkt_parse --- UPP包包解析结构体。
 * 返 回 值：设置命令应答状态（0不应答，1应答）。
 */
int upp_set_callback(parse_t *parse, cmd_id_e cmd_id) {
    int ret = 0;

    switch (cmd_id) {

            /* 0_device */
            //        case CMD_DB: dbg_set_level(sys_para.port.dbg_level); break;
        case CMD_RS: sys_state.sys_op.valid = true; break;
        case CMD_PR: sys_state.sys_op.valid = true; break;
        case CMD_BR:
            if (!is_valid_baudrate(sys_para.port.baudrate)) {
                sys_para.port.baudrate = 115200;
                parse->err             = UPP_ERR_DATA_RANGE;
                ret                    = -1;
            }
            /*生效波特率*/
            pc_init();
            break;
        case CMD_DB: dbg_set_level(sys_para.port.dbg_level); break;
        case CMD_DP:
            put_port = sys_para.port.dbg_port;
            if (sys_para.port.dbg_port % 2 == 1) {
                dump_sys_para();
            }
            break;
            /* 5_temptature（输入） */
        case CMD_AT: // 设置模拟温度
            sys_state.ana_temp_convert_flag = 1;
            break;
        case CMD_AM: // 设置模拟输出模式，更新范围
            ana_threshold_range();
            break;
        case CMD_AL: // 设置模拟输出下限，更新范围
            if (sys_para.ana.lower > sys_para.ana.upper) {
                return -1;
            }
            anolog_out_init();
            break;
        case CMD_AH: // 设置模拟输出上限，更新范围
            if (sys_para.ana.lower > sys_para.ana.upper) {
                return -1;
            }
            anolog_out_init();
            break;
        case CMD_DL: // 设置设备测温下限
            if (sys_para.temp.dev_temp_min > sys_para.temp.dev_temp_max) {
                return -1;
            }
            break;
        case CMD_DH: // 设置设备测温上限
            if (sys_para.temp.dev_temp_min > sys_para.temp.dev_temp_max) {
                return -1;
            }
            break;
        case CMD_IL: // 设置模拟输出校准下限
            if (sys_para.ana.type != ANA_0_10V) {
                get_current_calib_para();
            } else {
                get_voltage_calib_para();
            }
            break;
        case CMD_IH: // 设置模拟输出校准上限
            if (sys_para.ana.type != ANA_0_10V) {
                get_current_calib_para();
            } else {
                get_voltage_calib_para();
            }
            break;
        case CMD_SN: // 窄带挡位
            gain_sw_data[SENS_CH_SPEC_NB].flag = 1;
            if (sys_state.now_gain[SENS_CH_SPEC_NB] == 0) {
                gain_sw_data[SENS_CH_SPEC_NB].gm_to_gl = sys_para.gain.volt_discard;
            } else if (sys_state.now_gain[SENS_CH_SPEC_NB] == 1) {
                gain_sw_data[SENS_CH_SPEC_NB].gl_to_gm = sys_para.gain.volt_discard;
            } else if (sys_state.now_gain[SENS_CH_SPEC_NB] == 2) {
                gain_sw_data[SENS_CH_SPEC_NB].gm_to_gh = sys_para.gain.volt_discard;
            }
            break;
        case CMD_SW: // 宽带挡位
            gain_sw_data[SENS_CH_SPEC_WB].flag = 1;
            if (sys_state.now_gain[SENS_CH_SPEC_WB] == 0) {
                gain_sw_data[SENS_CH_SPEC_WB].gm_to_gl = sys_para.gain.volt_discard;
            } else if (sys_state.now_gain[SENS_CH_SPEC_WB] == 1) {
                gain_sw_data[SENS_CH_SPEC_WB].gl_to_gm = sys_para.gain.volt_discard;
            } else if (sys_state.now_gain[SENS_CH_SPEC_WB] == 2) {
                gain_sw_data[SENS_CH_SPEC_WB].gm_to_gh = sys_para.gain.volt_discard;
            }
            break;
        case CMD_CA:
            if (calibrate_temperature()) {
                parse->err = UPP_ERR_DATA_CRC;
                ret        = -1;
            }
            break;

            //        case CMD_TCLS: sys_para.temp.calib_low_set[sys_para.temp.temp_mode] = sys_state.temp_calib_low_set; break;
            //        case CMD_TCLR: sys_para.temp.calib_low_real[sys_para.temp.temp_mode] = sys_state.temp_calib_low_real; break;
            //        case CMD_TCHS: sys_para.temp.calib_high_set[sys_para.temp.temp_mode] = sys_state.temp_calib_high_set; break;
            //        case CMD_TCHR: sys_para.temp.calib_high_real[sys_para.temp.temp_mode] = sys_state.temp_calib_high_real; break;
        /* 6_network */
        case CMD_NR:
            //            netconn_set_recvtimeout(client_conn, sys_para.net.rev_time); /* 超时时间，单位毫秒 */
            break;
        case CMD_IP:
            //            netconn_set_recvtimeout(client_conn, sys_para.net.rev_time); /* 超时时间，单位毫秒 */
            break;

        case CMD_GW:
            //            netconn_set_recvtimeout(client_conn, sys_para.net.rev_time); /* 超时时间，单位毫秒 */
            break;
        case CMD_MA:
            //            netconn_set_recvtimeout(client_conn, sys_para.net.rev_time); /* 超时时间，单位毫秒 */
            break;
            /* 7_record */
#if SUPPORT_EEPROM
        case CMD_RI: {
            ostimer_set_period(OSTIMER_RECORD_TASK, sys_para.record.interval);
            break;
        }

        case CMD_RQ: {
            if (sys_state.index_clear == 80295) {
                record_clear();
            }
            break;
        }

        case CMD_RO: {
            for (int i = 0; i < REC_TYPE_CNT; i++) {
                rec_stop[i].duration = (sys_state.record_open == 0) ? ~0 : 0;
            }
            break;
        }
#endif /* SUPPORT_EEPROM */
            /* 9_burst */
        case CMD_V: {
            if (sys_para.burst.mode == 'B') {
                ostimer_open(OSTIMER_BURST_TASK, 1);
            } else if (sys_para.burst.mode == 'P') {
                ostimer_close(OSTIMER_BURST_TASK);
            }
            /* 仅当来源是TCP、且为SET操作、且命令为V=B时，才开启网口突发；否则清除 */
            if (g_cmd_src_group == tcp_group && parse->op == UPP_OP_SET) {
                if (sys_para.burst.mode == 'B') {
                    burst_set_tcp_enable(true);
                } else {
                    burst_clear_tcp_enable();
                }
            }
            break;
        }
        case CMD_BS: burst_init(); break;
        case CMD_BF: burst_init(); break;
        case CMD_AP: {
            if (algo_load(parse)) {
                ret = -1;
            }
            break;
        }
        case CMD_NWA: {
            int flag = algo_network_para_load(parse, CMD_NWA);
            if (flag < 0) {
                ret = -1;
            } else {
                ret = flag;
            }
            break;
        }
        case CMD_NWB: {
            int flag = algo_network_para_load(parse, CMD_NWB);
            if (flag < 0) {
                ret = -1;
            } else {
                ret = flag;
            }
            break;
        }
        case CMD_NB: {
            int flag = algo_network_para_load(parse, CMD_NB);
            if (flag < 0) {
                ret = -1;
            } else {
                ret = flag;
            }
            break;
        }
        case CMD_EM: // 设置曝光模式
            sys_para.video.ctl_state = 1;
            sys_para.video.ctl_val   = UVC_CTL_SET_EM_MODE; //     UVC_CTL_SET_EM_MODE   1
            break;
        case CMD_EV: // 设置曝光值
            sys_para.video.ctl_state = 1;
            sys_para.video.ctl_val   = UVC_CTL_SET_EM_VAL;
            break;
        case CMD_BN:
            sys_para.video.ctl_state = 1;
            sys_para.video.ctl_val   = UVC_CTL_SET_BN_VAL;
            break;
        case CMD_BL: // 设置逆光对比度
            sys_para.video.ctl_state = 1;
            sys_para.video.ctl_val   = UVC_CTL_SET_BL_VAL;
            break;
        case CMD_GN: // 增益
            sys_para.video.ctl_state = 1;
            sys_para.video.ctl_val   = UVC_CTL_SET_GN_VAL;
            break;
        default: break;
    }

    int8_t para_type = CMD_TBL[parse->cmd_idx].att.group;
    // uint16_t tmp16     = CMD_TBL[parse->cmd_idx].att.props;
    if (ret == 0 && (CMD_TBL[parse->cmd_idx].att.props & M) && para_type < PARA_TYPE_CNT) {
        pmm_save_group(para_type);
    }

    return ret;
}

/*
 * 功能描述：该函数配置除直接写参数外的其他操作
 * 入参说明：pkt_parse --- UPP包包解析结构体。
 * 返 回 值：设置命令应答状态（0不应答，1应答）。
 */
int upp_get_callback(parse_t *parse, cmd_id_e cmd_id) {
    int ret = 0;

    switch (cmd_id) {
            //        case CMD_XV: {
            //            hex_to_string(sys_state.dev_sn, *((__IO uint32_t *)(SIGNATURE_ADDRESS)), 8);
            //            break;
            //        }
            //        case CMD_XU: {
            //            string_copy(sys_state.dev_info, "E1RH-F2-V-0-0"); /* 适配弗鲁克假值 */
            //            break;
            //        }
            //        case CMD_XR: { /* 系统版本 */
            //                       //sprintf((char *)sys_state.dev_version, "BOOT:V%d.%d.%d,HW:V%d.%d.%d,APP:V%d.%d.%d",
            //                       BOOT_VER_MAJOR, BOOT_VER_MINOR,
            //                       // BOOT_VER_SERIAL,
            //            //                    HW_VER_MAJOR, HW_VER_MINOR, HW_VER_SERIAL, FW_VER_MAJOR, FW_VER_MINOR, FW_VER_SERIAL);
            //            sprintf((char *)sys_state.dev_version, "2.02.29"); /* 适配弗鲁克假值 */
            //            break;
            //        }
            //        /* 7_record */
            // #if SUPPORT_EEPROM
            //        case CMD_RN: {
            //            sprintf((char *)sys_state.record_max_cnt, "BBOX:%d,STATE:%d", REC_BBOX_CNT, REC_STATE_CNT);
            //            break;
            //        }
            // #endif /* SUPPORT_EEPROM */
            //        /* 11_video */
            //        case CMD_RC: {
            //            sprintf((char *)sys_state.video_relative_reticle_diameter, "0.100000");
            //            break;
            //        }
        case CMD_AP: {
            if (algo_get(parse)) {
                ret = -1;
            }
            break;
        }
        case CMD_NWA: {
            int flag = algo_network_para_get(parse, CMD_NWA);
            if (flag < 0) {
                ret = -1;
            } else {
                ret = flag;
            }
            break;
        }
        case CMD_NWB: {
            int flag = algo_network_para_get(parse, CMD_NWB);
            if (flag < 0) {
                ret = -1;
            } else {
                ret = flag;
            }
            break;
        }
        case CMD_NB: {
            int flag = algo_network_para_get(parse, CMD_NB);
            if (flag < 0) {
                ret = -1;
            } else {
                ret = flag;
            }
            break;
        }
        case CMD_EM: // 获取当前曝光模式
            sys_para.video.ctl_state = 1;
            sys_para.video.ctl_val   = UVC_CTL_GET_EM_MODE; //     UVC_CTL_SET_EM_MODE   1
            break;
        case CMD_EV: // 获取当前曝光值
            sys_para.video.ctl_state = 1;
            sys_para.video.ctl_val   = UVC_CTL_GET_EM_VAL;
            break;
        case CMD_BN: // 获取当前亮度值
            sys_para.video.ctl_state = 1;
            sys_para.video.ctl_val   = UVC_CTL_GET_BN_VAL;
            break;
        case CMD_BL: // 获取当前曝光值
            sys_para.video.ctl_state = 1;
            sys_para.video.ctl_val   = UVC_CTL_GET_BL_VAL;
            break;
        case CMD_GN: // 获取当前亮度值
            sys_para.video.ctl_state = 1;
            sys_para.video.ctl_val   = UVC_CTL_GET_GN_VAL;
            break;
        default: break;
    }

    return ret;
}

/*
 * 功能描述：该函数用于从 UPP 包解析结构体中提取逗号分隔的整数数据，
 *           并存储到指定的缓冲区中，同时配置相关操作。
 * 入参说明：pkt_parse --- UPP 包解析结构体，包含数据和数据长度。
 *           buff --- 用于存储提取的整数数据；
 *           type --- 数据类型。
 * 返 回 值：无
 */
void str_extract_data(parse_t *pkt_parse, void *buff, int type) {
    char      cfg_copy[50];
    char     *token;
    char     *pos_u8;
    float    *pos_f;
    uint32_t *pos_u32;
    uint32_t  val32;
    float     valf;

    /* 根据传入的类型，初始化对应的指针 */
    switch (type) {
        case TYPE_UINT8: pos_u8 = (char *)buff; break;
        case TYPE_FLOAT: pos_f = (float *)buff; break;
        case TYPE_UINT32: pos_u32 = (uint32_t *)buff; break;
        default: return; /* 如果类型不匹配，直接返回 */
    }

    /* 清空配置缓冲区 */
    mem_set(cfg_copy, sizeof(cfg_copy), 0);
    string_copy_n(cfg_copy, pkt_parse->data, pkt_parse->data_len);
    cfg_copy[pkt_parse->data_len] = '\0'; /* 确保字符串以 '\0' 结尾 */

    /* 使用 simple_strtok 分割数据 */
    token = simple_strtok(cfg_copy, ",");
    while (token != NULL) {
        switch (type) {
            case TYPE_UINT8:
                val32   = atoi(token); /* 转换为整数 */
                *pos_u8 = (char)val32; /* 存储到 uint8_t 指针 */
                pos_u8++;
                break;
            case TYPE_FLOAT:
                valf   = (float)atof(token); /* 转换为浮点数 */
                *pos_f = valf;               /* 存储到 float 指针 */
                pos_f++;
                break;
            case TYPE_UINT32:
                val32    = atoi(token); /* 转换为整数 */
                *pos_u32 = val32;       /* 存储到 uint32_t 指针 */
                pos_u32++;
                break;
            default: break; /* 如果没有匹配类型，则跳过 */
        }
        token = simple_strtok(NULL, ",");
    }
}
