/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_timer.c
 * 文件描述：TIMER-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/12/05 陈军  增加超时定时器功能
 *           V1.2 2025/02/17 李超  超时定时器时基修正
 */

#include "gd32_timer.h"

/*
 * 功能描述：此函数将定时器初始化为通用周期性定时器
 * 入参说明：dev --- TIMERx（x=0至13）
 *           prescale --- 时钟预分频值，1至65536
 *           period --- 计数周期，1至65536
 * 返 回 值：无
 */
void gd32_timer_init(uint32_t dev, int prescale, int period) {
    timer_parameter_struct para;

    /* 启动定时器外设 */
    if (dev == TIMER0) {
        rcu_periph_clock_enable(RCU_TIMER0);
    } else if (dev == TIMER1) {
        rcu_periph_clock_enable(RCU_TIMER1);
    } else if (dev == TIMER2) {
        rcu_periph_clock_enable(RCU_TIMER2);
    } else if (dev == TIMER3) {
        rcu_periph_clock_enable(RCU_TIMER3);
    } else if (dev == TIMER4) {
        rcu_periph_clock_enable(RCU_TIMER4);
    } else if (dev == TIMER5) {
        rcu_periph_clock_enable(RCU_TIMER5);
    } else if (dev == TIMER6) {
        rcu_periph_clock_enable(RCU_TIMER6);
    } else if (dev == TIMER7) {
        rcu_periph_clock_enable(RCU_TIMER7);
    } else if (dev == TIMER8) {
        rcu_periph_clock_enable(RCU_TIMER8);
    } else if (dev == TIMER9) {
        rcu_periph_clock_enable(RCU_TIMER9);
    } else if (dev == TIMER10) {
        rcu_periph_clock_enable(RCU_TIMER10);
    } else if (dev == TIMER11) {
        rcu_periph_clock_enable(RCU_TIMER11);
    } else if (dev == TIMER12) {
        rcu_periph_clock_enable(RCU_TIMER12);
    } else if (dev == TIMER13) {
        rcu_periph_clock_enable(RCU_TIMER13);
    }

    timer_deinit(dev);

    /* 参数 */
    timer_struct_para_init(&para);
    para.alignedmode       = TIMER_COUNTER_EDGE;
    para.counterdirection  = TIMER_COUNTER_UP;
    para.clockdivision     = TIMER_CKDIV_DIV1;
    para.repetitioncounter = 0U;
    para.prescaler         = (prescale - 1) & 0x0ffff;
    para.period            = (period - 1) & 0x0ffff;

    /* 配置 */
    timer_init(dev, &para);
    timer_update_event_enable(dev);
    timer_auto_reload_shadow_enable(dev);
}

/*
 * 功能描述：此函数关闭定时器
 * 入参说明：dev --- TIMERx（x=0至13）
 * 返 回 值：无
 */
void gd32_timer_shutdown(uint32_t dev) {
    /* 停止定时器外设 */
    if (dev == TIMER0) {
        rcu_periph_clock_disable(RCU_TIMER0);
    } else if (dev == TIMER1) {
        rcu_periph_clock_disable(RCU_TIMER1);
    } else if (dev == TIMER2) {
        rcu_periph_clock_disable(RCU_TIMER2);
    } else if (dev == TIMER3) {
        rcu_periph_clock_disable(RCU_TIMER3);
    } else if (dev == TIMER4) {
        rcu_periph_clock_disable(RCU_TIMER4);
    } else if (dev == TIMER5) {
        rcu_periph_clock_disable(RCU_TIMER5);
    } else if (dev == TIMER6) {
        rcu_periph_clock_disable(RCU_TIMER6);
    } else if (dev == TIMER7) {
        rcu_periph_clock_disable(RCU_TIMER7);
    } else if (dev == TIMER8) {
        rcu_periph_clock_disable(RCU_TIMER8);
    } else if (dev == TIMER9) {
        rcu_periph_clock_disable(RCU_TIMER9);
    } else if (dev == TIMER10) {
        rcu_periph_clock_disable(RCU_TIMER10);
    } else if (dev == TIMER11) {
        rcu_periph_clock_disable(RCU_TIMER11);
    } else if (dev == TIMER12) {
        rcu_periph_clock_disable(RCU_TIMER12);
    } else if (dev == TIMER13) {
        rcu_periph_clock_disable(RCU_TIMER13);
    }
}

/*
 * 功能描述：此函数获取定时器的外设时钟频率
 * 入参说明：dev --- TIMERx（x=0至13）
 * 返 回 值：时钟频率（单位：赫兹），0 表示无效
 */
uint32_t gd32_timer_clk_freq(uint32_t dev) {
    uint32_t freq;

    if ((dev == TIMER0) || (dev == TIMER7) || (dev == TIMER8) || (dev == TIMER9) || (dev == TIMER10)) {
        freq = rcu_clock_freq_get(CK_APB2);
        freq = freq << 1;
    } else {
        freq = rcu_clock_freq_get(CK_APB1);
        freq = freq << 1;
    }

    return freq;
}

/*
 * 功能描述：此函数初始化接收超时定时器
 * 入参说明：timer --- 接收超时定时器外设
 *           timeo --- 接收超时时间，以毫秒为单位
 *           handler --- 定时器中断服务程序
 * 返 回 值：无
 */
void timer_timeo_init(uint32_t timer, uint32_t timeo, isr_handler_t handler) {
    int scale;

    /* 时钟周期1ms */
    scale = gd32_timer_clk_freq(timer) / 10000u;
    gd32_timer_init(timer, scale, timeo * 10);

    if (timer == TIMER0) {
        gd32_setup_isr(TIMER0_UP_TIMER9_IRQn, handler, 0);
        gd32_setup_irq(TIMER0_UP_TIMER9_IRQn, 15);
    } else if (timer == TIMER1) {
        gd32_setup_isr(TIMER1_IRQn, handler, 0);
        gd32_setup_irq(TIMER1_IRQn, 15);
    } else if (timer == TIMER2) {
        gd32_setup_isr(TIMER2_IRQn, handler, 0);
        gd32_setup_irq(TIMER2_IRQn, 15);
    } else if (timer == TIMER3) {
        gd32_setup_isr(TIMER3_IRQn, handler, 0);
        gd32_setup_irq(TIMER3_IRQn, 15);
    } else if (timer == TIMER4) {
        gd32_setup_isr(TIMER4_IRQn, handler, 0);
        gd32_setup_irq(TIMER4_IRQn, 15);
    } else if (timer == TIMER5) {
        gd32_setup_isr(TIMER5_DAC_IRQn, handler, 0);
        gd32_setup_irq(TIMER5_DAC_IRQn, 15);
    } else if (timer == TIMER6) {
        gd32_setup_isr(TIMER6_IRQn, handler, 0);
        gd32_setup_irq(TIMER6_IRQn, 15);
    } else if (timer == TIMER7) {
        gd32_setup_isr(TIMER7_UP_TIMER12_IRQn, handler, 0);
        gd32_setup_irq(TIMER7_UP_TIMER12_IRQn, 15);
    } else if (timer == TIMER8) {
        gd32_setup_isr(TIMER0_BRK_TIMER8_IRQn, handler, 0);
        gd32_setup_irq(TIMER0_BRK_TIMER8_IRQn, 15);
    } else if (timer == TIMER9) {
        gd32_setup_isr(TIMER0_UP_TIMER9_IRQn, handler, 0);
        gd32_setup_irq(TIMER0_UP_TIMER9_IRQn, 15);
    } else if (timer == TIMER10) {
        gd32_setup_isr(TIMER0_TRG_CMT_TIMER10_IRQn, handler, 0);
        gd32_setup_irq(TIMER0_TRG_CMT_TIMER10_IRQn, 15);
    } else if (timer == TIMER11) {
        gd32_setup_isr(TIMER7_BRK_TIMER11_IRQn, handler, 0);
        gd32_setup_irq(TIMER7_BRK_TIMER11_IRQn, 15);
    } else if (timer == TIMER12) {
        gd32_setup_isr(TIMER7_UP_TIMER12_IRQn, handler, 0);
        gd32_setup_irq(TIMER7_UP_TIMER12_IRQn, 15);
    } else if (timer == TIMER13) {
        gd32_setup_isr(TIMER7_TRG_CMT_TIMER13_IRQn, handler, 0);
        gd32_setup_irq(TIMER7_TRG_CMT_TIMER13_IRQn, 15);
    }
}

void pwm_timer_init(rcu_periph_enum timer_rcu, uint32_t timer, uint16_t period, uint16_t channel, uint32_t pulse) {

    /* -----------------------------------------------------------------------
    TIMER1 configuration: generate 1 PWM signals
    TIMER1CLK = SystemCoreClock / (239 + 1) = 1MHz

    TIMER1 channel1 duty cycle = (4000/ 16000)* 100  = 25%
    ----------------------------------------------------------------------- */
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct    timer_initpara;

    rcu_periph_clock_enable(timer_rcu);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);

    timer_deinit(TIMER1);

    /* TIMER1 configuration */
    timer_initpara.prescaler         = 239;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 5000;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER1, &timer_initpara);

    /* CH1,CH2 and CH3 configuration in PWM mode */
    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

    timer_channel_output_config(TIMER1, TIMER_CH_0, &timer_ocintpara);

    /* CH1 configuration in PWM mode1,duty cycle 25% */
    timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, 0);
    timer_channel_output_mode_config(TIMER1, TIMER_CH_0, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER1, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);

    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER1);
    /* auto-reload preload enable */
    timer_enable(TIMER1);
}

/*
 * 功能描述：Timer PWM输出高电平占空比调整
 * 入参说明：
 *      timer_periph: 定时器外设编号
 *      channel: 通道编号
 *      duty: 占空比，取值范围为0~PWM_PERIOD，调用时应提前校验
 * 返 回 值：无
 */
void timer_set_dutyCycle(uint32_t timer_periph, uint16_t channel, int duty) {
    timer_channel_output_pulse_value_config(timer_periph, channel, (uint32_t)duty);
}
