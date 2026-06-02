/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：measure.c
 * 文件描述：红外温度测量驱动代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/03/20  陈军    初始版本
 *           V1.1 2025/04/09  乔鹤    增加单色和比色测温业务
 *           V1.2 2025/04/28  李兆越  增加温度校准和两点校准业务
 */

#include "agent.h"
#include "ostimer.h"
#include "measure.h"
#include "analogout.h"
#include "tiag.h"
#include "ad7124.h"
#include "ntc_cfg.h"
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "self_check.h"
extern int  warm_start;
extern bool exti_triger_flag;
sens_spec_t sens_spec[SENS_CH_SPEC_CNT]; /* 传感器光谱数据 */
sens_ntc_t  sens_ntc[SENS_CH_NTC_CNT];   /* 传感器NTC数据 */

static uint8_t        sample_status;
static double         coef_a[SENS_CH_SPEC_CNT], coef_b[SENS_CH_SPEC_CNT];
static float          gain_compes_factor[SENS_CH_SPEC_CNT];
gain_sw_data_t        gain_sw_data[SENS_CH_SPEC_CNT];
static slid_remove_t  slid_remove_volt[SENS_CH_SPEC_CNT];
static slid_remove_t  slid_remove_ntc_volt;
static slid_holding_t slid_holding_data[SENS_CH_SPEC_CNT];
static slid_aver_t    slid_aver_temp[SENS_CH_SPEC_CNT];
static slid_aver_t    slid_aver_volt[SENS_CH_SPEC_CNT];
long long             filter_buffer[SENS_CH_SPEC_CNT][20];
long long             filter_buffer_NTC[SENS_CH_NTC_CNT][FILTER_LEN_MAX];
static void           temp_task(void *arg);
static void           sample_task(void *arg);
static float          adc_to_voltage(uint32_t adc_value);
static void           get_temp(sens_spec_t *data);
static float          get_single_temp(long long voltage, uint8_t ch);
static float          get_ratio_temp(sens_spec_t *data);
static void           get_internal_temp(sens_ntc_t *data);
static int            volt_normalization(sens_spec_t *data);
void                  gain_status_init(void);
static void           gain_temp_insert(sens_spec_t *data);
static void           volt_remove_average_filter(sens_spec_t *data, sens_ntc_t *ntc_data);
static void           get_sin_coefficient(double *coef1, double *coef2, float compes_volt, uint8_t spec_num);
static void           get_multi_coefficient(double *coef, float ratio_value, uint8_t poly_num);
static int            gain_switch_process(sens_spec_t *data);
static float          temp_two_point_calibrate(float raw_temp, int mode); /* 温度两点校准 */
static void           internal_temp_calibrate(sens_spec_t *data);
static long long      internal_temp_power_compensate(long long raw_value, int spec);
static void           timer4_init(void);
static void           timer4_trigger_callback(void);
static bool           timer4_trigger_flag = false;

float               ratio2temp(float ratio);
static TaskHandle_t os_sample_task;
/*
 * 功能描述：温度测量单元初始化
 * 入参说明：无
 * 返 回 值：0 -- 成功，其它 -- 失败。
 */
int temp_init(void) {
    /* 数据结构 */
    mem_set(&sens_spec, sizeof(sens_spec) * SENS_CH_SPEC_CNT, 0);
    mem_set(&sens_ntc, sizeof(sens_ntc) * SENS_CH_NTC_CNT, 0);

    sample_status = SAMPLE_NO_START;

    /* 初始化跨阻单元 */
    tiag_init();
    gain_status_init();

    /* 初始化AD7124 */
    if (ad7124_init(sys_para.adc)) {
        dbg_info("%s\r\n", "AD7124 initialized failed!");
        ENTER_CRITICAL();
        sys_state.dev_error[DEV_ERR_AD7124] = DEV_ST_ERR;
        EXIT_CRITICAL();
        return -1;
    }
    timer4_init();
    // ostimer_register(OSTIMER_TEMP_TASK, temp_task, (void *)0, AGENT_PRIO_HI, TEMP_TASK_PERIOD);

    BaseType_t ret;
    ret = xTaskCreate((TaskFunction_t)sample_task,      /* 任务入口函数 */
                      (const char *)"sample_task",      /* 任务名字 */
                      (uint16_t)SAMPLE_TASK_STKSIZE,    /* 任务堆栈大小 */
                      (void *)NULL,                     /* 任务参数 */
                      (UBaseType_t)SAMPLE_TASK_PRIO,    /* 任务优先级 */
                      (TaskHandle_t *)&os_sample_task); /* 任务句柄 */
    if (ret != pdPASS) {
        printf("%s(): OS-Sample-Task create failed!\r\n", __FUNCTION__);
        return -1;
    }

    return 0;
}

/*
 * 功能描述：这个函数是温度采集任务
 * 入参说明：arg --- 入参
 * 返 回 值：无
 * 备    注：2025-4-23 13:57:08 测试3.826ms
 */
static void temp_task(void *arg) {
    // start_cycle_counter();           //!< 开始总计时
    sens_spec_t *volt_temp_data = sens_spec;
    sens_ntc_t  *ntc_temp_data  = sens_ntc;

    if (sample_status == SAMPLE_NO_START) {
        goto over;
    }

    /* 切档判断 */
    if (gain_switch_process(volt_temp_data)) {
        gain_sw_data[SENS_CH_SPEC_WB].smooth_sta      = 1;                     // 启用平滑过渡
        gain_sw_data[SENS_CH_SPEC_WB].smooth_lastTemp = sys_state.target_temp; // 获取切档前温度
        gain_sw_data[SENS_CH_SPEC_WB].discard_status  = 1;
        goto over;
    }

    /* 将电压做归一化处理 */
    if (volt_normalization(volt_temp_data)) {
        goto over;
    }

    /* 切档时做斜率补偿 */
    gain_temp_insert(volt_temp_data);

    /* 电压历史值记录，去极值滑移平均滤波 */
    volt_remove_average_filter(volt_temp_data, ntc_temp_data);

    /* 环境温度计算 */
    get_internal_temp(ntc_temp_data);

    /* 根据环境温度补偿能量 */
    internal_temp_calibrate(volt_temp_data);

    /* 电压转换为温度 */
    get_temp(volt_temp_data);

over:
    if (sample_status == SAMPLE_START) {
        if (warm_start) {
            if ((slid_remove_volt[SENS_CH_SPEC_NB].count == sys_para.filter.temp_filter_buf_size) ||
                (slid_remove_volt[SENS_CH_SPEC_WB].count == sys_para.filter.temp_filter_buf_size)) {
                anolog_output();
            }
        } else {
            anolog_output();
        }
    } else {
        sample_status = SAMPLE_START;
    }
    // agent_post(NOT_FROM_ISR, AGENT_PRIO_HI, sample_task, (void *)0);
    //  sys_state.long_cycles = time_shift_ms(stop_cycle_counter());  // 获取 start_cycle_counter 以来的时间
    //  dbg_info("temp_task:%fms\r\n", sys_state.long_cycles);
}

/*
 * 功能描述：这个函数根据输入的电压值计算目标温度
 * 入参说明：data --- 指向光谱传感器数据
 * 返 回 值：计算得到的目标温度值
 * 备    注：2025-4-23 13:59:04  测试7-77ms
 */
static void sample_task(void *arg) {
    // start_cycle_counter();           //!< 开始总计时
    uint32_t         data_with_status;
    uint8_t          ch_active;
    uint32_t         data;
    sens_spec_t     *spec    = sens_spec;
    sens_ntc_t      *ntc     = sens_ntc;
    const TickType_t xPeriod = 5;

    while (1) {
        for (uint8_t index = 0; index < SENS_CH_CNT; index++) {
            ad7124_low_cs();

            while (ad7124_covert_done()) {
            }

            ad7124_high_cs();

            ad7124_read_data(&data_with_status);

            ch_active = data_with_status & 0xf;
            data      = data_with_status >> 8;
            if (ch_active == ADC_CH_SPEC_NB) {
                spec[SENS_CH_SPEC_NB].volt_raw = adc_to_voltage(data);
            } else if (ch_active == ADC_CH_SPEC_WB) {
                spec[SENS_CH_SPEC_WB].volt_raw = adc_to_voltage(data);
            } else if (ch_active == ADC_CH_NTC_SPEC) {
                ntc[SENS_CH_NTC_SPEC].volt_raw = adc_to_voltage(data);
                vTaskDelay(xPeriod);
            } else if (ch_active == ADC_CH_NTC_TIA) {
                ntc[SENS_CH_NTC_TIA].volt_raw = adc_to_voltage(data);
            }
        }
        temp_task(NULL);
        vTaskDelay(xPeriod);
    }

    // sys_state.long_cycles = time_shift_ms(stop_cycle_counter());  // 获取 start_cycle_counter 以来的时间
    // dbg_info("sample_task:%fms\r\n", sys_state.long_cycles);
}

/*
 * 功能描述：这个函数将ADC采集值转化为电压值
 * 入参说明：adc_value --- adc采集值
 * 返 回 值：计算得到的电压值
 */
static float adc_to_voltage(uint32_t adc_value) {
    float voltage = ((float)adc_value * AD7124_REF_EXTER1) / (1 << 24);
    return voltage;
}

#define HANDLE_SINGLE_ERROR(out_range)                                                                                                               \
    do {                                                                                                                                             \
        if ((out_range) < 0) {                                                                                                                       \
            error_state_set(ERR_ENERGY_LOW);                                                                                                         \
            error_state_clear(ERR_DETECTOR_FAULT);                                                                                                   \
        } else if ((out_range) > 0) {                                                                                                                \
            error_state_clear(ERR_ENERGY_LOW);                                                                                                       \
            error_state_set(ERR_DETECTOR_FAULT);                                                                                                     \
        } else {                                                                                                                                     \
            error_state_clear(ERR_ENERGY_LOW);                                                                                                       \
            error_state_clear(ERR_DETECTOR_FAULT);                                                                                                   \
        }                                                                                                                                            \
    } while (0)

/*
 * 功能描述：这个函数计算当前衰减率
 * 入参说明：data --- 指向光谱传感器数据
 * 返 回 值：计算得到的衰减率
 */
static float get_attenuation_rate(sens_spec_t *data, double temp) {
    double mid_temp      = 0.0f;
    double voltage       = 0.0f;
    float  decay_rate    = 0.0f;
    double midIndexValue = 0.0f;
    /* 衰减率计算 */
    mid_temp      = 1.0f / (temp + 273.15f);
    midIndexValue = ((mid_temp - coef_b[SENS_CH_SPEC_NB]) / coef_a[SENS_CH_SPEC_NB]);
    voltage       = exp(midIndexValue);
    decay_rate    = 1.0f - ((data[SENS_CH_SPEC_NB].volt_compensate / 1000000.0f) / voltage);
    if (decay_rate > 0 && decay_rate < 1) {
        return decay_rate;
    } else {
        return 0;
        ;
    }
}

/*
 * 功能描述：这个函数根据测温模式获取目标温度
 * 入参说明：data --- 指向光谱传感器数据
 * 返 回 值：计算得到的目标温度值
 */
static void get_temp(sens_spec_t *data) {
    double temp[SENS_CH_SPEC_CNT];
    double ratio_temp;

    /* 电压同步上报 */
    for (int i = 0; i < SENS_CH_SPEC_CNT; i++) {
        sys_state.target_volt[i] = data[i].volt_raw;
    }

    /* 电压有效性判断 */
    if (sys_state.target_volt[SENS_CH_SPEC_NB] < sys_para.temp.valid_vol) {
        // sys_state.target_temp = 0;
        ratio_temp            = 0;
        temp[SENS_CH_SPEC_WB] = 0;
        temp[SENS_CH_SPEC_NB] = 0;
        memset(sys_state.temp_out_range, -1, sizeof(sys_state.temp_out_range));
        error_state_set(ERR_ENERGY_LOW);
        error_state_clear(ERR_DETECTOR_FAULT);
        // return;
    } else {
        /* 获取单色温度 */
        for (int i = 0; i < SENS_CH_SPEC_CNT; i++) {
            temp[i] = get_single_temp(data[i].volt_compensate, i);

            /* 温度两点校准 */
            temp[i] = temp_two_point_calibrate(temp[i], i);
            if (temp[i] > sys_para.temp.dev_temp_min && temp[i] < sys_para.temp.dev_temp_max) {
                /*保持不变*/
                sys_state.temp_out_range[i] = 0;
            } else if (temp[i] < sys_para.temp.dev_temp_min) {
                sys_state.temp_out_range[i] = -1;
                temp[i]                     = 0;
            } else if (temp[i] > sys_para.temp.dev_temp_max) {
                sys_state.temp_out_range[i] = 1;
                temp[i]                     = 0;
            }
        }
        /* 获取比色温度 */
        ratio_temp = get_ratio_temp(data);

        /* 温度两点校准 */
        ratio_temp = temp_two_point_calibrate(ratio_temp, 2);
        /* 衰减率计算 */
        sys_state.Measured_attenuation = get_attenuation_rate(data, ratio_temp);

        /* 切档平滑过渡 */
        if (gain_sw_data[SENS_CH_SPEC_WB].smooth_sta) {
            ratio_temp = switch_temp_smooth(0.996, 100, ratio_temp, SENS_CH_SPEC_WB);
        }

        if (ratio_temp > sys_para.temp.dev_temp_min && ratio_temp < sys_para.temp.dev_temp_max) {
            /*保持不变*/
            sys_state.temp_out_range[2] = 0;
        } else if (ratio_temp < sys_para.temp.dev_temp_min) {
            sys_state.temp_out_range[2] = -1;
            ratio_temp                  = 0;
        } else if (ratio_temp > sys_para.temp.dev_temp_max) {
            sys_state.temp_out_range[2] = 1;
            ratio_temp                  = 0;
        }
    }

    /*直接对温度进行滤波*/
    timer4_trigger_callback();
    if (exti_triger_flag) {
        slid_holding_data[0].count = 0;
        slid_holding_data[1].count = 0;
        exti_triger_flag           = false;
    }
    switch (sys_para.filter.mode) {
        case PEAK_HOLDING:
            /*峰值保持*/
            temp[SENS_CH_SPEC_WB] =
                peak_holding_fliter(&slid_holding_data[SENS_CH_SPEC_WB], sys_para.filter.peak_holding_time, temp[SENS_CH_SPEC_WB]);
            temp[SENS_CH_SPEC_NB] =
                peak_holding_fliter(&slid_holding_data[SENS_CH_SPEC_NB], sys_para.filter.valley_holding_time, temp[SENS_CH_SPEC_NB]);
            ratio_temp = peak_holding_fliter(&slid_holding_data[2], sys_para.filter.valley_holding_time, ratio_temp);
            break;
        case VALLEY_HOLDING:
            /*谷值保持*/
            temp[SENS_CH_SPEC_WB] =
                valley_holding_fliter(&slid_holding_data[SENS_CH_SPEC_WB], sys_para.filter.peak_holding_time, temp[SENS_CH_SPEC_WB]);
            temp[SENS_CH_SPEC_NB] =
                valley_holding_fliter(&slid_holding_data[SENS_CH_SPEC_NB], sys_para.filter.valley_holding_time, temp[SENS_CH_SPEC_NB]);
            ratio_temp = valley_holding_fliter(&slid_holding_data[2], sys_para.filter.valley_holding_time, ratio_temp);
            break;
        default: break;
    }
    /* 根据测温模式输出对应温度 */
    for (int i = 0; i < SENS_CH_SPEC_CNT; i++) {
        sys_state.single_temp[i] = temp[i]; // 单色上报
    }
    sys_state.ratio_temp = ratio_temp;

    /*温度上报更新*/
    if (sys_para.temp.temp_mode == 0) {
        /* 模式0 */
        HANDLE_SINGLE_ERROR(sys_state.temp_out_range[0]);
        sys_state.target_temp = sys_state.single_temp[SENS_CH_SPEC_WB];
    } else if (sys_para.temp.temp_mode == 1) {
        /* 模式1 */
        HANDLE_SINGLE_ERROR(sys_state.temp_out_range[1]);
        sys_state.target_temp = sys_state.single_temp[SENS_CH_SPEC_NB];
    } else {
        HANDLE_SINGLE_ERROR(sys_state.temp_out_range[2]);
        sys_state.target_temp = sys_state.ratio_temp;
    }
}
#undef HANDLE_SINGLE_ERROR
/*
 * 功能描述：这个函数根据输入的电压值计算单色温度
 * 入参说明：data --- 指向光谱传感器数据
 * 返 回 值：计算得到的目标温度值
 */
static float get_single_temp(long long voltage, uint8_t ch) {
    double temp[SENS_CH_SPEC_CNT];
    double compes_volt[SENS_CH_SPEC_CNT];

    taskENTER_CRITICAL();

    compes_volt[ch] = voltage / 1000000.0f; /* 电压单位转为（V） */
    compes_volt[ch] =
        compes_volt[ch] / (sys_para.temp.emissivity * sys_para.temp.trans[ch] * sys_para.temp.trans_interior[ch]); /* 电压发射率、透射率补偿 */
    get_sin_coefficient(&coef_a[ch], &coef_b[ch], compes_volt[ch], ch);                                            /* 获取对数系数 */
    if (compes_volt[ch] > 0 && &coef_a[ch] != 0) {                                                                 /* 避免非法操作 */
        temp[ch] = 1 / (coef_a[ch] * log(compes_volt[ch]) + coef_b[ch]) - 273.15f;                                 /* 电压转换为温度（摄氏度） */
    } else {
        // dbg_error("Error: Invalid input at index %d\n", i);
    }
    taskEXIT_CRITICAL();

    return temp[ch];
}

/*
 * 功能描述：这个函数根据输入的电压值计算比色温度
 * 入参说明：data --- 指向光谱传感器数据
 * 返 回 值：计算得到的目标温度值
 */
static float get_ratio_temp(sens_spec_t *data) {
    double      compes_volt[SENS_CH_SPEC_CNT];
    double      ratio_value = 0;
    double      ratio_temp  = 0;
    algo_para_t algo_para   = sys_para.algo;

    taskENTER_CRITICAL();

    for (int i = 0; i < SENS_CH_SPEC_CNT; i++) {
        compes_volt[i] = data[i].volt_compensate /
                         (sys_para.temp.emissivity * sys_para.temp.trans[i] * sys_para.temp.trans_interior[i]); /* 电压发射率、透射率补偿 */
    }
    ratio_value = (double)compes_volt[SENS_CH_SPEC_WB] / (double)compes_volt[SENS_CH_SPEC_NB]; /* 宽带与窄带比值 */
    ratio_value /= sys_para.temp.slope;                                                        /* 坡度补偿 */
    ratio_value /= sys_para.temp.slope_interior;                                               /* 坡度补偿(内部) */

    ratio_value -= (algo_para.poly_para[0].subtrahend * 1000.0f);                             /* t-t0 */
                                                                                              // ratio_value = 1.013942017;//测试值 2400
    get_multi_coefficient(&ratio_temp, (float)ratio_value, algo_para.poly_para[0].poly_term); /* 获取多项式温度 */

    sys_state.ratio_temp_nerve = ratio2temp(ratio_value);

    taskEXIT_CRITICAL();

    return ratio_temp;
}

/*
 * 功能描述：这个函数根据输入的电压值计算目标温度
 * 入参说明：data --- 指向光谱传感器数据
 * 返 回 值：计算得到的目标温度值
 */
static void get_internal_temp(sens_ntc_t *data) {
    sys_state.internal_temp[SENS_CH_NTC_SPEC] = get_ntc_temp(get_ntc_res(data[SENS_CH_NTC_SPEC].volt_filter));
    sys_state.internal_temp[SENS_CH_NTC_TIA]  = get_ntc_temp(get_ntc_res(data[SENS_CH_NTC_TIA].volt_raw));
}

/*
 * 功能描述：这个函数用于传感器原始电压归一化处理
 * 入参说明：data --- 指向光谱传感器数据
 * 返 回 值：0 表示进行后续处理  -1 表示终止后续处理流程
 */
static int volt_normalization(sens_spec_t *data) {

    for (int i = 0; i < SENS_CH_SPEC_CNT; i++) {
        /* 电压异常则退出 */
        if (data[i].volt_raw < 0 || data[i].volt_raw > 3299999) {
            return -1;
        }
        gain_compes_factor[i] = (sys_state.now_gain[i] == GAIN_M)   ? sys_para.gain.k_lm[i]
                                : (sys_state.now_gain[i] == GAIN_H) ? sys_para.gain.k_mh[i] * sys_para.gain.k_lm[i]
                                                                    : 1;
        data[i].volt_norm     = data[i].volt_raw * gain_compes_factor[i];
    }
    return 0;
}

/*
 * 功能描述：这个函数用于初始化切档相关状态参数
 * 入参说明：无
 * 返 回 值：无
 */
void gain_status_init(void) {
    for (int i = 0; i < SENS_CH_SPEC_CNT; i++) {
        gain_sw_data[i].history_volt.buffer_size = 20;
        gain_sw_data[i].history_volt.count       = 0;
        gain_sw_data[i].history_volt.front       = 0;
        gain_sw_data[i].gl_to_gm                 = 0;
        gain_sw_data[i].gm_to_gh                 = 0;
        gain_sw_data[i].gh_to_gm                 = 0;
        gain_sw_data[i].gm_to_gl                 = 0;
    }
}

/*
 * 功能描述：这个函数对切档后的异常值，根据斜率做插值处理
 * 入参说明：data --- 指向光谱传感器数据
 * 返 回 值：无
 */
static void gain_temp_insert(sens_spec_t *data) {
    for (int i = 0; i < SENS_CH_SPEC_CNT; i++) {
        if (gain_sw_data[i].gl_to_gm == 0 && gain_sw_data[i].gm_to_gh == 0 && gain_sw_data[i].gh_to_gm == 0 && gain_sw_data[i].gm_to_gl == 0) {
            continue;
        }

        /* 做插值处理 */
        if (gain_sw_data[i].gl_to_gm != 0) {
            gain_sw_data[i].gl_to_gm--;
        } else if (gain_sw_data[i].gm_to_gh != 0) {
            gain_sw_data[i].gm_to_gh--;
        } else if (gain_sw_data[i].gh_to_gm != 0) {
            gain_sw_data[i].gh_to_gm--;
        } else if (gain_sw_data[i].gm_to_gl != 0) {
            gain_sw_data[i].gm_to_gl--;
        }
        uint8_t tail      = (gain_sw_data[i].history_volt.front + 1) % gain_sw_data[i].history_volt.buffer_size;
        data[i].volt_norm = slid_remove_volt[i].buff[(slid_remove_volt[i].front - 1 + FILTER_LEN_MAX) % FILTER_LEN_MAX] +
                            (gain_sw_data[i].history_volt.buff[gain_sw_data[i].history_volt.front] - gain_sw_data[i].history_volt.buff[tail]) /
                                (gain_sw_data[i].history_volt.count - 1.0f);
        //        data[i].volt_norm = data[i].last_volt_filter;
    }
}

/*
 * 功能描述：电压去极值后均值滤波处理
 * 入参说明：data --- 指向光谱传感器数据
 * 返 回 值：无
 */
static void volt_remove_average_filter(sens_spec_t *data, sens_ntc_t *ntc_data) {
    uint16_t buf_size;
    if (sys_para.filter.temp_filter_buf_size < FILTER_LEN_MAX) {
        buf_size = sys_para.filter.temp_filter_buf_size;
    } else {
        buf_size = 25;
    }
    for (int i = 0; i < SENS_CH_SPEC_CNT; i++) {
        /*平均滤波*/
        data[i].volt_filter = sliding_remove_average(&slid_remove_volt[i], buf_size, data[i].volt_norm, filter_buffer[i]);

        /* 更新历史电压数据 */
        if (gain_sw_data[i].history_volt.count >= gain_sw_data[i].history_volt.buffer_size) {
            gain_sw_data[i].history_volt.front = (gain_sw_data[i].history_volt.front + 1) % gain_sw_data[i].history_volt.buffer_size;
            gain_sw_data[i].history_volt.buff[gain_sw_data[i].history_volt.front] = data[i].volt_filter;
        } else {
            gain_sw_data[i].history_volt.buff[gain_sw_data[i].history_volt.front + gain_sw_data[i].history_volt.count] = data[i].volt_filter;
            gain_sw_data[i].history_volt.count++;
        }
    }
    ntc_data[SENS_CH_NTC_SPEC].volt_filter =
        sliding_remove_average(&slid_remove_ntc_volt, NTC_TEMP_AVER_LEN, ntc_data[SENS_CH_NTC_SPEC].volt_raw, filter_buffer_NTC[SENS_CH_NTC_SPEC]);
}

/*
 * 功能描述：此函数根据不同的条件获取两个系数 coef1 和 coef2
 * 入参说明：coef1 --- 指向第一个系数的指针
 *           coef2 --- 指向第二个系数的指针
 *           compes_volt --- 补偿电压值
 *           spec_num --- 光谱通道号
 * 返 回 值：无
 */
static void get_sin_coefficient(double *coef1, double *coef2, float compes_volt, uint8_t spec_num) {
    uint8_t index               = 0;
    float   seg_threshold_value = sys_para.algo.log_para[spec_num][index].threshold;

    if (0 == seg_threshold_value || compes_volt < seg_threshold_value) {
        index = 0;
    } else {
        index = 1;
    }
    /* 根据spec_num和sinindex设定系数 */
    *coef1 = sys_para.algo.log_para[spec_num][index].coef[0].base * pow(10, sys_para.algo.log_para[spec_num][index].coef[0].index);
    *coef2 = sys_para.algo.log_para[spec_num][index].coef[1].base * pow(10, sys_para.algo.log_para[spec_num][index].coef[1].index);
}

/*
 * 功能描述：根据比值和多项式阶数计算多项式温度值
 * 入参说明：coef --- 指向最终温度结果的指针
 *           ratio_value --- 电压比值（已做补偿）
 *           poly_num --- 多项式的阶数
 * 返 回 值：无
 */
static void get_multi_coefficient(double *coef, float ratio_value, uint8_t poly_num) {
    double      midIndexValue;
    double      midValue;
    algo_para_t algo_para = sys_para.algo;

    *coef = 0; /* 初始化系数总值 */
    for (int i = 0; i < poly_num; i++) {
        midIndexValue = pow((double)ratio_value, i);          /* (t-t0)^i */
        midIndexValue *= algo_para.poly_para[0].coef[i].base; /* ai * (t-t0)^i */
        midValue = algo_para.poly_para[0].coef[i].index;      /* 获取幂 */
        midIndexValue *= pow(10, midValue);                   /* ai * 10^m * (t-t0)^i */
        *coef += midIndexValue;                               /* 累加到最终结果 */
    }
}

/*
 * 功能描述：档位切换逻辑处理
 * 入参说明：data --- 传感器数据信息指针
 * 返 回 值：0 表示进行后续处理  -1 表示终止后续处理流程
 */
static int gain_switch_process(sens_spec_t *data) {
    static uint8_t bsp_gain[SENS_CH_SPEC_CNT];

    ENTER_CRITICAL();
    for (int i = 0; i < SENS_CH_SPEC_CNT; i++) {
        bsp_gain[i] = sys_state.now_gain[i]; /* 保存当前档位 */
        tiag_switch(i, data[i].volt_raw, sys_state.now_gain[i]);
        if (bsp_gain[i] != sys_state.now_gain[i]) {
            gain_sw_data[i].flag = 1;
        }
        if (gain_sw_data[i].flag) {
            gain_sw_data[i].flag     = 0;
            data[i].last_volt_filter = data[i].volt_filter;
            if (bsp_gain[i] < sys_state.now_gain[i]) {
                if (bsp_gain[i] == GAIN_L) {
                    gain_sw_data[i].gl_to_gm = sys_para.gain.volt_discard;
                } else if (bsp_gain[i] == GAIN_M) {
                    gain_sw_data[i].gm_to_gh = sys_para.gain.volt_discard;
                }
            } else if ((bsp_gain[i] > sys_state.now_gain[i])) {
                if (bsp_gain[i] == GAIN_M) {
                    gain_sw_data[i].gm_to_gl = sys_para.gain.volt_discard;
                } else if (bsp_gain[i] == GAIN_H) {
                    gain_sw_data[i].gh_to_gm = sys_para.gain.volt_discard;
                }
            }

            //            mem_set(slid_aver_temp, sizeof(slid_aver_t) * SENS_CH_SPEC_CNT, 0x00);
            //            mem_set(slid_aver_volt, sizeof(slid_aver_t) * SENS_CH_SPEC_CNT, 0x00);
            EXIT_CRITICAL();
            return -1;
        }
    }
    EXIT_CRITICAL();

    return 0;
}
// 查表法切档过渡100
float smooth_table_100[] = {2.9125283e-04, 1.1560470e-03, 2.5852596e-03, 4.5696111e-03, 7.0996676e-03, 1.0165843e-02, 1.3758399e-02, 1.7867454e-02,
                            2.2482975e-02, 2.7594790e-02, 3.3192584e-02, 3.9265906e-02, 4.5804167e-02, 5.2796646e-02, 6.0232493e-02, 6.8100728e-02,
                            7.6390249e-02, 8.5089831e-02, 9.4188131e-02, 1.0367369e-01, 1.1353493e-01, 1.2376018e-01, 1.3433765e-01, 1.4525545e-01,
                            1.5650158e-01, 1.6806397e-01, 1.7993043e-01, 1.9208869e-01, 2.0452641e-01, 2.1723113e-01, 2.3019034e-01, 2.4339146e-01,
                            2.5682182e-01, 2.7046869e-01, 2.8431927e-01, 2.9836070e-01, 3.1258008e-01, 3.2696443e-01, 3.4150074e-01, 3.5617594e-01,
                            3.7097695e-01, 3.8589061e-01, 4.0090374e-01, 4.1600316e-01, 4.3117563e-01, 4.4640789e-01, 4.6168667e-01, 4.7699870e-01,
                            4.9233068e-01, 5.0766932e-01, 5.2300130e-01, 5.3831333e-01, 5.5359211e-01, 5.6882437e-01, 5.8399684e-01, 5.9909626e-01,
                            6.1410939e-01, 6.2902305e-01, 6.4382406e-01, 6.5849926e-01, 6.7303557e-01, 6.8741992e-01, 7.0163930e-01, 7.1568073e-01,
                            7.2953131e-01, 7.4317818e-01, 7.5660854e-01, 7.6980966e-01, 7.8276887e-01, 7.9547359e-01, 8.0791131e-01, 8.2006957e-01,
                            8.3193603e-01, 8.4349842e-01, 8.5474455e-01, 8.6566235e-01, 8.7623982e-01, 8.8646507e-01, 8.9632631e-01, 9.0581187e-01,
                            9.1491017e-01, 9.2360975e-01, 9.3189927e-01, 9.3976751e-01, 9.4720335e-01, 9.5419583e-01, 9.6073409e-01, 9.6680742e-01,
                            9.7240521e-01, 9.7751703e-01, 9.8213255e-01, 9.8624160e-01, 9.8983416e-01, 9.9290033e-01, 9.9543039e-01, 9.9741474e-01,
                            9.9884395e-01, 9.9970875e-01, 1.0000000e+00, 9.9970875e-01};
// 查表法切档过渡200
float smooth_table_200[] = {
    7.2055957e-05, 2.8712900e-04, 6.4411443e-04, 1.1418978e-03, 1.7793548e-03, 2.5553515e-03, 3.4687445e-03, 4.5183806e-03, 5.7030973e-03,
    7.0217226e-03, 8.4730752e-03, 1.0055964e-02, 1.1769190e-02, 1.3611544e-02, 1.5581807e-02, 1.7678752e-02, 1.9901145e-02, 2.2247738e-02,
    2.4717280e-02, 2.7308507e-02, 3.0020148e-02, 3.2850925e-02, 3.5799548e-02, 3.8864721e-02, 4.2045140e-02, 4.5339492e-02, 4.8746455e-02,
    5.2264700e-02, 5.5892890e-02, 5.9629679e-02, 6.3473716e-02, 6.7423640e-02, 7.1478081e-02, 7.5635664e-02, 7.9895007e-02, 8.4254718e-02,
    8.8713399e-02, 9.3269646e-02, 9.7922047e-02, 1.0266918e-01, 1.0750962e-01, 1.1244194e-01, 1.1746470e-01, 1.2257644e-01, 1.2777572e-01,
    1.3306108e-01, 1.3843106e-01, 1.4388417e-01, 1.4941895e-01, 1.5503391e-01, 1.6072756e-01, 1.6649841e-01, 1.7234495e-01, 1.7826568e-01,
    1.8425908e-01, 1.9032365e-01, 1.9645785e-01, 2.0266016e-01, 2.0892905e-01, 2.1526298e-01, 2.2166040e-01, 2.2811977e-01, 2.3463954e-01,
    2.4121816e-01, 2.4785405e-01, 2.5454566e-01, 2.6129141e-01, 2.6808974e-01, 2.7493907e-01, 2.8183781e-01, 2.8878438e-01, 2.9577720e-01,
    3.0281468e-01, 3.0989521e-01, 3.1701720e-01, 3.2417905e-01, 3.3137915e-01, 3.3861591e-01, 3.4588770e-01, 3.5319292e-01, 3.6052995e-01,
    3.6789718e-01, 3.7529298e-01, 3.8271573e-01, 3.9016382e-01, 3.9763561e-01, 4.0512949e-01, 4.1264381e-01, 4.2017695e-01, 4.2772727e-01,
    4.3529316e-01, 4.4287296e-01, 4.5046505e-01, 4.5806778e-01, 4.6567953e-01, 4.7329865e-01, 4.8092351e-01, 4.8855247e-01, 4.9618388e-01,
    5.0381612e-01, 5.1144753e-01, 5.1907649e-01, 5.2670135e-01, 5.3432047e-01, 5.4193222e-01, 5.4953495e-01, 5.5712704e-01, 5.6470684e-01,
    5.7227273e-01, 5.7982305e-01, 5.8735619e-01, 5.9487051e-01, 6.0236439e-01, 6.0983618e-01, 6.1728427e-01, 6.2470702e-01, 6.3210282e-01,
    6.3947005e-01, 6.4680708e-01, 6.5411230e-01, 6.6138409e-01, 6.6862085e-01, 6.7582095e-01, 6.8298280e-01, 6.9010479e-01, 6.9718532e-01,
    7.0422280e-01, 7.1121562e-01, 7.1816219e-01, 7.2506093e-01, 7.3191026e-01, 7.3870859e-01, 7.4545434e-01, 7.5214595e-01, 7.5878184e-01,
    7.6536046e-01, 7.7188023e-01, 7.7833960e-01, 7.8473702e-01, 7.9107095e-01, 7.9733984e-01, 8.0354215e-01, 8.0967635e-01, 8.1574092e-01,
    8.2173432e-01, 8.2765505e-01, 8.3350159e-01, 8.3927244e-01, 8.4496609e-01, 8.5058105e-01, 8.5611583e-01, 8.6156894e-01, 8.6693892e-01,
    8.7222428e-01, 8.7742356e-01, 8.8253530e-01, 8.8755806e-01, 8.9249038e-01, 8.9733082e-01, 9.0207795e-01, 9.0673035e-01, 9.1128660e-01,
    9.1574528e-01, 9.2010499e-01, 9.2436434e-01, 9.2852192e-01, 9.3257636e-01, 9.3652628e-01, 9.4037032e-01, 9.4410711e-01, 9.4773530e-01,
    9.5125355e-01, 9.5466051e-01, 9.5795486e-01, 9.6113528e-01, 9.6420045e-01, 9.6714908e-01, 9.6997985e-01, 9.7269149e-01, 9.7528272e-01,
    9.7775226e-01, 9.8009886e-01, 9.8232125e-01, 9.8441819e-01, 9.8638846e-01, 9.8823081e-01, 9.8994404e-01, 9.9152692e-01, 9.9297828e-01,
    9.9429690e-01, 9.9548162e-01, 9.9653126e-01, 9.9744465e-01, 9.9822065e-01, 9.9885810e-01, 9.9935589e-01, 9.9971287e-01, 9.9992794e-01,
    1.0000000e+00, 9.9992794e-01};
/*
 *@brief：平滑切档温度阶跃
 *@smoothRate：平滑率
 *@smoothRate：平滑数
 *@currentTemp：当前温度
 */
float switch_temp_smooth(float smoothRate, int32_t smoothCount, float currentTemp, int32_t spect) {
    float smoothTemp = 0;

    if (abs((int)(gain_sw_data[spect].smooth_lastTemp - currentTemp)) > 70) {
        gain_sw_data[spect].smooth_sta   = 0; // 平滑结束
        gain_sw_data[spect].smooth_count = 0;
        smoothTemp                       = currentTemp;
        return smoothTemp;
    }

    // 一次平滑算法
    //    smoothTemp = smoothRate*adg633_obj.switch_smooth_lastTemp + (1-smoothRate)*currentTemp;
    // 查表平滑算法
    smoothTemp = gain_sw_data[spect].smooth_lastTemp +
                 (currentTemp - gain_sw_data[spect].smooth_lastTemp) * smooth_table_100[gain_sw_data[spect].smooth_count] / 8;
    gain_sw_data[spect].smooth_lastTemp = smoothTemp;
    gain_sw_data[spect].smooth_count++;

    if (gain_sw_data[spect].smooth_count > smoothCount) {
        gain_sw_data[spect].smooth_count = 0;
        gain_sw_data[spect].smooth_sta   = 0; // 平滑结束
    }

    return smoothTemp;
}

/*
 * 功能描述：根据目标温度校准透射率或坡度（单色或比色）
 * 入参说明：无
 * 返 回 值：0成功，-1失败
 */
int calibrate_temperature(void) {
    int     ret  = -1;
    uint8_t mode = sys_para.temp.temp_mode;

    float   current_temp = 0.0f;
    float   target_temp  = sys_state.temp_calibration;
    float   tolerance    = 0.1f; /* 允许误差（°C） */
    uint8_t max_iter     = 50;   /* 最大迭代次数 */
    uint8_t iter         = 0;

    float   min_value, max_value, mid_value;
    float   voltage;
    uint8_t ch;

    if (mode == 0) {
        ch = SENS_CH_SPEC_WB;
    } else if (mode == 1) {
        ch = SENS_CH_SPEC_NB;
    }

    if (mode == 0 || mode == 1) {
        voltage = sens_spec[ch].volt_filter;
        if (voltage <= 0) {
            /* 防止非法电压 */
            return ret;
        }
        min_value = 0.01f; /* 最小透射率 */
        max_value = 10.0f; /* 最大透射率 */
        mid_value = sys_para.temp.trans[ch];
    } else if (mode == 2) {
        /* 比色模式，无需单独电压，只用已有数据 */
        min_value = 0.01f; /* 最小坡度 */
        max_value = 10.0f; /* 最大坡度，可以根据需要调整 */
        mid_value = sys_para.temp.slope;
    } else {
        /* 非法mode */
        return ret;
    }

    taskENTER_CRITICAL();

    while (iter++ < max_iter) {
        /* 设置当前中点值 */
        if (mode == 0 || mode == 1) {
            sys_para.temp.trans[ch] = mid_value;
            current_temp            = get_single_temp(voltage, ch);
        } else if (mode == 2) {
            sys_para.temp.slope = mid_value;
            current_temp        = get_ratio_temp(sens_spec); /* 注意这里直接传sens_spec */
        }

        float diff = current_temp - target_temp;

        if (fabs(diff) < tolerance) {
            /* 差异足够小，校准成功 */
            ret = 0;
            pmm_save_group(PARA_TEMP);
            break;
        }

        /* 二分法调整范围 */
        if (mode == 0 || mode == 1) {
            if (diff > 0) {
                min_value = mid_value;
            } else {
                max_value = mid_value;
            }
        } else if (mode == 2) {
            if (diff < 0) {
                min_value = mid_value;
            } else {
                max_value = mid_value;
            }
        }

        /* 取新的中点 */
        mid_value = (min_value + max_value) / 2.0f;

        /* 防止越界 */
        if (mid_value <= min_value) {
            mid_value = min_value;
        } else if (mid_value >= max_value) {
            mid_value = max_value;
        }
    }

    taskEXIT_CRITICAL();
    return ret;
}

/*
 * 功能描述：两点温度补偿（单色/比色通用，按模式选择）
 * 入参说明：raw_temp 原始温度
 * 返 回 值：补偿后的温度
 */
static float temp_two_point_calibrate(float raw_temp, int mode) {
    //    float k = 1.0f;
    //    float b = 0.0f;
    if (mode >= 3) {
        mode = 2; /* 超范围保护，默认用比色 */
    }
    /* 是否启动计算 */
    if (sys_state.two_calib_open == 1) {
        sys_state.two_calib_open = 0;
        /* 读取参考点，按照当前模式选择 */
        float calib_low_set   = sys_state.calib_low_set[mode];
        float calib_low_real  = sys_state.calib_low_real[mode];
        float calib_high_set  = sys_state.calib_high_set[mode];
        float calib_high_real = sys_state.calib_high_real[mode];

        /* 防止除零错误 */
        if ((calib_high_real - calib_low_real) != 0.0f) {
            /* 计算补偿系数增益k和偏置b */
            sys_para.temp.temp_calib_gain[mode]   = (calib_high_set - calib_low_set) / (calib_high_real - calib_low_real);
            sys_para.temp.temp_calib_offset[mode] = calib_high_set - sys_para.temp.temp_calib_gain[mode] * calib_high_real;
        }
    }

    /* 应用补偿 */
    return (sys_para.temp.temp_calib_gain[mode] * raw_temp + sys_para.temp.temp_calib_offset[mode]);
}

/*
 * 功能描述：内部温度补偿能量
 * 入参说明：raw_value 原始能量
 * 返 回 值：补偿后的能量
 */
static long long internal_temp_power_compensate(long long raw_value, int spec) {

    double    coef_K;
    double    in_value;
    long long cmp_value;

    in_value = raw_value / 1000000.0f;
    if (spec == 0) {
        coef_K = -2.06134814E-06 * pow(in_value, 2) - 1.10086024E-03 * in_value - 8.39013272E-04;
    } else if (spec == 1) {
        coef_K = -2.49380851E-06 * pow(in_value, 2) - 8.71315941E-04 * in_value - 9.05925763E-04;
    } else {
        coef_K = 0;
    }

    double midIndexValue = coef_K * (sys_state.internal_temp[SENS_CH_NTC_TIA] - 45.00f);
    cmp_value            = (in_value - midIndexValue) * 1000000;

    return cmp_value;
}

/*
 * 功能描述：内部温度补偿
 * 入参说明：data --- 传感器数据信息指针
 * 返 回 值：补偿后的温度
 */
static void internal_temp_calibrate(sens_spec_t *data) {

    for (int i = 0; i < SENS_CH_SPEC_CNT; i++) {
        /*是否使能补偿*/
        if (sys_para.temp.ambient_comp_ctl == 1) {
            data[i].volt_compensate = internal_temp_power_compensate(data[i].volt_filter, i);
        } else {
            data[i].volt_compensate = data[i].volt_filter;
        }
        sys_state.output_volt[i] = data[i].volt_compensate;
    }
}

float ratio2temp(float ratio) {
    ////权值调整
    // for (int i = 0; i < 64; ++i) {
    //  effective_w1[i] = fc1_weight[i] / std_ratio;
    //  effective_b1[i] = fc1_bias[i] - (fc1_weight[i] * mean_ratio) / std_ratio;
    //  effective_w2[i] = fc2_weight[i] * std_temp;
    // }
    // effective_b2 = fc2_bias * std_temp + mean_temp;

    // 第一次线性变换
    float fc1_out[64];
    for (int i = 0; i < 64; ++i) {
        // fc1_out[i] = effective_w1[i] * ratio + effective_b1[i];
        fc1_out[i] = sys_para.algo.network_para.effective_w1[i] * ratio + sys_para.algo.network_para.effective_b1[i];
    }
    // ReLU
    float fc1_relu[64];
    for (int i = 0; i < 64; ++i) {
        fc1_relu[i] = fc1_out[i] > 0 ? fc1_out[i] : 0;
    }
    // 第二次线性变换
    float fc2_out = 0.0;
    for (int i = 0; i < 64; ++i) {
        // fc2_out += effective_w2[i] * fc1_relu[i];
        fc2_out += sys_para.algo.network_para.effective_w2[i] * fc1_relu[i];
    }
    // fc2_out += effective_b2;
    fc2_out += sys_para.algo.network_para.effective_b2[0];
    return fc2_out;
}

void timer4_handler() {
    UBaseType_t isr_status = taskENTER_CRITICAL_FROM_ISR();
    if (timer_flag_get(TIMER4, TIMER_FLAG_UP) == SET) {
        timer_flag_clear(TIMER4, TIMER_FLAG_UP);
        timer4_trigger_flag = true;
    }
    taskEXIT_CRITICAL_FROM_ISR(isr_status);
}
void timer4_init() {
    /* 定时器4（用于进行峰谷值保持功能的时间计时），当前设置100ms触发   */
    int timer_clk = gd32_timer_clk_freq(TIMER4);
    gd32_timer_init(TIMER4, timer_clk / 4000, 4 * 100); // 100表示100ms触发一次
    gd32_setup_isr(TIMER4_IRQn, timer4_handler, 0);
    gd32_setup_irq(TIMER4_IRQn, 15);

    timer_enable(TIMER4);
    timer_event_software_generate(TIMER4, TIMER_EVENT_SRC_UPG);
    timer_flag_clear(TIMER4, TIMER_FLAG_UP);
    timer_interrupt_enable(TIMER4, TIMER_INT_UP);
}
void timer4_trigger_callback() {
    if (timer4_trigger_flag) {
        timer4_trigger_flag = false;
        for (int i = 0; i < SENS_CH_SPEC_CNT + 1; i++) {
            switch (sys_para.filter.mode) {
                    //                case AVERAGE_FILTER:
                    /*采用的是固定个数进行滤波，不需要定时器处理*/
                    //                    break;
                case PEAK_HOLDING:
                    /*峰值保持*/
                    if (slid_holding_data[i].count == sys_para.filter.peak_holding_time * 10) {
                        slid_holding_data[i].count = 0;
                        // dbg_info("%d---timer:holding time arrive timer4_dbg info\r\n", i);
                    } else if (slid_holding_data[i].count != 0) {
                        slid_holding_data[i].count++;
                    }
                    break;
                case VALLEY_HOLDING:
                    /*谷值保持*/
                    if (slid_holding_data[i].count == sys_para.filter.valley_holding_time * 10) {
                        slid_holding_data[i].count = 0;
                        // dbg_info("%d---timer:holding time arrive timer4_dbg info\r\n", i);
                    } else if (slid_holding_data[i].count != 0) {
                        slid_holding_data[i].count++;
                    }
                    break;
                default: break;
            }
        }
    }
}
