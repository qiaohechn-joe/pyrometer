/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：analogout.c
 * 文件描述：模拟输出业务驱动代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 张韩 陈军  初始版本
 */

#include "analogout.h"
#include "ltc2606.h"
#include "system.h"

static ana_status_t ana_status;

static float anolog_temp_convert(float temp);
static int   analog_write_volt_lock(float volt);

/*
 * 功能描述：模拟输出初始化
 * 入参说明：无
 * 返 回 值：0 -- 成功，其它 -- 失败
 */
int anolog_out_init(void) {
    /* 初始化LTC2606 */
    ltc2606_init(LTC2606_I2C_BUS);

    ENTER_CRITICAL();
    /* 默认自动控制输出 */
    sys_state.ana_curt_value = ANA_OUT_AUTO;
    sys_state.ana_volt_value = ANA_OUT_AUTO;
    /* 模拟温度上下限值 */
    ana_status.temp_lower = (int)(sys_para.ana.lower * 100);
    ana_status.temp_upper = (int)(sys_para.ana.upper * 100);
    EXIT_CRITICAL();

    /* 模拟输出阈值范围 */
    ana_threshold_range();

    /* 两点校准原理得出校准参数 */
    if (sys_para.ana.type != ANA_0_10V) {
        get_current_calib_para();
    } else {
        get_voltage_calib_para();
    }

    return 0;
}
extern ErrorStatus_t err_table[];
/*
 * 功能描述：通过自动/手动的方式输出模拟电流/电压
 * 入参说明：无
 * 返 回 值：无
 */
void anolog_output(void) {
    float f_value;
    if (sys_para.ana.type != ANA_0_10V) {
        if (sys_state.ana_curt_value == ANA_OUT_AUTO) {
            if (sys_state.ana_temp_convert_flag == 1) {
                /*AT 模拟温度输出，掉电恢复*/
                f_value = anolog_temp_convert(sys_state.target_temp);

            } else {
                /*根据错误状态模拟输出*/
                int err_index = get_highest_error_index();
                if (err_index >= 0 && err_index < ERROR_ALL_CNT) {
                    if (sys_para.ana.type == ANA_4_20MA) {
                        memcpy(&f_value, err_table[err_index].analog_output[1], sizeof(float));
                        // f_value = *p1;
                    } else {
                        memcpy(&f_value, err_table[err_index].analog_output[0], sizeof(float));
                        // f_value = *p2;
                    }
                } else {
                    /*无错误正常输出*/
                    f_value = anolog_temp_convert(sys_state.target_temp);
                }
            }
            f_value *= 1000000.0f;
            f_value = (f_value - ana_status.current_b) / ana_status.current_a * ANA_ITOV_COEF;
        } else {
            f_value = (float)sys_state.ana_curt_value * ANA_ITOV_COEF;
        }
    } else {
        if (sys_state.ana_volt_value == ANA_OUT_AUTO) {
            f_value = anolog_temp_convert(sys_state.target_temp);
            f_value *= 1000000.0f;
            f_value = (f_value - ana_status.voltage_b) / ana_status.voltage_a * ANA_VTOV_COEF;
        } else {
            f_value = (float)sys_state.ana_volt_value * ANA_VTOV_COEF;
        }
    }

    ENTER_CRITICAL();
    if (f_value > 5000000.0f) {
        sys_state.dev_error[DEV_ERR_ANA_OUTRANGE] = DEV_ST_ERR;
    } else {
        sys_state.dev_error[DEV_ERR_ANA_OUTRANGE] = DEV_ST_OK;
    }
    EXIT_CRITICAL();

    if (!analog_write_volt_lock(f_value)) {
        sys_state.dev_error[DEV_ERR_LTC2606] = DEV_ST_ERR;
    } else {
        sys_state.dev_error[DEV_ERR_LTC2606] = DEV_ST_OK;
    }
}

/*
 * 功能描述：依据模拟输出范围更改阈值范围。
 * 入参说明：无。
 * 返 回 值：无。
 */
void ana_threshold_range(void) {
    ENTER_CRITICAL();

    /* 电流 */
    switch (sys_para.ana.type) {
        case ANA_0_20MA: ana_status.current_range = (20 - 0); break;
        case ANA_4_20MA: ana_status.current_range = (20 - 4); break;
        default: break;
    }

    /* 电压 */
    ana_status.voltage_range = (ANA_VOLT_RANGE_MAX - ANA_VOLT_RANGE_MIN);

    EXIT_CRITICAL();
}

/*
 * 功能描述：使用2mA和20mA两点来得出校准参数。（用于电流）
 * 入参说明：无。
 * 返 回 值：无。
 * 备注：采用两点校准原理（线性），Y = AX+B，X是理论输出电流（0~20000000nA），Y是实际输出电流。
 */
void get_current_calib_para(void) {
    float current_high;
    float current_low;

    /* OI,OV处理后恢复自动控制输出 */
    sys_state.ana_curt_value = ANA_OUT_AUTO;
    sys_state.ana_volt_value = ANA_OUT_AUTO;

    /* 2mA和20mA实际输出电流值 */
    current_low  = (float)sys_para.ana.real_curt_2ma;
    current_high = (float)sys_para.ana.real_curt_20ma;

    /* 计算出A和B的值 */
    ENTER_CRITICAL();
    ana_status.current_a = (current_high - current_low) / (CALIB_CURRENT_HIGH - CALIB_CURRENT_LOW);
    ana_status.current_b = current_high - ana_status.current_a * CALIB_CURRENT_HIGH;
    EXIT_CRITICAL();
}
/*
 * 功能描述：使用1V和10V两点来得出校准参数。（用于电压,单位：uV）
 * 入参说明：无。
 * 返 回 值：无。
 * 备注：采用两点校准原理（线性），Y = AX+B，X是理论输出电压电压（0~5000000uV），Y是实际输出电压。
 */
void get_voltage_calib_para(void) {
    float volt_high;
    float volt_low;

    /* OI,OV处理后恢复自动控制输出 */
    sys_state.ana_curt_value = ANA_OUT_AUTO;
    sys_state.ana_volt_value = ANA_OUT_AUTO;

    /* 1V和10V实际输出电压值 */
    volt_low  = (float)sys_para.ana.real_volt_1v;
    volt_high = (float)sys_para.ana.real_volt_10v;

    /* 计算出A和B的值 */
    ENTER_CRITICAL();
    ana_status.voltage_a = (float)((volt_high - volt_low) / (CALIB_VOLT_HIGH - CALIB_VOLT_LOW));
    ana_status.voltage_b = (float)(volt_high - ana_status.voltage_a * CALIB_VOLT_HIGH);
    EXIT_CRITICAL();
}

/*
 * 功能描述：根据目标温度和模拟输出范围转化为模拟输出电流/电压值。
 * 入参说明：temp --- 测得的目标温度。
 * 返 回 值：模拟输出目标温度对应的理论电流/电压值。
 */
static float anolog_temp_convert(float temp) {
    float bsp_val = 0.0f;
    float current;
    float voltage;
    int   real_temp;

    /* 温度取整 */
    real_temp = (int)(temp * 100.0f);

    /* 校准后根据温度输出对应电流/电压，掉电恢复AT */
    if (sys_state.ana_temp_convert_flag == 1) {
        sys_state.ana_calib_flag = 0;
        real_temp                = (int)(sys_state.ana_temp_convert_value * 100);
    }

    /* 超出测温范围 */
    if (real_temp < ana_status.temp_lower) {
        if (sys_para.ana.type != ANA_0_10V) { /* ANA_OUT_0_20MA / ANA_OUT_4_20MA */
            bsp_val = 0.0f;
            if (sys_para.ana.type == ANA_4_20MA) {
                bsp_val = 4.0f;
            }
        } else {
            bsp_val = 1.0f;
        }
    } else if (real_temp > ana_status.temp_upper) {
        if (sys_para.ana.type != ANA_0_10V) {
            bsp_val = 20.0f;
        } else {
            bsp_val = 10.0f;
        }
    } else {
        bsp_val = (float)(real_temp - ana_status.temp_lower) / (float)(ana_status.temp_upper - ana_status.temp_lower);
        if (sys_para.ana.type != ANA_0_10V) {
            /* 计算当前温度的理论电流值（线性） */
            current = bsp_val * (float)ana_status.current_range;
            /* 4~20mA类型下 */
            if (sys_para.ana.type == ANA_4_20MA) {
                current += 4.0f;
            }
            bsp_val = current;
        } else {
            /* 计算当前温度的理论电压值（线性） */
            voltage = bsp_val * (float)ana_status.voltage_range + ANA_VOLT_RANGE_MIN;
            bsp_val = voltage;
        }
    }

    /* 校准后模拟输出的测试值，掉电恢复AC */
    if (sys_state.ana_calib_flag == 1) {
        bsp_val = (float)sys_state.ana_calib_value / 1000000.0f;

        return bsp_val;
    }

    return bsp_val;
}

/*
 * 功能描述：此函数向LTC2606写入电压（受互斥锁保护）
 * 入参说明：volt --- 输出电压（单位：uV）
 * 返 回 值：成功写入的字节数
 */
static int analog_write_volt_lock(float volt) {
    int ret;

    //    if (dac_camera_lock()) {
    //        return 0;
    //    }
    ret = ltc2606_write_volt(volt);
    //    dac_camera_unlock();

    return ret;
}
