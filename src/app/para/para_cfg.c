/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：para_cfg.c
 * 文件描述：系统参数配置接口代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈  军 初始版本
 *           V1.2 2024/04/10 李兆越 增加参数备份逻辑
 *           V1.3 2024/08/15 段政宏 梳理参数保存机制，简化参数操作逻辑
 */

#include "platform.h"
#include "proto_char.h"
#include "para.h"
#include "para_cfg.h"
#include "storage.h"
#include "ui.h"
#include "alarm.h"
#include "ad7124.h"

#if SUPPORT_BACKUP
sys_para_t sys_para_bsp; /* 备份参数 */
#endif

sys_para_t  sys_para;        /* 用户参数 */
sys_state_t sys_state = {0}; /* 系统全局参数（不保存） */

sys_quick_save_t   sys_quick_save __attribute__((section("NO_INIT"), zero_init)); /* Things that require quick saving */
extern HEATER_DATA heater_data;
/* 参数信息表 */
/* clang-format off */
const para_item_t PARA_INFO_TBL[] = {
    [PARA_DEV]     = {.desc = "dev",     .offset = offset_of(dev, sys_para_t),     .size = sizeof(dev_para_t)},
    [PARA_PORT]    = {.desc = "port",    .offset = offset_of(port, sys_para_t),    .size = sizeof(port_para_t)},
    [PARA_GAIN]    = {.desc = "gain",    .offset = offset_of(gain, sys_para_t),    .size = sizeof(gain_para_t)},
    [PARA_ANA]     = {.desc = "ana",     .offset = offset_of(ana, sys_para_t),     .size = sizeof(ana_para_t)},
    [PARA_ADC]     = {.desc = "adc",     .offset = offset_of(adc, sys_para_t),     .size = sizeof(ad7124_para_t)},
    [PARA_TEMP]    = {.desc = "temp",    .offset = offset_of(temp, sys_para_t),    .size = sizeof(temp_para_t)},
    [PARA_NET]     = {.desc = "net",     .offset = offset_of(net, sys_para_t),     .size = sizeof(net_para_t)},
    [PARA_RECORD]  = {.desc = "record",  .offset = offset_of(record, sys_para_t),  .size = sizeof(record_para_t)},
    [PARA_ALGO]    = {.desc = "algo",    .offset = offset_of(algo, sys_para_t),    .size = sizeof(algo_para_t)},
    [PARA_BURST]   = {.desc = "burst",   .offset = offset_of(burst, sys_para_t),   .size = sizeof(burst_para_t)},
    [PARA_ALARM]   = {.desc = "alarm",   .offset = offset_of(alarm, sys_para_t),   .size = sizeof(alarm_para_t)},
    [PARA_VIDEO]   = {.desc = "video",   .offset = offset_of(video, sys_para_t),   .size = sizeof(video_para_t)},
    [PARA_FILTER]  = {.desc = "filter",  .offset = offset_of(filter, sys_para_t),  .size = sizeof(filter_para_t)},
    [PARA_HEATER]  = {.desc = "heater",  .offset = offset_of(heater, sys_para_t),  .size = sizeof(heater_para_t)},
};

/* 默认参数表 */
/* 注意：修改后记得保存 para_save */
const sys_para_t SYS_DEF_PARA = {
    .dev = /* 0_device */
    {
        .addr        = 0,            /* 设备地址：1 */
        .name        = "LED102_V3",  /* 设备名字 */
        .sn          = "2312140001", /* 设备SN码，时间加+编号 */
        .factory_set = false,        /* 默认未出厂参数设置,N.Y */
        .panel_lock = 'U',
    },

    .port = /* 1_port */
    {
        .mode      = COM_FULL_DUPLEX, /* todo，UART_FULL_DUPLEX 冗余 全双工 */
        .baudrate  = 115200,          /* 波特率115200 */
        .dbg_level = DBG_LEVEL_NONE,   /* DEBUG等级关闭 */
        .check_fmt = "8N1",
    },

    .gain = /* 2_gain */
    {
        .upper        = {3250000, 3250000},         /* 跨阻切换临界值（上限） */
        .lowerml      = {3220000, 3220000},         /* 跨阻切换临界值（下限）M档切L档 */
        .lowerhm      = {3220000, 3220000},         /* 跨阻切换临界值（下限）H档切M档 */
        .k_lm         = {10000 / 120, 47000 / 680}, /* L档/M档所需系数 */
        .k_mh         = {120 / 1.5f, 680 / 10.0f},  /* M档/H档所需系数 */
        .cnt_thod     = 3,                          /* 切挡阈值检测区间的检测个数 */
        .volt_discard = 10,                          /* VD丢弃电压个数 */
        .gl_to_gm     = 5,
        .gm_to_gh     = 5,
        .gh_to_gm     = 5,
        .gm_to_gl     = 5,
    },

    .ana = /* 3_analogout */
    {
        .type            = ANA_4_20MA, /* 模拟输出电流范围：4~20mA */
        .upper           = 2800, /* 模拟输出温度上限 */
        .lower           = 800, /* 模拟输出温度下限 */
        .exceed_min_range= 2.5,
        .exceed_max_range= 21,
        .real_curt_2ma   = 1000000,    /* 理论2mA实际输出的电流：2 mA */
        .real_curt_20ma  = 19500000,   /* 理论20mA实际输出的电流：20mA */
        .real_volt_1v    = 1000000,    /* 理论1V实际输出的电压：1V */
        .real_volt_10v   = 10000000,   /* 理论10V实际输出的电压：10V */
        .expos_time      = 1000,       /* 响应时间：1000ms */
        .lower_alarm_out = 2500000,    /* 下限报警输出值：2.5mA */
        .upper_alarm_out = 21000000,   /* 上限报警输出值：21mA */
    },

    .adc = /* 4_adc */
    {
        .ctrl_pwr_mode  = FULL_POWER_MODE, /* ADC_CTRL 功率模式 */
        .ctrl_work_mode = CONTINU_MODE,    /* ADC_CTRL 工作模式 */
        .ctrl_clk_src   = INTER_CLK,       /* ADC_CTRL 时钟源选择 */
        .pga            = 0,               /* 可编程增益: PGA1 */
        .offs0          = 8388050,         /* 0通道默认偏置(0x0800000) */
        .offs2          = 8388050,         /* 2通道默认偏置(0x0800000) */
    },

    .temp = /* 5_temptature */
    {
        .unit              = TEMP_UNIT_INTERNAL, /* 温度单位 */
        .dev_temp_max      = 2800,         /* 输出温度上限 */
        .dev_temp_min      = 800,         /* 输出温度下限 */
        .volt_filter_len   = 25,                 /* 电压滤波长度 */
        .emissivity        = 1.0000f,            /* 发射率 */
        .trans             = {1.0000f, 1.0000f}, /* 透射率 */
        .trans_interior    = {1.0000f, 1.0000f}, /* 透射率内部 */
        .slope             = 1.0000f,            /* 坡度 */
        .slope_interior    = 1.0000f,            /* 坡度(内部) */
        .temp_mode         = 2,                  /* 测温模式，默认比色 */
        .temp_calib_gain   = {1.0f, 1.0f, 1.0f}, /* 温度校准增益 [宽带, 窄带, 比色] */
        .temp_calib_offset = {0.0f, 0.0f, 0.0f}, /* 温度校准偏置 [宽带, 窄带, 比色] */
        .valid_vol = 15000,                      /* 有效电压阈值 */
        .ambient_comp_ctl = 0,                  /* */
    },

    .net = /* 6_network */
    {
        .interval  = RECONNECT_INTERVAL, /* 网络连接间隔，默认3s，单位为秒 */
        .rev_time  = RECV_TIMEOUT_S,     /* 接收超时时间，默认5s，单位为秒 */
        .dhcp      = 0,                  /* 启用DHCP，0关闭，1打开 */
        .port      = 6363,               /* 网络端口号 */
        .ip_addr   = "192.168.042.132",  /* 本地ip地址 */
        .remote_ip = "192.168.042.100",  /* 远端ip地址 */
        .net_mask  = "255.255.255.000",  /* 子网掩码 */
        .gateway   = "192.168.042.001",  /* 网关 */
    },

    .record = /* 7_record */
    {
        .interval = 5000, /* 黑盒日志保存间隔，默认5，,单位秒  */
    },
    .algo = /* 8_algorithm */
    {
        .log_para =
            {{{.threshold = 0.0f, .coef = {{.base = -651905818493707, .index = -19}, {.base = 651905818493707, .index = -18}}},   /* ch1 seg1 */
              {.threshold = 0.0f, .coef = {{.base = -651905818493707, .index = -19}, {.base = 651905818493707, .index = -18}}}},  /* ch1 seg2 */
             {{.threshold = 0.0f, .coef = {{.base = -753280579290912, .index = -19}, {.base = 729182522345416, .index = -18}}},   /* ch2 seg1 */
              {.threshold = 0.0f, .coef = {{.base = -753280579290912, .index = -19}, {.base = 729182522345416, .index = -18}}}}}, /* ch2 seg2 */
        .poly_para = {{.poly_term  = 14,
                       .subtrahend = 0.0f,
                       .coef       = {{.base = 158252892724310, .index = -8},
                                      {.base = -221546080998633, .index = -7},
                                      {.base = 139736955204198, .index = -6},
                                      {.base = -526150785636936, .index = -6},
                                      {.base = 132101803699095, .index = -5},
                                      {.base = -233815522865189, .index = -5},
                                      {.base = 300487436456394, .index = -5},
                                      {.base = -284197599553157, .index = -5},
                                      {.base = 198005705736088, .index = -5},
                                      {.base = -100459445611889, .index = -5},
                                      {.base = 361062560771199, .index = -6},
                                      {.base = -871201356289380, .index = -7},
                                      {.base = 126577776890338, .index = -7},
                                      {.base = -836901428276062, .index = -9}}}},
        .network_para = {.effective_w1 = {
                                        5.145944896835715,  5.303004188150759,    -1.0338710269038023, 6.126637116564077,   -1.2943014166810225, 1.1919089027451732, -3.4556343639441716,
                                        3.3572506624122807, 4.805472215074436,    -4.752001172102049,  5.424100498123296,   1.1061399876954876,  4.858462426168612,  0.3856187059390941,
                                        3.5837411603990783, -1.3498039214614383,  4.739205544891455,   0.8734124234175158,  -2.54008217313577,   1.5059999891241618, -2.4004105592230327,
                                        -0.692814931096748, -2.4497619234969616,  3.9240908140624877,  -5.179697250092993,  -3.2977840266359437, -1.298776167160511, -3.9822482052672186,
                                        0.9693369730561593, -5.731996467619839,   5.831459220749384,   -5.4598325814597715, 5.17905708096482,    0.9834778014744124, -2.0041737176508376,
                                        3.6516445625545857, 0.9209738874996497,   5.425894804765588,   0.5480054840337653,  -1.863662763047472,  2.3564292480405293, -1.6022904504120585,
                                        2.485993169545714,  6.171765514939905,    3.4157232029565416,  -2.9285925681488765, 3.0860646557464597,  0.9499751177022816, 2.6217987400505893,
                                        -3.844121140382615, -5.563809302478226,   -2.704915853191308,  -4.496338561762354,  5.109866995264251,   1.7019566052965347, 2.4466989116276343,
                                        2.5663125391370114, -0.10278834538397565, 5.292784043126528,   -4.60079258949862,   0.37200013003372023, -4.243847185716187, 2.0219801841083593,
                                        -2.035229499462102},
                        .effective_b1 = {
                                        -3.6798133116137848, -4.309563107431072,  1.5744527125220023,  -5.27379504155682,    0.42981750995957047, -1.5404719841259964,
                                        3.762901536814493,   -2.3486941447097673, -2.8770452634190287, 3.040692773667864,    -5.250694691671868,  -1.6579151165342552,
                                        -4.439562577397817,  0.19419433685256238, -2.4544069275773697, 2.0133779552540316,   -4.235199684039726,  -1.373350054937731,
                                        2.478688732985477,   -1.5971510466600092, 2.42243415854593,    0.3119947627456383,   2.558537744044954,   -3.9042019784612214,
                                        3.73136983075209,    3.000479984610666,   1.1746941165090867,  3.0033460580609246,   -0.2462062113210819, 5.195395814918857,
                                        -5.262979816872043,  3.9013940285886504,  -3.151709989803612,  -1.116553124166329,   1.2648423142030316,  -3.86038853185081,
                                        -1.3019996392144246, -3.9316464761909105, -0.5826428623745903, 0.7515327706701505,   -1.7113797410724663, 0.5872068607408287,
                                        -2.817443806656516,  -5.179589893254828,  -3.581044774362959,  1.7626447283775324,   -1.3667961293948419, -0.5739268248293802,
                                        -1.8142978573997883, 2.139380897205181,   3.6996485056582324,  1.9142818303777287,   3.8224268508661106,  -4.891788162079611,
                                        -1.7791098393757505, -2.5370932312039676, -1.9364671435931662, -0.29038770460760926, -4.097349984876369,  3.057627095554052,
                                        -0.9806779392128149, 2.846184282440293,   -2.025790738662742,  0.8246169297570534},
                        .effective_w2 = {118.55137158607266,  37.00026114841003,   16.3105447639788,   66.19991983250941,   -64.17143142522804,  52.4682665062934,
                                        -71.73713809949595,  22.222828610005635,  -14.73209615851932, -89.00631116988791,  72.01430518236754,   -16.554599635630794,
                                        77.56304211660934,   -71.92970380492568,  109.7252323731129,  -111.45637907065746, 27.82474271304255,   15.828168350244065,
                                        -6.988732583863066,  61.175420192953105,  23.307840032025005, -60.08372911976136,  -29.066126713658537, 86.58586245989176,
                                        -40.79500399328412,  -106.9338527293604,  24.590077538981983, -93.13972634878068,  -107.37283741453308, -20.62942880158348,
                                        70.35016755145878,   -72.72760328765638,  119.38124927695492, 16.65373162865615,   5.080300957026268,   -66.78588397814397,
                                        31.811809312542223,  43.582634530183626,  -26.22088022825798, 65.09326851146511,   79.21878156726955,   -41.94007800651562,
                                        -38.36356574209037,  44.11025009123729,   42.32588778411567,  -6.249590401674473,  -8.450266942803772,  20.944412569020663,
                                        -6.761262648407345,  71.27717831470412,   32.63796955368603,  -101.39169595250233, -27.843309610732586, 81.3026699250327,
                                        13.748381850039033,  -27.000995196272786, 101.00119187705737, 39.83356837917615,   84.89814932352493,   -38.46620262994213,
                                        -20.071922140716808, -15.555256888058471, 112.19274164512647, -22.59092397078633},
                        .effective_b2     = {1792.9523056649618},
        }
    },

    .burst = /* 9_burst */
    {
        .mode     = 'P',                          /* 轮询模式 */
        .speed    = 50,                           /* 上报总间隔，单位：ms ,大于100ms *4 */
        .cmd_list = "T,WT,NT,WV,NV,I,ST,B,SW,SN", /* 上报类型 */
        .fmt      = "NS",                         /* 上传格式指令 */
    },

    .alarm = /* 10_alarm */
    {
        .relay_alarm_mode        = ALARM_NO_TARGET_INTERNAL, /* 报警输出由目标温度或内部温度触发，正常开路 */
        .target_temp_alarm_on    = 1000.0f,                  /* 目标温度上升到1000报警 */
        .target_temp_alarm_off   = 1.0f,                   /* 目标温度下降到980恢复 */
        .internal_temp_alarm_on  = 65.0f,                    /* 内部温度上升到65报警 */
        .internal_temp_alarm_off = 1.0f,                    /* 内部温度下降到60恢复 */
        .decay_rate              = 90,                       /* 衰减率阈值 */
        .decay_fail_safe         = 90,                       /* 衰减安全保护 */
        .attenuation_alarm_on    = 0.90f,                    /* 衰减报警触发（90%） */
        .attenuation_alarm_off   = 0.01f,                    /* 衰减恢复阈值（95%） */
    },

    .video = /* 11_video */
    {
        .aiming_x               = 100.0f, /* 瞄准坐标X */
        .aiming_y               = 100.0f, /* 瞄准坐标Y */
        .exposure_mode          = 8,      /* EV曝光等级 */
        .exposure_level         = -7,
        .brightness_val         = 0,
        .backlight_val          = 2,
        .gain_val               = 3,
        .ctl_state              = 0,
        .ctl_val                = 0,
        .video_control          = 1, /* 视频控制 */
        .video_protocol_control = 0, /* 视频协议控制 */
        .video_send_fre         = 1,
        .test_delay             = 10,
        .send_size_test         = 2000,

    },
    .filter = /* 12_filter */
    {
        .temp_filter_buf_size      = 25, /* 传感器温度滤波buf大小*/
        .advance_hold_threshold    = 0,  /* 高级保持门限（未用到） */
        .valley_holding_time       = 0,  /* 谷值保持时间（未用到） */
        .advance_hold_average_time = 0,  /* 高级保持平均时间（未用到） */
        .peak_holding_time         = 0,  /* 峰值保持时间（未用到） */

    },

    .heater = /* 13_heater */
    {
        .set_temp = 50,    /* 加热器目标温度 */
        .kp       = 12.0f, /* 加热器PID比例 */
        .ki       = 0.03f, /* 加热器PID积分 */
        .kd       = 70.0f, /* 加热器PID微分 */

    },
};
/* clang-format on */

/*
 * 功能描述：这个函数用于在上电时获取串口参数
 * 入参说明：para --- 参数结构体
 * 返 回 值：0 --- 成功，其它 --- 失败
 * 备    注：
 */
int powerup_uart_para_get(port_para_t *para) {
    uint32_t   offset;
    sys_para_t bsp_para;

    /* 从GD-FLASH读取串口参数 */
    offset = offset_of(port, sys_para_t);
    mem_copy(&bsp_para, (const void *)USER_PARA_ADDR, sizeof(bsp_para));

    if (bsp_para.crc[PARA_PORT] != (uint16_t)~crc16_RTU((uint8_t *)&bsp_para + offset, sizeof(port_para_t))) return -1;

    para->mode     = bsp_para.port.mode;
    para->baudrate = bsp_para.port.baudrate;
    para->dbg_port = bsp_para.port.dbg_port;
    mem_copy(para->dbg_level, bsp_para.port.dbg_level, sizeof(para->dbg_level));

    return 0;
}

/*
 * 功能描述：这个函数用于在上电时获取需要稳定的参数
 * 入参说明：无
 * 返 回 值：0 --- 成功，其它 --- 失败
 * 备    注：
 */
void powerup_quick_save_get(void) {

    /* 从FLASH读取参数 */
    mem_copy(&sys_quick_save, (const void *)QUICK_SAVE_ADDR, sizeof(sys_quick_save));

    if (sys_quick_save.flag_temp_internal == 1) {
        sys_quick_save.flag_temp_internal = 0;
        sys_state.internal_temp[0]        = sys_quick_save.temp_internal_n;
        sys_state.internal_temp[1]        = sys_quick_save.temp_internal_w;
    }

    return;
}

/*
 * 功能描述：安全复位
 * 入参说明：无。
 * 返 回 值：无。
 * 备    注：无。
 */
void safe_reset(void) {
    // #if SUPPORT_EEPROM
    //     /* 参数保存 */
    //     sys_quick_save.flag_temp_internal = 1;
    //     sys_quick_save.temp_internal_n    = sys_state.internal_temp[0];
    //     sys_quick_save.temp_internal_w    = sys_state.internal_temp[1];
    //     gdf_write_data_by_page(QUICK_SAVE_ADDR, sizeof(sys_quick_save_t), (uint8_t *)&sys_quick_save);
    // #endif
    hot_start_quick_save_set();
    /* 复位 */
    // gd32_delay_ms(10); /* 确保写入完成 */
    NVIC_SystemReset();
}

/*
 * 功能描述：这个函数用于在热启动时获取需要稳定的参数
 * 入参说明：无
 * 返 回 值：0 --- 成功，其它 --- 失败
 * 备    注：
 */
void hot_start_quick_save_get(void) {

    if (sys_quick_save.flag_temp_internal == 1) {
        sys_quick_save.flag_temp_internal = 0;
        sys_state.internal_temp[0]        = sys_quick_save.temp_internal_n;
        sys_state.internal_temp[1]        = sys_quick_save.temp_internal_w;

        sys_state.ratio_temp       = sys_quick_save.ratio_temp;
        sys_state.ratio_temp_nerve = sys_quick_save.ratio_temp_nerve;
        sys_state.target_temp      = sys_quick_save.target_temp;

        heater_data.integral          = sys_quick_save.integral;
        heater_data.previous_integral = sys_quick_save.integral;

        sys_state.now_gain[SENS_CH_SPEC_NB] = sys_quick_save.nb_gain;
        sys_state.now_gain[SENS_CH_SPEC_WB] = sys_quick_save.wb_gain;
        // memcpy((void *)&heater_data, (void *)&sys_quick_save.heater_data, sizeof(HEATER_DATA));
    }
    return;
}

/*
 * 功能描述：这个函数用于为热启动时提供需要稳定的参数
 * 入参说明：无
 * 返 回 值：0 --- 成功，其它 --- 失败
 * 备    注：
 */
void hot_start_quick_save_set(void) {
    sys_quick_save.flag_temp_internal = 1;
    sys_quick_save.temp_internal_n    = sys_state.internal_temp[0];
    sys_quick_save.temp_internal_w    = sys_state.internal_temp[1];

    sys_quick_save.ratio_temp       = sys_state.ratio_temp;
    sys_quick_save.ratio_temp_nerve = sys_state.ratio_temp_nerve;
    sys_quick_save.target_temp      = sys_state.target_temp;

    sys_quick_save.integral = heater_data.integral;

    sys_quick_save.nb_gain = sys_state.now_gain[SENS_CH_SPEC_NB];
    sys_quick_save.wb_gain = sys_state.now_gain[SENS_CH_SPEC_WB];
    // memcpy((void *)&sys_quick_save.heater_data, (void *)&heater_data, sizeof(HEATER_DATA));
    return;
}
