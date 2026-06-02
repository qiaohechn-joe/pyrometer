/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：display.h
 * 文件描述：数码管及OLED显示业务代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/10/31 乔鹤   初始版本
 *           V1.1 2025/03/19 李兆越 适配新框架
 *           V1.2 2025/03/26 李兆越 加入OLED处理逻辑
 */

#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "ui.h"

#define IP_SEG_LIMITE 256
#define BLINKING_TIME 1

void display_task_init(void);
void display_task(void);
void display_process(void);
void dis_led_tube_process(void);
void dis_write_laser_led(void);

void oled_show_info(void);
void oled_show_menu(void);
void oled_show_config(void);

void do_temp_mode(void);
void do_temp_units(void);
void do_relay_mode(void);
void do_atten_relay(void);
void do_atten_failsafe(void);
void do_aim_device(void);
void do_fact_default(void);
void do_panel_lock(void);

void do_emissivity(void);
void do_slope(void);
void do_transmissivity_low(void);
void do_transmissivity_high(void);
void do_temp_filt_size(void);

void do_multi_addr(void);
void do_baud_rate(void);

void do_analog_mode(void);
void do_analog_out_low(void);
void do_analog_out_high(void);

void do_remote_ip(void);
void do_local_ip(void);
void do_eth_gw(void);
void do_eth_nm(void);
void do_eth_port(void);

#endif /* _DISPLAY_H */
