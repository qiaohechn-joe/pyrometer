/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：display.c
 * 文件描述：数码管及OLED显示业务代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/10/31 乔鹤   初始版本
 *           V1.1 2025/03/19 李兆越 适配新框架
 *           V1.2 2025/03/26 李兆越 加入OLED处理逻辑
 *           V1.3 2025/09/16 张赛   优化OLED显示逻辑
 */

#include "display.h"
#include "tm1639.h"
#include "oled.h"
#include "system.h"
#include "ui.h"
#include "self_check.h"

extern ErrorStatus_t err_table[];

static void dis_write_nixie_tube(uint8_t *buf);

void dis_led_tube_process(void);

/*
 * 功能描述：更新数码管显示函数（拆分4字节数据为半字节格式）
 * 入参说明：buf --- 指向4字节数据的指针，每字节拆成两个半字节显示
 * 返 回 值：无
 */
static void dis_write_nixie_tube(uint8_t *buf) {
    uint8_t i, j = 0;
    uint8_t wrbuf[12];

    for (i = 0; i < 4; i++) {
        wrbuf[j++] = buf[i] & 0x0F;
        wrbuf[j++] = (buf[i] >> 4) & 0x0F;
    }
    tm1639_write_buff(0, wrbuf, 8);
}

/*
 * 功能描述：刷新LED数码管目标温度界面，支持显示正负数温度及异常状态（EUUU）
 * 入参说明：无
 * 返 回 值：无
 */
void dis_led_tube_process(void) {
    int16_t temp;
    uint8_t a, b, c, d;
    uint8_t wrbuf[6];
    bool    negative = 0;

    //    sys_state.target_temp = 100; /* 模拟固定目标温度 */
    //    temp = get_random_value(10000, 20000); /* 生成一个随机温度值（900~1000）代替实际温度 */

    /*单位转换*/
    float target_temp = convert_uint(sys_state.target_temp, TEMP_UNIT_INTERNAL, sys_para.temp.unit);

    temp          = target_temp * 10;
    int err_index = get_highest_error_index();
    if (err_index >= 0 && err_index < ERROR_ALL_CNT) {
        /* 测温异常，显示 EUUU */
        for (uint8_t i = 0; i < 4; i++) {
            wrbuf[i] = err_table[err_index].err_text[i];
        }
    } else {
        if (temp < 0) {
            temp     = -temp;
            negative = 1;
        }

        if (negative) {
            /* 负数处理（显示前置 '-'） */
            a = temp / 1000;
            b = (temp / 100) % 10;

            if ((a == 0) && (b == 0)) {
                wrbuf[0] = specialchar[0]; /* 空白 */
                wrbuf[1] = specialchar[0]; /* 空白 */
                wrbuf[2] = specialchar[4]; /* '-' 负号 */
            } else if (a == 0) {
                wrbuf[0] = specialchar[0]; /* 空白 */
                wrbuf[1] = specialchar[4]; /* '-' */
                wrbuf[2] = numchar[b];     /* 显示数字 */
            } else {
                wrbuf[0] = specialchar[4]; /* '-' */
                wrbuf[1] = numchar[a];
                wrbuf[2] = numchar[b];
            }
        } else {
            /* 正数处理 */
            a = temp / 10000;
            b = (temp / 1000) % 10;
            c = (temp / 100) % 10;
            d = (temp / 10) % 10;

            wrbuf[0] = (a == 0) ? specialchar[0] : numchar[a];
            wrbuf[1] = ((a == 0) && (b == 0)) ? specialchar[0] : numchar[b];
            wrbuf[2] = ((a == 0) && (b == 0) && (c == 0)) ? specialchar[0] : numchar[c];
            wrbuf[3] = numchar[d];
        }
    }

    dis_write_nixie_tube(wrbuf); /* 刷新显示 */
}

/*
 * 功能描述：面板指示灯闪烁
 * 入参说明：item - 菜单项指针
 * 返 回 值：无
 * 备    注：闪烁为环温未稳定，常量为环温稳定
 */
void dis_write_laser_led(void) {
    bool    states;
    uint8_t wrbuf = 0;

    if (sys_state.internal_temp[SENS_CH_NTC_SPEC] > (sys_para.heater.set_temp - 0.3f) &&
        sys_state.internal_temp[SENS_CH_NTC_SPEC] < (sys_para.heater.set_temp + 0.3f)) {
        states = 1;
    } else {
        if (states) {
            states = 0;
        } else {
            states = 1;
        }
    }

    if (states) {
        wrbuf = 0xFF;
    }
    tm1639_write_buff(0xC, &wrbuf, 1);
}

/*
 * 功能描述：OLED信息展示
 * 入参说明：item - 菜单项指针
 * 返 回 值：无
 */
void oled_show_info(void) {
    float   temp1;
    float   temp2;
    uint8_t temp3;
    char    buffer1[50];
    char    buffer2[50];

    /* 面板锁定检测 */
    if (sys_state.panel_lock == 'L') { // todo：这是干啥的？？？
        gd32_delay_ms(50);
    }

    /* 根据当前索引显示不同内容 */
    switch (ui.sub_page_id) {
        case 0: {
            /* 2-COLOR内部温度显示 */
            temp1 = convert_uint(sys_state.internal_temp[SENS_CH_NTC_TIA], TEMP_UNIT_INTERNAL, sys_para.temp.unit);

            uint8_t len      = sprintf(buffer1, "%d-COLOR  %.1f", sys_para.temp.temp_mode, temp1);
            buffer1[len]     = ' ' + 95;
            buffer1[len + 1] = sys_para.temp.unit;
            oled_show_string(0, 0, (uint8_t *)buffer1, 16, 1);

            /* 传感器数据和衰减率 */
            temp2 = sys_para.temp.slope;
            temp3 = sys_state.Measured_attenuation * 100;
            sprintf(buffer2, "S=%.3f  At:%3u%%", temp2, temp3);
            oled_show_string(0, 16, (uint8_t *)buffer2, 16, 1);
            break;
        }
        case 1: {
            /* 内部温度显示 */
            sprintf(buffer1, "INTERNAL TEMP");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);

            temp1 = convert_uint(sys_state.internal_temp[SENS_CH_NTC_SPEC], TEMP_UNIT_INTERNAL, sys_para.temp.unit);
            sprintf(buffer2, "%.1f %c%c", temp1, ' ' + 95, sys_para.temp.unit);
            oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
            break;
        }
        case 2: {
            /* 衰减率显示 */
            sprintf(buffer1, "ATTENUATION");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);

            temp3 = sys_state.Measured_attenuation * 100;
            sprintf(buffer2, "At:%3u%%", temp3);
            oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
            break;
        }
        case 3: {
            /* 温度下限显示 */
            sprintf(buffer1, "LOW LIMIT");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);

            temp1 = convert_uint(sys_para.temp.dev_temp_min, TEMP_UNIT_INTERNAL, sys_para.temp.unit);
            sprintf(buffer2, "%.1f %c%c", temp1, ' ' + 95, sys_para.temp.unit);
            oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
            break;
        }
        case 4: {
            /* 温度上限显示 */
            sprintf(buffer1, "HIGH LIMIT");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);

            temp1 = convert_uint(sys_para.temp.dev_temp_max, TEMP_UNIT_INTERNAL, sys_para.temp.unit);
            sprintf(buffer2, "%.1f %c%c", temp1, ' ' + 95, sys_para.temp.unit);
            oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
            break;
        }
        case 5: {
            /* 测温仪型号 */
            sprintf(buffer1, "SENSOR IDENT");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);

            sprintf(buffer2, "LED102-V3");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
            break;
        }
        case 6: {
            /* 测温仪固件版本 */
            sprintf(buffer1, "SENSOR REVISION");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);

            char buffer2[16];
            sprintf(buffer2, "V%d.%d.%d", version_major(SYS_VERSION.fw), version_minor(SYS_VERSION.fw), version_serial(SYS_VERSION.fw));
            oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
            break;
        }
        case 7: {
            /* 序列号显示 */
            sprintf(buffer1, "SERIAL NUMBER");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);

            sprintf(buffer2, "2405060201");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
            break;
        }
        case 8: {
            /* MAC地址显示 */
            sprintf(buffer1, "MAC ADDRESS");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);

            sprintf(buffer2, "001dBd2aaa01");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
            break;
        }
        default: break;
    }
}

/*
 * 功能描述：OLED菜单展示
 * 入参说明：item - 菜单项指针
 * 返 回 值：无
 */
void oled_show_menu(void) {
    char buffer1[50];
    char buffer2[50];
    char buffer3[3];
    /* 根据当前索引显示不同内容 */
    switch (ui.main_page_id) {
        case 0: {
            sprintf(buffer1, "INFORMATION");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
            break;
        }
        case 1: {
            sprintf(buffer1, "CONFIGURATION");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
            break;
        }
        case 2: {
            sprintf(buffer1, "PARA CFG");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
            break;
        }
        case 3: {
            sprintf(buffer1, "INTERFACE");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
            break;
        }
        case 4: {
            sprintf(buffer1, "ANALOG");
            oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
            break;
        }
        default: break;
    }
    sprintf(buffer2, "MENU");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    if (ui.main_page_id > 0 && ui.main_page_id < 5 && sys_para.dev.panel_lock != 'U') {
        sprintf(buffer3, "L");
        oled_show_string(10, 16, (uint8_t *)buffer3, 16, 1);
    }
}

/*
 * 功能描述：测温模式显示界面
 * 入参说明：无
 * 返 回 值：无
 */
void do_temp_mode(void) {
    char buffer1[50];
    char buffer2[50];

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "              ";

    if (ui.is_blinking == false) {
        if (sys_para.temp.temp_mode == 2) {
            ui.dis_value = 1;
        } else {
            ui.dis_value = 0;
        }
    }

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;
        if (ui.dis_value < (int16_t)0) {
            ui.dis_value = 1;
        } else if (ui.dis_value > (int16_t)1) {
            ui.dis_value = 0;
        }
    }
    sprintf(buffer1, "MODE");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%d - COLOR", ui.dis_value + 1);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);

    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }

    if (ui.cfg_states) {
        ui.cfg_states = 0;

        if (sys_para.temp.temp_mode == 1 || sys_para.temp.temp_mode == 2) {
            if (ui.dis_value) {
                sys_para.temp.temp_mode = 2;
            } else {
                sys_para.temp.temp_mode = 1;
            }
            pmm_save_group(PARA_TEMP);

            sprintf(buffer2, "%d - COLOR", ui.dis_value + 1);
            oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
        }
    }
}

/*
 * 功能描述：温度单位设置
 * 入参说明：无
 * 返 回 值：无
 */
void do_temp_units(void) {
    char buffer1[50];
    char buffer2[50];
    char unit[] = {'C', 'F', 'K'};

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        if (sys_para.temp.unit == 'C') {
            ui.dis_value = 0;
        } else if (sys_para.temp.unit == 'F') {
            ui.dis_value = 1;
        } else {
            ui.dis_value = 2;
        }
    }

    sprintf(buffer1, "TEMP UNIT");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%c", unit[ui.dis_value]);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;

        if (ui.dis_value < (int16_t)0) {
            ui.dis_value = 2;
        }
        if (ui.dis_value > (int16_t)2) {
            ui.dis_value = 0;
        }
    }
    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }

    if (ui.cfg_states) {
        ui.cfg_states      = 0;
        sys_para.temp.unit = unit[ui.dis_value];
        pmm_save_group(PARA_TEMP);

        sprintf(buffer2, "%c", unit[ui.dis_value]);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：报警模式设置
 * 入参说明：无
 * 返 回 值：无
 */
void do_relay_mode(void) {
    char buffer1[50];
    char buffer2[50];

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        ui.dis_value = sys_para.alarm.relay_alarm_mode;
    }

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;

        if (ui.dis_value < (int16_t)0) {
            ui.dis_value = 9;
        }
        if (ui.dis_value > (int16_t)9) {
            ui.dis_value = 0;
        }
    }
    sprintf(buffer1, "RELAY MODE");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%d", ui.dis_value);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);

    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }

    if (ui.cfg_states) {
        ui.cfg_states = 0;

        sys_para.alarm.relay_alarm_mode = ui.dis_value;
        pmm_save_group(PARA_ALARM);

        sprintf(buffer2, "%d", ui.dis_value);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：衰减报警设置
 * 入参说明：无
 * 返 回 值：无
 */
void do_atten_relay(void) {
    char    buffer1[50];
    char    buffer2[50];
    uint8_t temp3;

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;
        if (ui.dis_value < (int16_t)0) {
            ui.dis_value = 0;
        } else if (ui.dis_value > (int16_t)100) {
            ui.dis_value = 100;
        }
    } else if (ui.cfg_states) {
        ui.cfg_states             = 0;
        sys_para.alarm.decay_rate = (unsigned int)ui.dis_value;
        pmm_save_group(PARA_ALARM);
    } else {
        ui.dis_value = (int16_t)sys_para.alarm.decay_rate;
    }

    sprintf(buffer1, "ATTENUATION");
    oled_show_string(4, 0, (uint8_t *)buffer1, 16, 1);
    temp3 = sys_para.alarm.decay_rate * 100;
    sprintf(buffer2, "RELAY   %3u%%", temp3);
    oled_show_string(8, 16, (uint8_t *)buffer2, 16, 1);
}

/*
 * 功能描述：衰减安全保护设置
 * 入参说明：无
 * 返 回 值：无
 */
void do_atten_failsafe(void) {
    char    buffer1[50];
    char    buffer2[50];
    uint8_t temp3;

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;
        if (ui.dis_value < (int16_t)0) {
            ui.dis_value = 0;
        } else if (ui.dis_value > (int16_t)100) {
            ui.dis_value = 100;
        }
    } else if (ui.cfg_states) {
        ui.cfg_states                  = 0;
        sys_para.alarm.decay_fail_safe = (unsigned int)ui.dis_value;
        pmm_save_group(PARA_ALARM);
    } else {
        ui.dis_value = (int16_t)sys_para.alarm.decay_fail_safe;
    }

    sprintf(buffer1, "ATTENUATION");
    oled_show_string(4, 0, (uint8_t *)buffer1, 16, 1);
    temp3 = sys_para.alarm.decay_fail_safe;
    sprintf(buffer2, "FAILSAFE   %3u%%", temp3);
    oled_show_string(8, 16, (uint8_t *)buffer2, 16, 1);
}

/*
 * 功能描述：瞄准设备设置
 * 入参说明：无
 * 返 回 值：无
 */
void do_aim_device(void) {
    char buffer1[50];
    char buffer2[50];

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        ui.dis_value = sys_para.video.video_control;
    }

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;

        if (ui.dis_value < (int16_t)0) {
            ui.dis_value = 1;
        }
        if (ui.dis_value > (int16_t)1) {
            ui.dis_value = 0;
        }
    }
    sprintf(buffer1, "CAMERA");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%s", (ui.dis_value == 1) ? "ON" : "OFF");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);

    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }
    if (ui.cfg_states) {
        ui.cfg_states                = 0;
        sys_para.video.video_control = ui.dis_value;
        pmm_save_group(PARA_VIDEO);

        sprintf(buffer2, "%s", (ui.dis_value == 1) ? "ON" : "OFF");
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：出厂默认设置
 * 入参说明：无
 * 返 回 值：无
 */
void do_fact_default(void) {
    char buffer1[50];
    char buffer2[50];

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        ui.dis_value = 0;
    }

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;

        if (ui.dis_value < (int16_t)0) {
            ui.dis_value = 1;
        }
        if (ui.dis_value > (int16_t)1) {
            ui.dis_value = 0;
        }
    }
    sprintf(buffer1, "FACTORY DEFAULT");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%s", (ui.dis_value == 1) ? "YES" : "NO");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);

    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }

    if (ui.cfg_states) {
        ui.cfg_states = 0;
        pmm_restore_to_default();
        sprintf(buffer2, "%s", (ui.dis_value == 1) ? "YES" : "NO");
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：解锁，上锁设置
 * 入参说明：无
 * 返 回 值：无
 */
void do_panel_lock(void) {
    char buffer1[50];
    char buffer2[50];
    char value;

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;

        if (ui.dis_value < (int16_t)0) {
            ui.dis_value = 1;
        }
        if (ui.dis_value > (int16_t)1) {
            ui.dis_value = 0;
        }
    } else if (ui.cfg_states) {
        ui.cfg_states = 0;

        if (ui.dis_value == 0) {
            sys_state.panel_lock = 'L';
        } else if (ui.dis_value == 1) {
            sys_state.panel_lock = 'U';
        }
    } else {
        if (sys_state.panel_lock == 'L') {
            ui.dis_value = 0;
        } else if (sys_state.panel_lock == 'U') {
            ui.dis_value = 1;
        }
    }

    if (ui.dis_value == 0) {
        value = 'L';
    } else if (ui.dis_value == 1) {
        value = 'U';
    }

    sprintf(buffer1, "CPP_PANEL_LOCK");
    oled_show_string(4, 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%c", value);
    oled_show_string(8, 16, (uint8_t *)buffer2, 16, 1);
}

/*
 * 功能描述：发射率显示界面
 * 入参说明：无
 * 返 回 值：无
 * 备    注：%hd为short
 */
void do_emissivity(void) {
    char buffer1[50];
    char buffer2[50];

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        ui.dis_value = (int16_t)(sys_para.temp.emissivity * 1000);
    }

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;
        if (ui.dis_value < (int16_t)100) {
            ui.dis_value = 100;
        } else if (ui.dis_value > (int16_t)1150) {
            ui.dis_value = 1150;
        }
    }
    sprintf(buffer1, "EMISSIVITY");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%hd", (short)ui.dis_value);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }

    if (ui.cfg_states) {
        ui.cfg_states            = 0;
        sys_para.temp.emissivity = (float)ui.dis_value / (float)1000;
        pmm_save_group(PARA_TEMP);

        sprintf(buffer2, "%hd", (short)ui.dis_value);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：坡度显示界面
 * 入参说明：无
 * 返 回 值：无
 */
void do_slope(void) {
    char buffer1[50];
    char buffer2[50];

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        ui.dis_value = (int16_t)(sys_para.temp.slope * 1000);
    }

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;
        if (ui.dis_value < (int16_t)100) {
            ui.dis_value = 100;
        } else if (ui.dis_value > (int16_t)1150) {
            ui.dis_value = 1150;
        }
    }
    sprintf(buffer1, "SLOPE");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%hd", (short)ui.dis_value);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }
    if (ui.cfg_states) {
        ui.cfg_states       = 0;
        sys_para.temp.slope = (float)ui.dis_value / (float)1000;
        //        ps_write_flash((uint8_t*)&sys_para.temp.emi[SENS_CH_SPEC_WB], 4);
        pmm_save_group(PARA_TEMP);

        sprintf(buffer2, "%hd", (short)ui.dis_value);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：窄带透射率显示界面
 * 入参说明：无
 * 返 回 值：无
 */
void do_transmissivity_low(void) {
    char buffer1[50];
    char buffer2[50];

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        ui.dis_value = (int16_t)(sys_para.temp.trans[SENS_CH_SPEC_NB] * 1000);
    }

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;
        if (ui.dis_value < (int16_t)100) {
            ui.dis_value = 100;
        } else if (ui.dis_value > (int16_t)1500) {
            ui.dis_value = 1500;
        }
    }
    sprintf(buffer1, "TRANSMISSIVITY");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "N  %hd", (short)ui.dis_value);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }

    if (ui.cfg_states) {
        ui.cfg_states                        = 0;
        sys_para.temp.trans[SENS_CH_SPEC_NB] = (float)ui.dis_value / (float)1000;
        //        ps_write_flash((uint8_t*)&sys_para.temp.trans[SENS_CH_SPEC_NB], 4);
        pmm_save_group(PARA_TEMP);

        sprintf(buffer2, "N  %hd", (short)ui.dis_value);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：宽带透射率显示界面
 * 入参说明：无
 * 返 回 值：无
 */
void do_transmissivity_high(void) {
    char buffer1[50];
    char buffer2[50];

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        ui.dis_value = (int16_t)(sys_para.temp.trans[SENS_CH_SPEC_WB] * 1000);
    }

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;
        if (ui.dis_value < (int16_t)100) {
            ui.dis_value = 100;
        } else if (ui.dis_value > (int16_t)1500) {
            ui.dis_value = 1500;
        }
    }
    sprintf(buffer1, "TRANSMISSIVITY");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "W  %hd", (short)ui.dis_value);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }
    if (ui.cfg_states) {
        ui.cfg_states                        = 0;
        sys_para.temp.trans[SENS_CH_SPEC_WB] = (float)ui.dis_value / (float)1000;
        pmm_save_group(PARA_TEMP);

        sprintf(buffer2, "W  %hd", (short)ui.dis_value);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：温度滤波BUF大小显示界面
 * 入参说明：无
 * 返 回 值：无
 */
void do_temp_filt_size(void) {
    char buffer1[50];
    char buffer2[50];

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";
    if (ui.is_blinking == false) {
        ui.dis_value = (int16_t)sys_para.filter.temp_filter_buf_size;
    }

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;
        if (ui.dis_value < (int16_t)1) {
            ui.dis_value = 1;
        } else if (ui.dis_value > (int16_t)150) {
            ui.dis_value = 150;
        }
    }
    sprintf(buffer1, "TEMP FILT SIZE");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%hd", (short)ui.dis_value);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }
    if (ui.cfg_states) {
        ui.cfg_states                        = 0;
        sys_para.filter.temp_filter_buf_size = (unsigned int)ui.dis_value;
        pmm_save_group(PARA_FILTER);

        sprintf(buffer2, "%hd", (short)ui.dis_value);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：模拟输出模式
 * 入参说明：无
 * 返 回 值：无
 */
void do_analog_mode(void) {
    char buffer1[50];
    char buffer2[50];

    char str[3][20] = {"0-20mA", "4-20mA", "0-10V"};
    char r_value[3] = {'0', '4', '5'};

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        if (sys_para.ana.type == '0') {
            ui.dis_value = 0;
        } else if (sys_para.ana.type == '4') {
            ui.dis_value = 1;
        } else {
            ui.dis_value = 2;
        }
    }

    sprintf(buffer1, "ANALOG MODE");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%s", str[ui.dis_value]);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;

        if (ui.dis_value < (int16_t)0) {
            ui.dis_value = 2;
        }
        if (ui.dis_value > (int16_t)2) {
            ui.dis_value = 0;
        }
    }
    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }

    if (ui.cfg_states) {
        ui.cfg_states     = 0;
        sys_para.ana.type = r_value[ui.dis_value];
        pmm_save_group(PARA_ANA);

        sprintf(buffer2, "%s", str[ui.dis_value]);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：模拟输出下限显示界面
 * 入参说明：无
 * 返 回 值：无
 */
void do_analog_out_low(void) {
    char    buffer1[50];
    char    buffer2[50];
    int16_t min_limit = sys_para.temp.dev_temp_min;
    int16_t max_limit = sys_para.temp.dev_temp_max;

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        ui.dis_value = (int16_t)(sys_para.ana.lower);
    }

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;

        if (ui.dis_value < min_limit) {
            ui.dis_value = min_limit;
        } else {
            if (ui.dis_value > max_limit) {
                ui.dis_value = max_limit;
            }
        }
    }

    sprintf(buffer1, "ANALOG OUT");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "LOW   %hd", (short)ui.dis_value);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);

    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }

    if (ui.cfg_states) {
        ui.cfg_states      = 0;
        sys_para.ana.lower = (float)ui.dis_value;
        //        anologOut_mode_change_process();
        pmm_save_group(PARA_ANA);

        sprintf(buffer2, "LOW   %hd", (short)ui.dis_value);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：模拟输出上限显示界面
 * 入参说明：无
 * 返 回 值：无
 */
void do_analog_out_high(void) {
    char    buffer1[50];
    char    buffer2[50];
    int16_t min_limit = sys_para.temp.dev_temp_min;
    int16_t max_limit = sys_para.temp.dev_temp_max;

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        ui.dis_value = (int16_t)(sys_para.ana.upper);
    }

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;

        if (ui.dis_value < min_limit) {
            ui.dis_value = min_limit;
        } else {
            if (ui.dis_value > max_limit) {
                ui.dis_value = max_limit;
            }
        }
    }

    sprintf(buffer1, "ANALOG OUT");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "HIGH   %hd", (short)ui.dis_value);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);

    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }

    if (ui.cfg_states) {
        ui.cfg_states      = 0;
        sys_para.ana.upper = (float)ui.dis_value;
        //        anologOut_mode_change_process();
        pmm_save_group(PARA_ANA);

        sprintf(buffer2, "HIGH   %hd", (short)ui.dis_value);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：多点地址设置
 * 入参说明：无
 * 返 回 值：无
 */
void do_multi_addr(void) {
    char buffer1[50];
    char buffer2[50];

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        ui.dis_value = (int16_t)sys_para.dev.addr;
    }

    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;
        if (ui.dis_value < (int16_t)0) {
            ui.dis_value = 0;
        } else if (ui.dis_value > (int16_t)32) {
            ui.dis_value = 32;
        }
    }
    sprintf(buffer1, "MULTI ADDR");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%hd", (short)ui.dis_value);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);

    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }

    if (ui.cfg_states) {
        ui.cfg_states     = 0;
        sys_para.dev.addr = (unsigned int)ui.dis_value;
        pmm_save_group(PARA_DEV);

        sprintf(buffer2, "%hd", (short)ui.dis_value);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：波特率设置
 * 入参说明：无
 * 返 回 值：无
 */
void do_baud_rate(void) {
    char buffer1[50];
    char buffer2[50];

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        ui.dis_value = (int16_t)(sys_para.port.baudrate / 100);
    }
    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;

        if (ui.dis_value < (int16_t)3) {
            ui.dis_value = 3;
        }
        if (ui.dis_value > (int16_t)1152) {
            ui.dis_value = 1152;
        }
    }

    sprintf(buffer1, "BAUD RATE");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%hd", (short)ui.dis_value);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);

    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }
    if (ui.cfg_states) {
        ui.cfg_states          = 0;
        sys_para.port.baudrate = (unsigned int)(ui.dis_value * 100);
        if (!is_valid_baudrate(sys_para.port.baudrate)) {
            sys_para.port.baudrate = 115200;
        }

        pmm_save_group(PARA_PORT);
        /*生效波特率*/
        pc_init();

        sprintf(buffer2, "%hd", (short)ui.dis_value);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}

/*
 * 功能描述：本地IP地址设置
 * 入参说明：无
 * 返 回 值：无
 */
void do_local_ip(void) {
    char buffer1[50];
    char buffer2[50];

    static int IPconfig_value[4]    = {0};
    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "   ";
    if (ui.is_blinking == false) {
        sscanf((char *)sys_para.net.ip_addr, "%3u.%3u.%3u.%3u", &IPconfig_value[0], &IPconfig_value[1], &IPconfig_value[2], &IPconfig_value[3]);
    }

    ui.ip_cfg_disvalue[0] = IPconfig_value[0];
    ui.ip_cfg_disvalue[1] = IPconfig_value[1];
    ui.ip_cfg_disvalue[2] = IPconfig_value[2];
    ui.ip_cfg_disvalue[3] = IPconfig_value[3];

    if (ui.set_states) {
        ui.set_states                             = 0;
        ui.ip_cfg_disvalue[ui.current_para_index] = (ui.ip_cfg_disvalue[ui.current_para_index] + ui.set_value) % IP_SEG_LIMITE;
        IPconfig_value[ui.current_para_index]     = ui.ip_cfg_disvalue[ui.current_para_index];
    }
    sprintf(buffer1, "LOCAL IP");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%03u.%03u.%03u.%03u", ui.ip_cfg_disvalue[0], ui.ip_cfg_disvalue[1], ui.ip_cfg_disvalue[2], ui.ip_cfg_disvalue[3]);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }

    if (ui.cfg_states) {
        ui.cfg_states = 0;
        sprintf((char *)sys_para.net.ip_addr, "%03u.%03u.%03u.%03u", ui.ip_cfg_disvalue[0], ui.ip_cfg_disvalue[1], ui.ip_cfg_disvalue[2],
                ui.ip_cfg_disvalue[3]);
        pmm_save_group(PARA_NET);
        sprintf(buffer2, "%03u.%03u.%03u.%03u", ui.ip_cfg_disvalue[0], ui.ip_cfg_disvalue[1], ui.ip_cfg_disvalue[2], ui.ip_cfg_disvalue[3]);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }

    ui.set_states = 0; /* 防止标志位异常置位 */
    ui.cfg_states = 0;
}

/*
 * 功能描述：网关地址设置
 * 入参说明：无
 * 返 回 值：无
 */
void do_eth_gw(void) {
    char buffer1[50];
    char buffer2[50];

    static int IPconfig_value[4]    = {0};
    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "   ";
    if (ui.is_blinking == false) {
        sscanf((char *)sys_para.net.gateway, "%3u.%3u.%3u.%3u", &IPconfig_value[0], &IPconfig_value[1], &IPconfig_value[2], &IPconfig_value[3]);
    }

    ui.ip_cfg_disvalue[0] = IPconfig_value[0];
    ui.ip_cfg_disvalue[1] = IPconfig_value[1];
    ui.ip_cfg_disvalue[2] = IPconfig_value[2];
    ui.ip_cfg_disvalue[3] = IPconfig_value[3];

    if (ui.set_states) {
        ui.set_states                             = 0;
        ui.ip_cfg_disvalue[ui.current_para_index] = (ui.ip_cfg_disvalue[ui.current_para_index] + ui.set_value) % IP_SEG_LIMITE;
        IPconfig_value[ui.current_para_index]     = ui.ip_cfg_disvalue[ui.current_para_index];
    }

    sprintf(buffer1, "GATEWAY");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%03u.%03u.%03u.%03u", ui.ip_cfg_disvalue[0], ui.ip_cfg_disvalue[1], ui.ip_cfg_disvalue[2], ui.ip_cfg_disvalue[3]);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);

    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }

    if (ui.cfg_states) {
        ui.cfg_states = 0;
        sprintf((char *)sys_para.net.gateway, "%03u.%03u.%03u.%03u", ui.ip_cfg_disvalue[0], ui.ip_cfg_disvalue[1], ui.ip_cfg_disvalue[2],
                ui.ip_cfg_disvalue[3]);
        pmm_save_group(PARA_NET);
        sprintf(buffer2, "%03u.%03u.%03u.%03u", ui.ip_cfg_disvalue[0], ui.ip_cfg_disvalue[1], ui.ip_cfg_disvalue[2], ui.ip_cfg_disvalue[3]);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }

    ui.set_states = 0; /* 防止标志位异常置位 */
    ui.cfg_states = 0;
}

/*
 * 功能描述：子网掩码设置
 * 入参说明：无
 * 返 回 值：无
 */
void do_eth_nm(void) {
    char buffer1[50];
    char buffer2[50];

    static int IPconfig_value[4]    = {0};
    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "   ";
    if (ui.is_blinking == false) {
        sscanf((char *)sys_para.net.net_mask, "%3u.%3u.%3u.%3u", &IPconfig_value[0], &IPconfig_value[1], &IPconfig_value[2], &IPconfig_value[3]);
    }

    ui.ip_cfg_disvalue[0] = IPconfig_value[0];
    ui.ip_cfg_disvalue[1] = IPconfig_value[1];
    ui.ip_cfg_disvalue[2] = IPconfig_value[2];
    ui.ip_cfg_disvalue[3] = IPconfig_value[3];

    if (ui.set_states) {
        ui.set_states                             = 0;
        ui.ip_cfg_disvalue[ui.current_para_index] = (ui.ip_cfg_disvalue[ui.current_para_index] + ui.set_value) % IP_SEG_LIMITE;
        IPconfig_value[ui.current_para_index]     = ui.ip_cfg_disvalue[ui.current_para_index];
    }

    sprintf(buffer1, "NET MASK");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%03u.%03u.%03u.%03u", ui.ip_cfg_disvalue[0], ui.ip_cfg_disvalue[1], ui.ip_cfg_disvalue[2], ui.ip_cfg_disvalue[3]);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);

    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }

    if (ui.cfg_states) {
        ui.cfg_states = 0;
        sprintf((char *)sys_para.net.net_mask, "%03u.%03u.%03u.%03u", ui.ip_cfg_disvalue[0], ui.ip_cfg_disvalue[1], ui.ip_cfg_disvalue[2],
                ui.ip_cfg_disvalue[3]);
        pmm_save_group(PARA_NET);
        sprintf(buffer2, "%03u.%03u.%03u.%03u", ui.ip_cfg_disvalue[0], ui.ip_cfg_disvalue[1], ui.ip_cfg_disvalue[2], ui.ip_cfg_disvalue[3]);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }

    ui.set_states = 0; /* 防止标志位异常置位 */
    ui.cfg_states = 0;
}

/*
 * 功能描述：网络端口设置
 * 入参说明：无
 * 返 回 值：无
 */
void do_eth_port(void) {
    char buffer1[50];
    char buffer2[50];

    static int blinking_flag        = BLINKING_TIME;
    char       change_data_buffer[] = "               ";

    if (ui.is_blinking == false) {
        ui.dis_value = sys_para.net.port;
    }
    if (ui.set_states) {
        ui.set_states = 0;
        ui.dis_value += ui.set_value;
        if (ui.dis_value < 0) {
            ui.dis_value = 65535;
        }
        if (ui.dis_value > 65535) {
            ui.dis_value = 0;
        }
    }

    sprintf(buffer1, "PORT");
    oled_show_string(OLED_X_MID - 4 * strlen(buffer1), 0, (uint8_t *)buffer1, 16, 1);
    sprintf(buffer2, "%d", ui.dis_value);
    oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);

    if (ui.is_blinking && blinking_flag-- <= 0) {
        blinking_flag = BLINKING_TIME;
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2) + (ui.current_para_index * 4 * 8), 16, (uint8_t *)change_data_buffer, 16, 1);
    }
    if (ui.cfg_states) {
        ui.cfg_states     = 0;
        sys_para.net.port = (unsigned int)ui.dis_value;
        pmm_save_group(PARA_NET);

        sprintf(buffer2, "%d", ui.dis_value);
        oled_show_string(OLED_X_MID - 4 * strlen(buffer2), 16, (uint8_t *)buffer2, 16, 1);
    }
}
