#include "self_check.h"
#include "storage.h"
#include "vti7064x.h"
#include "ov5640.h"

float         min_0_20ma  = 0;
ErrorStatus_t err_table[] = {
    {.error_flags   = ERR_DETECTOR_FAULT,
     .error_index   = 0,
     .err_text      = {0x79, 0x76, 0x76, 0x76, 0x00},
     .analog_output = {(void *)&sys_para.ana.exceed_max_range, (void *)&sys_para.ana.exceed_max_range}}, // EHHH
    {.error_flags   = ERR_HEATER_HIGH,
     .error_index   = 1,
     .err_text      = {0x79, 0x39, 0x76, 0x76, 0x00},
     .analog_output = {(void *)&sys_para.ana.exceed_max_range, (void *)&sys_para.ana.exceed_max_range}}, // ECHH
    {.error_flags   = ERR_HEATER_LOW,
     .error_index   = 2,
     .err_text      = {0x79, 0x39, 0x3E, 0x3E, 0x00},
     .analog_output = {(void *)&min_0_20ma, (void *)&sys_para.ana.exceed_min_range}}, // ECUU
    {.error_flags   = ERR_INTERNAL_HIGH,
     .error_index   = 3,
     .err_text      = {0x79, 0x0F, 0x76, 0x76, 0x00},
     .analog_output = {(void *)&sys_para.ana.exceed_max_range, (void *)&sys_para.ana.exceed_max_range}}, // EIHH
    {.error_flags   = ERR_INTERNAL_LOW,
     .error_index   = 4,
     .err_text      = {0x79, 0x0F, 0x3E, 0x3E, 0x00},
     .analog_output = {(void *)&min_0_20ma, (void *)&sys_para.ana.exceed_min_range}}, // EIUU
    {.error_flags   = ERR_ENERGY_LOW,
     .error_index   = 5,
     .err_text      = {0x79, 0x3E, 0x3E, 0x3E, 0x00},
     .analog_output = {(void *)&min_0_20ma, (void *)&sys_para.ana.exceed_min_range}}, // EUUU
    {.error_flags   = ERR_ATTEN_HIGH,
     .error_index   = 6,
     .err_text      = {0x79, 0x77, 0x77, 0x77, 0x00},
     .analog_output = {(void *)&min_0_20ma, (void *)&sys_para.ana.exceed_min_range}}, // EAAA
};
uint16_t error_flags = 0xFFFF;

static void exti_fun(void);
/*
 * 功能描述：这个函数处理设备开机自检
 * 入参说明：无
 * 返 回 值：无
 */
void dev_self_check(void) {
    memset(sys_state.dev_error, DEV_ST_ERR, (sizeof(char) * DEV_ERR_CNT));
    /* 自检EEPROM */
    if (eeprom_test()) {
        dbg_info("%s\r\n", "eeprom test failed!");
        ENTER_CRITICAL();
        sys_state.dev_error[DEV_ERR_EEPROM] = DEV_ST_ERR;
        EXIT_CRITICAL();
    } else {
        sys_state.dev_error[DEV_ERR_EEPROM] = DEV_ST_OK;
    }

    /*spipsram自检测试 vti7064x*/
    if (!vti7064x_init()) {
        dbg_info("%s\r\n", "spipsram vti7064 initialized failed!");
        ENTER_CRITICAL();
        sys_state.dev_error[DEV_ERR_PSRAM] = DEV_ST_ERR;
        EXIT_CRITICAL();
    } else {
        sys_state.dev_error[DEV_ERR_PSRAM] = DEV_ST_OK;
    }

    /*adc自检测试 ad7124*/
    /* 初始化AD7124 */
    if (ad7124_init(sys_para.adc)) {
        dbg_info("%s\r\n", "AD7124 initialized failed!");
        ENTER_CRITICAL();
        sys_state.dev_error[DEV_ERR_AD7124] = DEV_ST_ERR;
        EXIT_CRITICAL();
    } else {
        sys_state.dev_error[DEV_ERR_AD7124] = DEV_ST_OK;
    }
    /* dac IIC通信，初始化无返回结果，暂不检查，运行中检查*/

    //    /*摄像头运行时自检*/
    //    if (ov5640_init()) {
    //        sys_state.dev_error[DEV_ERR_OV5640] = DEV_ST_ERR;
    //    }

    /* 打印设备开机自检结果 */
    dbg_self_check_info();
}
/*
 * 功能描述：这个函数处理设备外部触发的相关IO初始化并配置中断
 * 入参说明：无
 * 返 回 值：无
 */
void Extern_Triger_GPIO_config(void) {
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_SYSCFG);

    gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_0);

    nvic_irq_enable(EXTI0_IRQn, 1U, 0U);

    syscfg_exti_line_config(EXTI_SOURCE_GPIOB, EXTI_SOURCE_PIN0);
    exti_init(EXTI_0, EXTI_INTERRUPT, EXTI_TRIG_RISING);

    exti_interrupt_flag_clear(EXTI_0);

    gd32_setup_isr(EXTI0_IRQn, exti_fun, 0);
    gd32_setup_irq(EXTI0_IRQn, 0);
}

/*
 * 功能描述：这个函数是外部触发功能的终端服务函数
 * 入参说明：无
 * 返 回 值：无
 */
bool        exti_triger_flag = false;
static void exti_fun(void) {
    /*处理外部触发功能——峰谷值保持*/
    exti_interrupt_flag_clear(EXTI_0);
    exti_triger_flag = true;
}

/*
 * 功能描述：这个函数是自检功能开启后对自检结果信息的打印函数
 * 入参说明：无
 * 返 回 值：无
 */
void dbg_self_check_info(void) {
    /* 打印设备自检结果 */
    dbg_info("%s", "Device self check result: ");
    for (int i = 0; i < DEV_ERR_CNT; i++) {
        dbg_info("%c ", sys_state.dev_error[i]);
    }
    dbg_info("%s", "\r\n");
}
/*******************************************************************************************************/
// 获取当前最高优先级的错误索引
int get_highest_error_index(void) {
    if (error_flags & ERR_DETECTOR_FAULT) return 0;
    if (error_flags & ERR_HEATER_HIGH) return 1;
    if (error_flags & ERR_HEATER_LOW) return 2;
    if (error_flags & ERR_INTERNAL_HIGH) return 3;
    if (error_flags & ERR_INTERNAL_LOW) return 4;
    if (error_flags & ERR_ENERGY_LOW) return 5;
    if (error_flags & ERR_ATTEN_HIGH) return 6;
    return -1;
}
void error_state_set(ErrorFlag_t err_flag) {
    error_flags = (error_flags | err_flag);
}
void error_state_clear(ErrorFlag_t err_flag) {
    error_flags = (error_flags & (~err_flag));
}

void error_state_upgrade(void) {
    /*测量温度*/
    // 耦合在get_temp上报温度处，快速响应
    /*探测器故障--探测电压为0*/
    if ((sys_para.temp.temp_mode == 2) && ((sys_state.target_volt[SENS_CH_SPEC_WB] == 0) || (sys_state.target_volt[SENS_CH_SPEC_NB] == 0))) {
        /*双色模式一个错都错*/
        error_state_set(ERR_DETECTOR_FAULT);
    } else if ((sys_para.temp.temp_mode == 0) && (sys_state.target_volt[SENS_CH_SPEC_WB] == 0)) {
        /*单色模式针对错*/
        error_state_set(ERR_DETECTOR_FAULT);
    } else if ((sys_para.temp.temp_mode == 1) && (sys_state.target_volt[SENS_CH_SPEC_NB] == 0)) {
        /*单色模式针对错*/
        error_state_set(ERR_DETECTOR_FAULT);
    } else {
        error_state_clear(ERR_DETECTOR_FAULT);
    }

    /*加热器*/
    if (sys_para.heater.set_temp != 0) {
        if (sys_state.internal_temp[SENS_CH_NTC_SPEC] > 80) {
            error_state_set(ERR_HEATER_HIGH);
            error_state_clear(ERR_HEATER_LOW);

        } else if (sys_state.internal_temp[SENS_CH_NTC_SPEC] < (sys_para.heater.set_temp - 20)) {
            error_state_set(ERR_HEATER_LOW);
            error_state_clear(ERR_HEATER_HIGH);
        } else {
            error_state_clear(ERR_HEATER_LOW);
            error_state_clear(ERR_HEATER_HIGH);
        }
        //        /*内部温度*/
        //        if (sys_state.internal_temp[SENS_CH_NTC_TIA] > 80) {
        //            error_state_set(ERR_INTERNAL_HIGH);
        //            error_state_clear(ERR_INTERNAL_LOW);

        //        } else if (sys_state.internal_temp[SENS_CH_NTC_TIA] < 10) {
        //            error_state_set(ERR_INTERNAL_LOW);
        //            error_state_clear(ERR_INTERNAL_HIGH);

        //        } else {
        //            error_state_clear(ERR_INTERNAL_LOW);
        //            error_state_clear(ERR_INTERNAL_HIGH);
        //        }
    } else {
        error_state_clear(ERR_HEATER_LOW);
        error_state_clear(ERR_HEATER_HIGH);
        error_state_clear(ERR_INTERNAL_LOW);
        error_state_clear(ERR_INTERNAL_HIGH);
    }

    /*衰减过高*/
    if ((sys_state.Measured_attenuation * 100) > sys_para.alarm.decay_rate) {
        error_state_set(ERR_ATTEN_HIGH);
    } else {
        error_state_clear(ERR_ATTEN_HIGH);
    }
}
