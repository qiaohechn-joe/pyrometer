/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：alarm.c
 * 文件描述：继电器报警驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/08/06 乔鹤   初始版本
 */

#include "heater.h"
#include "ostimer.h"

// static pid_t pid;
HEATER_DATA heater_data;
extern int  warm_start;

/*
 * 功能描述：加热器初始化，用于注册加热器控制任务
 * 入参说明：无
 * 返 回 值：无
 */
void heater_init(void) {
    /* 全局变量初始化 */
    heater_data.internal_temperature = 25.0; // 初始温度
    heater_data.error                = 0.0;  // 误差
    if (!warm_start) {
        heater_data.integral = 0.0; // 误差累积值
    }
    heater_data.derivative     = 0.0; // 误差变化率
    heater_data.previous_error = 0.0; // 上一次误差
    heater_data.output         = 0.0; // 输出值
    heater_data.set_value      = 0;   // 设置值
    // 环温控制
    ptc_pin_init();
    pwm_timer_init(PWM_TIMER_RCU, PWM_TIMER, PWM_PERIOD, PWM_TIMER_CH, 50);

    ostimer_register(OSTIMER_HEATER_TASK, heater_task, (void *)0, AGENT_PRIO_HI, 100);
}

/*
 * 功能描述：加热器控制任务
 * 入参说明：无
 * 返 回 值：无
 */
void heater_task(void *arg) {
    heater_data.internal_temperature = sys_state.internal_temp[SENS_CH_NTC_SPEC];
    heater_data.setpoint             = sys_para.heater.set_temp;
    if (heater_data.internal_temperature > (heater_data.setpoint - 5) && heater_data.internal_temperature < 100) {
        ambient_temp_pid_ctrl();
        PTC_PWM_Set(heater_data.set_value);
    } else if (heater_data.internal_temperature < (heater_data.setpoint - 5)) {
        PTC_PWM_Set(95);
    } else if (heater_data.setpoint == 0) {
        PTC_PWM_Set(0);
    }
}

/*
 *@brief:内部温度PID控制
 *@parameter：
 *@return :
 */
static float ambient_temp_pid_ctrl(void) {

    heater_data.error = heater_data.setpoint - heater_data.internal_temperature; // 计算误差
    heater_data.integral += heater_data.error;                                   // 更新误差累积值
    heater_data.derivative = heater_data.error - heater_data.previous_error;     // 计算误差变化率
    heater_data.output     = sys_para.heater.kp * heater_data.error + sys_para.heater.ki * heater_data.integral +
                         sys_para.heater.kd * heater_data.derivative; // 计算输出值
    heater_data.previous_error = heater_data.error;                   // 保存当前误差作为上一次误差
    // 根据输出值进行相应的控制操作，调整加热器功率
    heater_data.set_value = 50.0f + heater_data.output;

    if (heater_data.set_value > 100.0f) {
        heater_data.set_value = 100.0f;
        heater_data.integral  = heater_data.previous_integral; // 使用上次积分制，防止积分饱和
    } else if (heater_data.set_value < 0) {
        heater_data.set_value = 0;
        heater_data.integral  = heater_data.previous_integral; // 使用上次积分制，防止积分饱和
    }
    heater_data.previous_integral = heater_data.integral; // 保存当前积分作为上一次积分
}

void PTC_PWM_Set(float value) {
    uint16_t dutyValue;

    if (value > 100) value = 100;

    dutyValue = value * 50;
    timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, dutyValue);
}
