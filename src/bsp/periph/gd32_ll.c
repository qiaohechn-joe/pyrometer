/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_ll.c
 * 文件描述：LL-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/06/06 陈军  向量表偏移设置函数nvic_vector_table_set()移除
 */

#include "gd32_ll.h"

/* NVIC 优先级分组
   3 = 预优先级：4位  子优先级：0位
   4 = 预优先级：3位  子优先级：1位
   5 = 预优先级：2位  子优先级：2位
   6 = 预优先级：1位  子优先级：3位
   7 = 预优先级：0位  子优先级：4位
*/
#define NVIC_PRIO_GRP 3

/* 各型号最大支持ISR数量 */
#if defined(GD32F450) || defined(GD32F470)
#define MAX_ISR_CNT 91
#endif

#if defined(GD32F405) || defined(GD32F425)
#define MAX_ISR_CNT 82
#endif

#if defined(GD32F407) || defined(GD32F427)
#define MAX_ISR_CNT 82
#endif

/* 每个ISR的处理程序计数 */
#define ISR_HANDLER_CNT 2

/* 处理程序数组，每个ISR有2个处理程序 */
static isr_handler_t isr_handlers[MAX_ISR_CNT][ISR_HANDLER_CNT] = {0};

/* 延迟常数 */
static uint32_t delay_const;

#define DEFINE_ISR(n)                                                                                                                                \
    void n##_IRQHandler(void) {                                                                                                                      \
        isr_handler_t handler;                                                                                                                       \
        handler = isr_handlers[n##_IRQn][0];                                                                                                         \
        if (handler) handler();                                                                                                                      \
        handler = isr_handlers[n##_IRQn][1];                                                                                                         \
        if (handler) handler();                                                                                                                      \
    }

/*
 * 功能描述：该函数是gd32底层初始化代码
 * 入参说明：无
 * 返 回 值：无
 * 备注说明：在main()函数中操作系统前调用
 */
void gd32_ll_init(void) {
    uint32_t freq;

    NVIC_SetPriorityGrouping(NVIC_PRIO_GRP); /* 优先级分组 */

    /* 使能PMU SYSCFG TCMSRAM */
    rcu_periph_clock_enable(RCU_PMU);
    rcu_periph_clock_enable(RCU_SYSCFG);
    rcu_periph_clock_enable(RCU_TCMSRAM);

    /* 更新SYSCLK频率 */
    SystemCoreClockUpdate();

    /* 延时定时器 */
    freq          = rcu_clock_freq_get(CK_AHB);
    delay_const   = freq / 1000000;
    SysTick->CTRL = (1 << 2);                        /* 时钟等于HCLK（系统时钟）,BIT2 */
    SysTick->LOAD = (freq / LL_DLY_TIMER_TICKS) - 1; /* 重载值 */
    SysTick->CTRL |= (1 << 0);                       /* 使能定时器,BIT1 */
}

/*
 * 功能描述：该函数设置一个中断的优先级，然后启用它
 * 入参说明：nr --- 中断号，XXX_IRQn（在 "gd32f4xx.h" 中定义）
 *           prio --- 优先级，0（最高）到15（最低）
 * 返 回 值：无
 * 备注说明：仅用于中断（nr > 0）
 */
void gd32_setup_irq(IRQn_Type nr, int prio) {
    uint32_t val;

    NVIC_DisableIRQ(nr);

    val = NVIC_EncodePriority(NVIC_PRIO_GRP, prio, 0);
    NVIC_SetPriority(nr, val);

    NVIC_EnableIRQ(nr);
}

/*
 * 功能描述：该函数为ISR设置一个处理函数
 * 入参说明：nr --- 中断号，xxx_IRQn
 *           handler --- 要调用的处理函数
 *           order --- 处理函数的调用顺序，0到ISR_HANDLER_CNT-1
 * 返 回 值：无
 */
void gd32_setup_isr(IRQn_Type nr, isr_handler_t handler, int order) {
    if ((nr < 0) || (nr >= MAX_ISR_CNT)) {
        return;
    }
    if ((order < 0) || (order >= ISR_HANDLER_CNT)) {
        return;
    }

    isr_handlers[nr][order] = handler;
}

/*
 * 功能描述：该函数延迟若干微秒，使用SysTick定时器。
 * 入参说明：us --- 要延迟的微秒数
 * 返 回 值：无
 * 备注说明：实际的延迟时间可能会更长
 */
void gd32_delay_us(uint32_t us) {
    uint32_t remain;
    uint32_t period;
    uint32_t prev;
    uint32_t curr;
    uint32_t delta;

    remain = us * delay_const;
    period = SysTick->LOAD + 1;
    prev   = SysTick->VAL;

    if (us == 0) {
        return;
    }

    while (1) {
        curr = SysTick->VAL;
        if (curr == prev) {
            continue;
        }

        delta = (curr < prev) ? (prev - curr) : (prev + period - curr);
        if (remain <= delta) {
            break;
        }
        remain -= delta;
        prev = curr;
    }
}

/* 通用部分 */
DEFINE_ISR(WWDGT)
DEFINE_ISR(LVD)
DEFINE_ISR(TAMPER_STAMP)
DEFINE_ISR(RTC_WKUP)
DEFINE_ISR(FMC)
DEFINE_ISR(RCU_CTC)
DEFINE_ISR(EXTI0)
DEFINE_ISR(EXTI1)
DEFINE_ISR(EXTI2)
DEFINE_ISR(EXTI3)
DEFINE_ISR(EXTI4)
DEFINE_ISR(DMA0_Channel0)
DEFINE_ISR(DMA0_Channel1)
DEFINE_ISR(DMA0_Channel2)
DEFINE_ISR(DMA0_Channel3)
DEFINE_ISR(DMA0_Channel4)
DEFINE_ISR(DMA0_Channel5)
DEFINE_ISR(DMA0_Channel6)
DEFINE_ISR(ADC)
DEFINE_ISR(CAN0_TX)
DEFINE_ISR(CAN0_RX0)
DEFINE_ISR(CAN0_RX1)
DEFINE_ISR(CAN0_EWMC)
DEFINE_ISR(EXTI5_9)
DEFINE_ISR(TIMER0_BRK_TIMER8)
DEFINE_ISR(TIMER0_UP_TIMER9)
DEFINE_ISR(TIMER0_TRG_CMT_TIMER10)
DEFINE_ISR(TIMER0_Channel)
DEFINE_ISR(TIMER1)
DEFINE_ISR(TIMER2)
DEFINE_ISR(TIMER3)
DEFINE_ISR(I2C0_EV)
DEFINE_ISR(I2C0_ER)
DEFINE_ISR(I2C1_EV)
DEFINE_ISR(I2C1_ER)
DEFINE_ISR(SPI0)
DEFINE_ISR(SPI1)
DEFINE_ISR(USART0)
DEFINE_ISR(USART1)
DEFINE_ISR(USART2)
DEFINE_ISR(EXTI10_15)
DEFINE_ISR(RTC_Alarm)
DEFINE_ISR(USBFS_WKUP)
DEFINE_ISR(TIMER7_BRK_TIMER11)
DEFINE_ISR(TIMER7_UP_TIMER12)
DEFINE_ISR(TIMER7_TRG_CMT_TIMER13)
DEFINE_ISR(TIMER7_Channel)
DEFINE_ISR(DMA0_Channel7)

/* GD32F450  GD32F470 */
#if defined(GD32F450) || defined(GD32F470)
DEFINE_ISR(EXMC)
DEFINE_ISR(SDIO)
DEFINE_ISR(TIMER4)
DEFINE_ISR(SPI2)
DEFINE_ISR(UART3)
DEFINE_ISR(UART4)
DEFINE_ISR(TIMER5_DAC)
DEFINE_ISR(TIMER6)
DEFINE_ISR(DMA1_Channel0)
DEFINE_ISR(DMA1_Channel1)
DEFINE_ISR(DMA1_Channel2)
DEFINE_ISR(DMA1_Channel3)
DEFINE_ISR(DMA1_Channel4)
DEFINE_ISR(ENET)
DEFINE_ISR(ENET_WKUP)
DEFINE_ISR(CAN1_TX)
DEFINE_ISR(CAN1_RX0)
DEFINE_ISR(CAN1_RX1)
DEFINE_ISR(CAN1_EWMC)
// DEFINE_ISR(USBFS)
DEFINE_ISR(DMA1_Channel5)
DEFINE_ISR(DMA1_Channel6)
DEFINE_ISR(DMA1_Channel7)
DEFINE_ISR(USART5)
DEFINE_ISR(I2C2_EV)
DEFINE_ISR(I2C2_ER)
DEFINE_ISR(USBHS_EP1_Out)
DEFINE_ISR(USBHS_EP1_In)
DEFINE_ISR(USBHS_WKUP)
DEFINE_ISR(USBHS)
DEFINE_ISR(DCI)
DEFINE_ISR(TRNG)
DEFINE_ISR(FPU)
DEFINE_ISR(UART6)
DEFINE_ISR(UART7)
DEFINE_ISR(SPI3)
DEFINE_ISR(SPI4)
DEFINE_ISR(SPI5)
DEFINE_ISR(TLI)
DEFINE_ISR(TLI_ER)
DEFINE_ISR(IPA)
#endif

/* GD32F405  GD32F425 */
#if defined(GD32F405) || defined(GD32F425)
DEFINE_ISR(SDIO)
DEFINE_ISR(TIMER4)
DEFINE_ISR(SPI2)
DEFINE_ISR(UART3)
DEFINE_ISR(UART4)
DEFINE_ISR(TIMER5_DAC)
DEFINE_ISR(TIMER6)
DEFINE_ISR(DMA1_Channel0)
DEFINE_ISR(DMA1_Channel1)
DEFINE_ISR(DMA1_Channel2)
DEFINE_ISR(DMA1_Channel3)
DEFINE_ISR(DMA1_Channel4)
DEFINE_ISR(CAN1_TX)
DEFINE_ISR(CAN1_RX0)
DEFINE_ISR(CAN1_RX1)
DEFINE_ISR(CAN1_EWMC)
DEFINE_ISR(USBFS)
DEFINE_ISR(DMA1_Channel5)
DEFINE_ISR(DMA1_Channel6)
DEFINE_ISR(DMA1_Channel7)
DEFINE_ISR(USART5)
DEFINE_ISR(I2C2_EV)
DEFINE_ISR(I2C2_ER)
DEFINE_ISR(USBHS_EP1_Out)
DEFINE_ISR(USBHS_EP1_In)
DEFINE_ISR(USBHS_WKUP)
DEFINE_ISR(USBHS)
DEFINE_ISR(DCI)
DEFINE_ISR(TRNG)
DEFINE_ISR(FPU)
#endif

/* GD32F407  GD32F427 */
#if defined(GD32F407) || defined(GD32F427)
DEFINE_ISR(EXMC)
DEFINE_ISR(SDIO)
DEFINE_ISR(TIMER4)
DEFINE_ISR(SPI2)
DEFINE_ISR(UART3)
DEFINE_ISR(UART4)
DEFINE_ISR(TIMER5_DAC)
DEFINE_ISR(TIMER6)
DEFINE_ISR(DMA1_Channel0)
DEFINE_ISR(DMA1_Channel1)
DEFINE_ISR(DMA1_Channel2)
DEFINE_ISR(DMA1_Channel3)
DEFINE_ISR(DMA1_Channel4)
DEFINE_ISR(ENET)
DEFINE_ISR(ENET_WKUP)
DEFINE_ISR(CAN1_TX)
DEFINE_ISR(CAN1_RX0)
DEFINE_ISR(CAN1_RX1)
DEFINE_ISR(CAN1_EWMC)
// DEFINE_ISR(USBFS)
DEFINE_ISR(DMA1_Channel5)
DEFINE_ISR(DMA1_Channel6)
DEFINE_ISR(DMA1_Channel7)
DEFINE_ISR(USART5)
DEFINE_ISR(I2C2_EV)
DEFINE_ISR(I2C2_ER)
DEFINE_ISR(USBHS_EP1_Out)
DEFINE_ISR(USBHS_EP1_In)
DEFINE_ISR(USBHS_WKUP)
DEFINE_ISR(USBHS)
DEFINE_ISR(DCI)
DEFINE_ISR(TRNG)
DEFINE_ISR(FPU)
#endif
