/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：alarm.c
 * 文件描述：继电器报警驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/10/31 乔鹤   初始版本
 *           V1.1 2025/03/19 李兆越 适配新框架
 *           V1.2 2025/04/18 李兆越 继电器报警采用软定时统一管理
 *           V1.3 2025/04/28 李兆越 继电器报警死区优化
 */

#include "alarm.h"
#include "ui.h"
#include "ostimer.h"

static void set_relay_states(bool states);

/*
 * 功能描述：报警初始化，用于注册报警任务
 * 入参说明：无
 * 返 回 值：无
 */
void alarm_init(void) {
    alarm_pin_init();
    ostimer_register(OSTIMER_ALARM_TASK, alarm_task, (void *)0, AGENT_PRIO_HI, 100);
}

/*
 * 功能描述：继电器报警任务
 * 入参说明：无
 * 返 回 值：无
 */
void alarm_task(void *arg) {
    static uint8_t target_alarm_active   = 0; /* 目标温度报警状态 */
    static uint8_t internal_alarm_active = 0; /* 内部温度报警状态 */
    static uint8_t decay_alarm_active    = 0; /* 衰减报警状态 */

    arg = arg;

    /* 更新目标温度报警状态 */
    if (sys_state.target_temp >= sys_para.alarm.target_temp_alarm_on) {
        target_alarm_active = 1;
    } else if (sys_state.target_temp <= (sys_para.alarm.target_temp_alarm_on - sys_para.alarm.target_temp_alarm_off)) {
        target_alarm_active = 0;
    }

    /* 更新内部温度报警状态 */
    if (sys_state.internal_temp[SENS_CH_NTC_SPEC] >= sys_para.alarm.internal_temp_alarm_on) {
        internal_alarm_active = 1;
    } else if (sys_state.internal_temp[SENS_CH_NTC_SPEC] <= (sys_para.alarm.internal_temp_alarm_on - sys_para.alarm.internal_temp_alarm_off)) {
        internal_alarm_active = 0;
    }

    /* 更新衰减报警状态 */
    if (sys_state.Measured_attenuation >= sys_para.alarm.attenuation_alarm_on) {
        decay_alarm_active = 1;
    } else if (sys_state.Measured_attenuation <= (sys_para.alarm.attenuation_alarm_on - sys_para.alarm.attenuation_alarm_off)) {
        decay_alarm_active = 0;
    }

    /* 根据报警模式设置继电器状态 */
    switch (sys_para.alarm.relay_alarm_mode) {
        case ALARM_RELAY_OFF: /* 继电器始终断开 */ set_relay_states(0); break;

        case ALARM_RELAY_ON: /* 继电器始终闭合 */ set_relay_states(1); break;

        case ALARM_NO_TARGET_INTERNAL: /* 常开，目标或内部温度报警 */ set_relay_states(target_alarm_active || internal_alarm_active); break;

        case ALARM_NC_TARGET_INTERNAL: /* 常闭，目标或内部温度报警 */ set_relay_states(!(target_alarm_active || internal_alarm_active)); break;

        case ALARM_NO_INTERNAL: /* 常开，仅内部温度报警 */ set_relay_states(internal_alarm_active); break;

        case ALARM_NC_INTERNAL: /* 常闭，仅内部温度报警 */ set_relay_states(!internal_alarm_active); break;

        case ALARM_NO_TARGET: /* 常开，仅目标温度报警 */ set_relay_states(target_alarm_active); break;

        case ALARM_NC_TARGET: /* 常闭，仅目标温度报警 */ set_relay_states(!target_alarm_active); break;

        case ALARM_NO_DECAY: /* 常开，仅衰减报警 */ set_relay_states(decay_alarm_active); break;

        case ALARM_NC_DECAY: /* 常闭，仅衰减报警 */ set_relay_states(!decay_alarm_active); break;

        default: break;
    }
}

/*
 * 功能描述：设置固态继电器状态
 * 入参说明：states：0：开路；1：闭合
 * 返 回 值：无
 */
void set_relay_states(bool states) {
    if (states)
        gpio_bit_set(RELAY_GPIO_PORT, RELAY_GPIO_PIN);
    else
        gpio_bit_reset(RELAY_GPIO_PORT, RELAY_GPIO_PIN);
}
