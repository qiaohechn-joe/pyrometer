/************************************************ 
* WKS GD32F427VET6核心板
* usbd_hw 驱动代码  
* 版本：V1.0								  
************************************************/	

#include "usbh_hw.h"
#include "usbh_usr.h"
#include "platform.h"
#include "usbh_video.h"
//#include "rs485.h"
//#include "protocol_modbus.h"
//#include "para.h"
#include "ostimer.h"
#include "drv_usbh_int.h"
//#include "coef.h"
//#include "upgrade.h"
//#include "agent.h"

usbh_host usb_host_uvc;

//static void timer1_handler(void) ;
//static void  timer1_init(void);


/*!
    \brief      configure USB clock
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void usb_rcu_config (void)
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
    \brief      configure USB GPIO
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void usb_gpio_config (void)
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
        rcu_periph_clock_enable(RCU_GPIOA);
        rcu_periph_clock_enable(RCU_GPIOB);
        rcu_periph_clock_enable(RCU_GPIOC);
        rcu_periph_clock_enable(RCU_GPIOH);
        rcu_periph_clock_enable(RCU_GPIOI);

        /* ULPI_STP(PC0) GPIO pin configuration */
        gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0);
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_0);

        /* ULPI_CK(PA5) GPIO pin configuration */
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_5);

        /* ULPI_NXT(PH4) GPIO pin configuration */
        gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4);
        gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_4);

        /* ULPI_DIR(PI11) GPIO pin configuration */
        gpio_mode_set(GPIOI, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_11);
        gpio_output_options_set(GPIOI, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_11);

        /* ULPI_D1(PB0), ULPI_D2(PB1), ULPI_D3(PB10), ULPI_D4(PB11) \
           ULPI_D5(PB12), ULPI_D6(PB13) and ULPI_D7(PB5) GPIO pin configuration */
        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, \
                        GPIO_PIN_5 | GPIO_PIN_13 | GPIO_PIN_12 |\
                        GPIO_PIN_11 | GPIO_PIN_10 | GPIO_PIN_1 | GPIO_PIN_0);
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, \
                        GPIO_PIN_5 | GPIO_PIN_13 | GPIO_PIN_12 |\
                        GPIO_PIN_11 | GPIO_PIN_10 | GPIO_PIN_1 | GPIO_PIN_0);

        /* ULPI_D0(PA3) GPIO pin configuration */
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_3);

        gpio_af_set(GPIOC, GPIO_AF_10, GPIO_PIN_0);
        gpio_af_set(GPIOH, GPIO_AF_10, GPIO_PIN_4);
        gpio_af_set(GPIOI, GPIO_AF_10, GPIO_PIN_11);
        gpio_af_set(GPIOA, GPIO_AF_10, GPIO_PIN_5 | GPIO_PIN_3);
        gpio_af_set(GPIOB, GPIO_AF_10, GPIO_PIN_5 | GPIO_PIN_13 | GPIO_PIN_12 |\
                                       GPIO_PIN_11 | GPIO_PIN_10 | GPIO_PIN_1 | GPIO_PIN_0);
    #elif defined(USE_EMBEDDED_PHY)
        rcu_periph_clock_enable(RCU_GPIOB);

        /* USBHS_DM(PB14) and USBHS_DP(PB15) GPIO pin configuration */
        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_14 | GPIO_PIN_15);
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_14 | GPIO_PIN_15);
        gpio_af_set(GPIOB, GPIO_AF_12, GPIO_PIN_14 | GPIO_PIN_15);
    #endif /* USE_ULPI_PHY */

#endif /* USE_USBFS */
}

/**
 * @brief       配置USB中断
 * @param       无
 * @retval      无
 */
void usb_intr_config(void)
{
//    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2); 
    nvic_irq_enable((uint8_t)USBFS_IRQn, 0U, 0U);     /* USBFS中断配置，抢占优先级0，子优先级3 */
	
//	    gd32_setup_isr(LVD_IRQn, lvd_handler, 0);
//    gd32_setup_irq(LVD_IRQn, 1);
}

/**
 * @brief       USBD 延时函数(以us为单位)
 * @param       usec   : 延时的us数
 * @retval      无
 */
void usb_udelay (const uint32_t usec)
{
    //delay_us(usec);
		gd32_delay_us(usec);
}

/**
 * @brief       USBD 延时函数(以ms为单位)
 * @param       msec   : 延时的ms数
 * @retval      无
 */
void usb_mdelay (const uint32_t msec)
{
    //delay_ms(msec);
		gd32_delay_us(msec*1000);
}

/*!
    \brief      this function handles USBFS IRQ Handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void USBFS_IRQHandler(void)
{
     usbh_isr(&usbh_core);
}



void usb_host_mode_init(void)
{
    usb_gpio_config();
    usb_rcu_config();
//    usb_timer_init();

    /* configure GPIO pin used for switching VBUS power and charge pump I/O */
//    usb_vbus_config();

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
              &usr_cb);  //

    /* enable interrupts */
    usb_intr_config ();
		
		ostimer_register(OSTIMER_USBH_TASK, usbh_core_task_test, (void *)&usb_host_uvc, AGENT_PRIO_HI, 10);		
}
