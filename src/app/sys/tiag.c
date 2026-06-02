/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：tiag.c
 * 文件描述：跨阻放大器增益档位控制驱动代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/03/20 陈军  初始版本
 */

#include "tiag.h"
extern int warm_start;
typedef enum { UP_L = 0, DOWN_M, UP_M, DOWN_H } gain_flag_t;

/* 切挡计数 */
typedef struct {
    uint32_t up_gl;
    uint32_t up_gm;
    uint32_t down_gm;
    uint32_t down_gh;
} gain_cnt_t;

static gain_cnt_t gain_cnt[SENS_CH_SPEC_CNT];

static void tiag_cmd_switch(uint8_t ch_spec, uint8_t gain);
static void tiag_auto_switch(uint8_t ch_spec, uint32_t volt, uint8_t gain);
static void gain_set(uint8_t ch_spec, uint8_t gain);
static void gain_count_init(uint8_t ch_spec);
static void gain_count_reset(uint8_t ch_spec, gain_flag_t gain_flag);

/*
 * 功能描述：跨阻部分初始化
 * 入参说明：无
 * 返 回 值：无
 */
void tiag_init(void) {
    /* 模拟开关引脚初始化 */
    was_pin_init();
    nas_pin_init();

    /* 默认自动切换跨阻 */
    ENTER_CRITICAL();
    sys_state.tiag_switch_mode = TIAG_AUTO_MODE;
    // sys_para.gain.cnt_thod     = 1;

    /* 跨阻档位初始化默认为高温段 */
    if (warm_start) {
        gain_set(SENS_CH_SPEC_NB, sys_state.now_gain[SENS_CH_SPEC_NB]);
        gain_set(SENS_CH_SPEC_WB, sys_state.now_gain[SENS_CH_SPEC_WB]);
    } else {
        gain_set(SENS_CH_SPEC_NB, GAIN_H);
        gain_set(SENS_CH_SPEC_WB, GAIN_H);
    }
    EXIT_CRITICAL();

    /* 档位切换检测计数初始化 */
    gain_count_init(SENS_CH_SPEC_NB);
    gain_count_init(SENS_CH_SPEC_WB);
}

/*
 * 功能描述：此函数用于跨阻档位切换
 * 入参说明：ch_spec --- 宽带/宽带通道
 *           volt --- 当前采集宽带/宽带通道光谱电压值
 *           gain --- 跨阻档位（增益）
 * 返 回 值：无
 * 备    注：系统默认为自动切换模式，可以临时更改为命令切换，复位设备后切回自动模式
 */
void tiag_switch(uint8_t ch_spec, uint32_t volt, uint8_t gain) {
    if (sys_state.tiag_switch_mode == TIAG_AUTO_MODE) { /* 自动切换模式（默认） */
        tiag_auto_switch(ch_spec, volt, gain);
    } else { /* 命令切换模式（调试） */
        tiag_cmd_switch(ch_spec, gain);
    }
}

/*
 * 功能描述：命令切换模式的逻辑单元
 * 入参说明：ch_spec --- 宽带/宽带通道
 *           gain --- 跨阻档位（增益）
 * 返 回 值：无
 */
static void tiag_cmd_switch(uint8_t ch_spec, uint8_t gain) {
    if (ch_spec >= SENS_CH_SPEC_CNT) return;
    if (gain >= GAIN_CNT) return;

    /* 设置档位 */
    gain_set(ch_spec, gain);
}

/*
 * 功能描述：跨阻自动切换模式（跨阻档位随光谱传感器采集电压变化自适应）
 * 入参说明：ch_spec --- 宽带/宽带通道
 *           volt --- 当前采集宽带/宽带通道光谱电压值
 *           gain --- 跨阻档位（增益）
 * 返 回 值：无
 */
static void tiag_auto_switch(uint8_t ch_spec, uint32_t volt, uint8_t gain) {
    if (ch_spec >= SENS_CH_SPEC_CNT) return;
    if (gain >= GAIN_CNT) return;

    switch (gain) {
        case GAIN_L:
            if (volt > sys_para.gain.upper[ch_spec]) {
                gain_cnt[ch_spec].up_gl++;
                gain_count_reset(ch_spec, UP_L);
                if (gain_cnt[ch_spec].up_gl >= sys_para.gain.cnt_thod) {
                    gain_set(ch_spec, GAIN_M);
                    gain_cnt[ch_spec].up_gl = 0;
                }
            }
            break;

        case GAIN_M:
            if (volt > sys_para.gain.upper[ch_spec]) {
                gain_cnt[ch_spec].up_gm++;
                gain_count_reset(ch_spec, UP_M);
                if (gain_cnt[ch_spec].up_gm >= sys_para.gain.cnt_thod) {
                    gain_set(ch_spec, GAIN_H);
                    gain_cnt[ch_spec].up_gm = 0;
                }
            } else if (volt < sys_para.gain.lowerml[ch_spec] / sys_para.gain.k_lm[ch_spec]) {
                gain_cnt[ch_spec].down_gm++;
                gain_count_reset(ch_spec, DOWN_M);
                if (gain_cnt[ch_spec].down_gm >= sys_para.gain.cnt_thod) {
                    gain_set(ch_spec, GAIN_L);
                    gain_cnt[ch_spec].down_gm = 0;
                }
            }
            break;

        case GAIN_H:
            if (volt < sys_para.gain.lowerhm[ch_spec] / sys_para.gain.k_mh[ch_spec]) {
                gain_cnt[ch_spec].down_gh++;
                gain_count_reset(ch_spec, DOWN_H);
                if (gain_cnt[ch_spec].down_gh >= sys_para.gain.cnt_thod) {
                    gain_set(ch_spec, GAIN_M);
                    gain_cnt[ch_spec].down_gh = 0;
                }
            }
            break;

        default: break;
    }
}

/*
 * 功能描述：设置指定光谱通道传感器跨阻的档位（增益）
 * 入参说明：ch_spec --- 宽带/宽带通道
 *           gain --- 跨阻档位（增益）
 * 返 回 值：无
 */
static void gain_set(uint8_t ch_spec, uint8_t gain) {
    if (ch_spec == SENS_CH_SPEC_NB) /* 窄带通道 */
    {
        switch (gain) {
            case GAIN_L: gain_nb_low(); break;
            case GAIN_M: gain_nb_middle(); break;
            case GAIN_H: gain_nb_high(); break;
            default: break;
        }
    } else if (ch_spec == SENS_CH_SPEC_WB) /* 宽带通道 */
    {
        switch (gain) {
            case GAIN_L: gain_wb_low(); break;
            case GAIN_M: gain_wb_middle(); break;
            case GAIN_H: gain_wb_high(); break;
            default: break;
        }
    }

    ENTER_CRITICAL();
    sys_state.now_gain[ch_spec] = gain;
    EXIT_CRITICAL();
}

/*
 * 功能描述：该函数为档位切换检测计数初始化
 * 入参说明：ch_spec --- 宽带/宽带通道
 * 返 回 值：无
 */
static void gain_count_init(uint8_t ch_spec) {
    gain_cnt[ch_spec].up_gl   = 0;
    gain_cnt[ch_spec].down_gm = 0;
    gain_cnt[ch_spec].up_gm   = 0;
    gain_cnt[ch_spec].down_gh = 0;
}

/*
 * 功能描述：档位切换检测非当前计数flag之外的清零
 * 入参说明：ch_spec --- 宽带/宽带通道
 *           gain_flag --- 各挡位对应的枚举值
 * 返 回 值：无
 */
static void gain_count_reset(uint8_t ch_spec, gain_flag_t gain_flag) {
    switch (gain_flag) {
        case UP_L:
            gain_cnt[ch_spec].down_gm = 0;
            gain_cnt[ch_spec].up_gm   = 0;
            gain_cnt[ch_spec].down_gh = 0;
            break;
        case DOWN_M:
            gain_cnt[ch_spec].up_gl   = 0;
            gain_cnt[ch_spec].up_gm   = 0;
            gain_cnt[ch_spec].down_gh = 0;
            break;
        case UP_M:
            gain_cnt[ch_spec].up_gl   = 0;
            gain_cnt[ch_spec].down_gm = 0;
            gain_cnt[ch_spec].down_gh = 0;
            break;
        case DOWN_H:
            gain_cnt[ch_spec].up_gl   = 0;
            gain_cnt[ch_spec].down_gm = 0;
            gain_cnt[ch_spec].up_gm   = 0;
            break;
        default: break;
    }
}
