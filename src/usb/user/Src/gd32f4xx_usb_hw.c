/*!
    \file    gd32f4xx_usb_hw.c
    \brief   this file implements the board support package for the USB host library

    \version 2024-01-15, V3.2.0, firmware for GD32F4xx
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "drv_usb_hw.h"
#include "drv_usb_core.h"
#include "drv_usbh_int.h"
#include "usbh_usr.h"
#include "usbh_video.h"

//#include "usart.h"

#define TIM_MSEC_DELAY                          0x01
#define TIM_USEC_DELAY                          0x02

#define HOST_OVRCURR_PORT                       GPIOE
#define HOST_OVRCURR_LINE                       GPIO_PIN_1
#define HOST_OVRCURR_PORT_SOURCE                GPIO_PORT_SOURCE_GPIOE
#define HOST_OVRCURR_PIN_SOURCE                 GPIO_PINSOURCE1
#define HOST_OVRCURR_PORT_RCC                   RCC_APB2PERIPH_GPIOE
#define HOST_OVRCURR_EXTI_LINE                  EXTI_LINE1
#define HOST_OVRCURR_IRQn                       EXTI1_IRQn

#define HOST_POWERSW_PORT_RCC                   RCU_GPIOD
#define HOST_POWERSW_PORT                       GPIOD
#define HOST_POWERSW_VBUS                       GPIO_PIN_13

#define HOST_SOF_OUTPUT_RCC                     RCC_APB2PERIPH_GPIOA
#define HOST_SOF_PORT                           GPIOA
#define HOST_SOF_SIGNAL                         GPIO_PIN_8

__IO uint32_t delay_time = 0;
__IO uint16_t timer_prescaler = 5;

TaskHandle_t os_data_send_handle;
SemaphoreHandle_t xSendSemaphore = NULL;

/* local function prototypes ('static') */
static void hwp_time_set         (uint8_t unit);
//static void hwp_delay            (uint32_t ntime, uint8_t unit);

/*!
    \brief      configure USB GPIO
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usb_gpio_config (void)
{
    rcu_periph_clock_enable(RCU_SYSCFG);

#ifdef USE_USB_FS

    rcu_periph_clock_enable(RCU_GPIOA);

    /* USBFS_DM(PA11) and USBFS_DP(PA12) GPIO pin configuration */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_11 | GPIO_PIN_12);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_11 | GPIO_PIN_12);

    gpio_af_set(GPIOA, GPIO_AF_10, GPIO_PIN_11 | GPIO_PIN_12);

#elif defined(USE_USB_HS)

    #ifdef USE_ULPI_PHY
//        rcu_periph_clock_enable(RCU_GPIOA);
//        rcu_periph_clock_enable(RCU_GPIOB);
//        rcu_periph_clock_enable(RCU_GPIOC);
//        rcu_periph_clock_enable(RCU_GPIOH);
//        rcu_periph_clock_enable(RCU_GPIOI);

//        /* ULPI_STP(PC0) GPIO pin configuration */
//        gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0);
//        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_0);

//        /* ULPI_CK(PA5) GPIO pin configuration */
//        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
//        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_5);

//        /* ULPI_NXT(PH4) GPIO pin configuration */
//        gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4);
//        gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_4);

//        /* ULPI_DIR(PI11) GPIO pin configuration */
//        gpio_mode_set(GPIOI, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_11);
//        gpio_output_options_set(GPIOI, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_11);

//        /* ULPI_D1(PB0), ULPI_D2(PB1), ULPI_D3(PB10), ULPI_D4(PB11) \
//           ULPI_D5(PB12), ULPI_D6(PB13) and ULPI_D7(PB5) GPIO pin configuration */
//        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, \
//                        GPIO_PIN_5 | GPIO_PIN_13 | GPIO_PIN_12 |\
//                        GPIO_PIN_11 | GPIO_PIN_10 | GPIO_PIN_1 | GPIO_PIN_0);
//        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, \
//                        GPIO_PIN_5 | GPIO_PIN_13 | GPIO_PIN_12 |\
//                        GPIO_PIN_11 | GPIO_PIN_10 | GPIO_PIN_1 | GPIO_PIN_0);

//        /* ULPI_D0(PA3) GPIO pin configuration */
//        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
//        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_3);

//        gpio_af_set(GPIOC, GPIO_AF_10, GPIO_PIN_0);
//        gpio_af_set(GPIOH, GPIO_AF_10, GPIO_PIN_4);
//        gpio_af_set(GPIOI, GPIO_AF_10, GPIO_PIN_11);
//        gpio_af_set(GPIOA, GPIO_AF_10, GPIO_PIN_5 | GPIO_PIN_3);
//        gpio_af_set(GPIOB, GPIO_AF_10, GPIO_PIN_5 | GPIO_PIN_13 | GPIO_PIN_12 |\
//                                       GPIO_PIN_11 | GPIO_PIN_10 | GPIO_PIN_1 | GPIO_PIN_0);

        rcu_periph_clock_enable(RCU_GPIOA);
        rcu_periph_clock_enable(RCU_GPIOB);
        rcu_periph_clock_enable(RCU_GPIOC);
        rcu_periph_clock_enable(RCU_GPIOE);

        /* ULPI_STP(PC0) GPIO pin configuration */
        gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0);
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_0);

        /* ULPI_CK(PA5) GPIO pin configuration */
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_5);

        /* ULPI_NXT(PC3) GPIO pin configuration */
        gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_3);

        /* ULPI_DIR(PC2) GPIO pin configuration */
        gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_2);
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_2);
        
        /* ULPI_D0(PA3) GPIO pin configuration */
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_3);
        
        /* ULPI_D1(PB0), ULPI_D2(PB1), ULPI_D3(PB10), ULPI_D4(PB11) \
           ULPI_D5(PB12), ULPI_D6(PB13) and ULPI_D7(PB5) GPIO pin configuration */
        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, \
                        GPIO_PIN_5 | GPIO_PIN_13 | GPIO_PIN_12 |\
                        GPIO_PIN_11 | GPIO_PIN_10 | GPIO_PIN_1 | GPIO_PIN_0);
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, \
                        GPIO_PIN_5 | GPIO_PIN_13 | GPIO_PIN_12 |\
                        GPIO_PIN_11 | GPIO_PIN_10 | GPIO_PIN_1 | GPIO_PIN_0);

        gpio_af_set(GPIOC, GPIO_AF_10, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3);
        gpio_af_set(GPIOA, GPIO_AF_10, GPIO_PIN_3 | GPIO_PIN_5);
        gpio_af_set(GPIOB, GPIO_AF_10, GPIO_PIN_5 | GPIO_PIN_13 | GPIO_PIN_12 |\
                                       GPIO_PIN_11 | GPIO_PIN_10 | GPIO_PIN_1 | GPIO_PIN_0);
                                       
        /* Reset GPIO pin configuration */
        gpio_mode_set(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_7);
        gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_7);                                       
                       
        gpio_bit_reset(GPIOE, GPIO_PIN_7);
        for (int i=0; i<0xffff; i++)
        {
            ;
        }
        gpio_bit_set(GPIOE, GPIO_PIN_7);
    #elif defined(USE_EMBEDDED_PHY)
        rcu_periph_clock_enable(RCU_GPIOB);

        /* USBHS_DM(PB14) and USBHS_DP(PB15) GPIO pin configuration */
        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_14 | GPIO_PIN_15);
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_14 | GPIO_PIN_15);
        gpio_af_set(GPIOB, GPIO_AF_12, GPIO_PIN_14 | GPIO_PIN_15);
    #endif /* USE_ULPI_PHY */

#endif /* USE_USBFS */
}

/*!
    \brief      configure USB clock
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usb_rcu_config (void)
{
#ifdef USE_USB_FS
    rcu_pll48m_clock_config(RCU_PLL48MSRC_PLLQ);
    rcu_ck48m_clock_config(RCU_CK48MSRC_PLL48M);

    rcu_periph_clock_enable(RCU_USBFS);
#elif defined(USE_USB_HS)
    #ifdef USE_EMBEDDED_PHY
        rcu_pll48m_clock_config(RCU_PLL48MSRC_PLLQ);
        rcu_ck48m_clock_config(RCU_CK48MSRC_PLL48M);
    #elif defined(USE_ULPI_PHY)
        rcu_periph_clock_enable(RCU_USBHSULPI);
    #endif /* USE_EMBEDDED_PHY */

    rcu_periph_clock_enable(RCU_USBHS);
#endif /* USB_USBFS */
}


/*!
    \brief      this function handles USBFS interrupt Handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void USBFS_IRQHandler(void)
{
    usbh_isr(&usbh_core);
}


/*!
    \brief      configure USB global interrupt
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usb_intr_config (void)
{
//    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);

//      nvic_irq_enable((uint8_t)USBFS_IRQn, 2U, 0U);
      
//          gd32_setup_isr(USBFS_IRQn, handler, 0);
//        gd32_setup_irq(TIMER0_UP_TIMER9_IRQn, 15);
  
#ifdef USE_USB_FS
    nvic_irq_enable((uint8_t)USBFS_IRQn, 2U, 0U);
    #if USBFS_LOW_POWER
        /* enable the power module clock */
        rcu_periph_clock_enable(RCU_PMU);

        /* USB wakeup EXTI line configuration */
        exti_interrupt_flag_clear(EXTI_18);
        exti_init(EXTI_18, EXTI_INTERRUPT, EXTI_TRIG_RISING);
        exti_interrupt_enable(EXTI_18);

        nvic_irq_enable((uint8_t)USBFS_WKUP_IRQn, 0U, 0U);
    #endif /* USBFS_LOW_POWER */
#endif /* USE_USB_FS */

#ifdef USE_USB_HS
    nvic_irq_enable((uint8_t)USBHS_IRQn, 2U, 0U);

    #if USBHS_LOW_POWER
        /* enable the power module clock */
        rcu_periph_clock_enable(RCU_PMU);

        /* USB wakeup EXTI line configuration */
        exti_interrupt_flag_clear(EXTI_20);
        exti_init(EXTI_20, EXTI_INTERRUPT, EXTI_TRIG_RISING);
        exti_interrupt_enable(EXTI_20);

        nvic_irq_enable((uint8_t)USBHS_WKUP_IRQn, 0U, 0U);
    #endif /* USBHS_LOW_POWER */
#endif /* USE_USB_HS */
}

/*!
    \brief      drives the VBUS signal through gpio
    \param[in]  state: VBUS states
    \param[out] none
    \retval     none
*/
void usb_vbus_drive (uint8_t state)
{
    if (0U == state) {
        /* disable the power switch by driving the GPIO low */
        gpio_bit_reset(HOST_POWERSW_PORT, HOST_POWERSW_VBUS);
    } else {
        /* enable the power switch by driving the GPIO high */
        gpio_bit_set(HOST_POWERSW_PORT, HOST_POWERSW_VBUS);
    }
}

/*!
    \brief      configures the GPIO for the VBUS
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usb_vbus_config (void)
{
    rcu_periph_clock_enable(HOST_POWERSW_PORT_RCC);

    /* USBFS_VBUS_CTRL(PD13) GPIO pin configuration */
    gpio_mode_set(HOST_POWERSW_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, HOST_POWERSW_VBUS);
    gpio_output_options_set(HOST_POWERSW_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, HOST_POWERSW_VBUS);

    /* by default, disable is needed on output of the power switch */
    usb_vbus_drive(0U);

    /* delay is need for stabilizing the VBUS low in reset condition, 
     * when VBUS = 1 and reset-button is pressed by user 
     */
    usb_mdelay(200);
}

/*!
    \brief      initializes delay unit using Timer2
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usb_timer_init (void)
{
    /* configure the priority group to 2 bits */
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);

    /* enable the Timer2 global interrupt */
    nvic_irq_enable((uint8_t)TIMER3_IRQn, 1U, 0U);

    rcu_periph_clock_enable(RCU_TIMER3);
}

/*!
    \brief      delay in micro seconds
    \param[in]  usec: value of delay required in micro seconds
    \param[out] none
    \retval     none
*/
void usb_udelay (const uint32_t usec)
{
    //hwp_delay(usec, TIM_USEC_DELAY);
    gd32_delay_us(usec);
}

/*!
    \brief      delay in milli seconds
    \param[in]  msec: value of delay required in milli seconds
    \param[out] none
    \retval     none
*/
void usb_mdelay (const uint32_t msec)
{
//    hwp_delay(msec, TIM_MSEC_DELAY);
    gd32_delay_ms(msec);
}

/*!
    \brief      timer base IRQ
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usb_timer_irq (void)
{
    if (RESET != timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP)) {
        timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);

        if (delay_time > 0x00U){
            delay_time--;
        } else {
            timer_disable(TIMER3);
        }
    }
}

/*!
    \brief      delay routine based on TIMER2
    \param[in]  ntime: delay Time 
    \param[in]  unit: delay Time unit = mili sec / micro sec
    \param[out] none
    \retval     none
*/
//static void hwp_delay(uint32_t ntime, uint8_t unit)
//{
//    delay_time = ntime;
//    hwp_time_set(unit);

//    while(0U != delay_time);

//    timer_disable(TIMER3);
//}

/*!
    \brief      configures TIMER for delay routine based on Timer2
    \param[in]  unit: msec /usec
    \param[out] none
    \retval     none
*/
static void hwp_time_set(uint8_t unit)
{
    timer_parameter_struct  timer_basestructure;

    timer_prescaler = ((rcu_clock_freq_get(CK_APB1)/1000000*2)/12) - 1;

    timer_disable(TIMER3);
    timer_interrupt_disable(TIMER3, TIMER_INT_UP);

    if(unit == TIM_USEC_DELAY) {
        timer_basestructure.period = 11U;
    } else if(unit == TIM_MSEC_DELAY) {
        timer_basestructure.period = 11999U;
    } else {
        /* no operation */
    }

    timer_basestructure.prescaler         = timer_prescaler;
    timer_basestructure.alignedmode       = TIMER_COUNTER_EDGE;
    timer_basestructure.counterdirection  = TIMER_COUNTER_UP;
    timer_basestructure.clockdivision     = TIMER_CKDIV_DIV1;
    timer_basestructure.repetitioncounter = 0U;

    timer_init(TIMER3, &timer_basestructure);

    timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);

    timer_auto_reload_shadow_enable(TIMER3);

    /* timer2 interrupt enable */
    timer_interrupt_enable(TIMER3, TIMER_INT_UP);

    /* timer2 enable counter */
    timer_enable(TIMER3);
}


void usb_host_hard_init(void)
{
//    static TaskHandle_t usb_uvc_handle ;
    usb_gpio_config();
    usb_rcu_config();
//    usb_timer_init();

    /* configure GPIO pin used for switching VBUS power and charge pump I/O */
    usb_vbus_config();

    /* register device class */
    usbh_class_register(&usb_host_uvc, USBH_VIDEO_CLASS);

    /* initialize host library */
    usbh_init(&usb_host_uvc,
              &usbh_core,
#ifdef USE_USB_FS
              USB_CORE_ENUM_FS,
#elif defined(USE_USB_HS)
              USB_CORE_ENUM_HS,
#endif
              &usr_cb);

    /* enable interrupts */
    usb_intr_config ();
    
    //usart1_init();
    
    
    
        /* ≈‰÷√∂® ±∆˜: */
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL2);
    //gd32_timer_init(TIMER11, 100, 100);
    //gd32_timer_init(TIMER11, 100, 75);

    gd32_timer_init(TIMER11, 100, 25);
    gd32_setup_isr(TIMER7_BRK_TIMER11_IRQn, timer11_handler, 0);
    gd32_setup_irq(TIMER7_BRK_TIMER11_IRQn, 10);

    timer_event_software_generate(TIMER11, TIMER_EVENT_SRC_UPG);
    timer_flag_clear(TIMER11, TIMER_FLAG_UP);
    timer_interrupt_enable(TIMER11, TIMER_INT_UP);
//    
//    timer_enable(TIMER11);
    
    ostimer_register(OSTIMER_USB_UVC_TASK, usbh_core_task_test, (void *)&usb_host_uvc, AGENT_PRIO_LO, 30);   //usbh_core_task(&usb_host_uvc);
    xSendSemaphore = xSemaphoreCreateBinary();
    //ostimer_register(OSTIMER_DATA_SEND_TASK, data_send_task, NULL, AGENT_PRIO_MD, 15);     
     //xTaskCreate((TaskFunction_t)data_send_task, (const char *)"data_send_task", (uint16_t)2048,(void *)NULL,(UBaseType_t)28,(TaskHandle_t *)&os_data_send_handle);                                                                                                                                          
}

