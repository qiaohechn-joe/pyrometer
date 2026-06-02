/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_eth.c
 * 文件描述：网口驱动（基于GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#include "gd32_ll.h"
#include "gd32_timer.h"
#include "gd32_eth.h"
#include "hq_generic.h"
#include "hq_io.h"
#include "lwip/sys.h"
#include "FreeRTOS.h"

/*** PHY寄存器映射 ***/

/* 寄存器地址 */
#define PHY_BMCR            0x00 /* 基本模式控制寄存器 */
#define PHY_BMSR            0x01 /* 基本模式状态寄存器 */
#define PHY_IDR1            0x02 /* PHY标识寄存器1 */
#define PHY_IDR2            0x03 /* PHY标识寄存器2 */
#define PHY_ANAR            0x04 /* 自动协商广告寄存器 */
#define PHY_ANLPAR          0x05 /* 自动协商链路伙伴能力寄存器 */
#define PHY_ANER            0x06 /* 自动协商扩展寄存器 */
#define PHY_STSR            0x10 /* 状态寄存器 */
#define PHY_MICR            0x11 /* MII中断控制寄存器 */
#define PHY_MISR            0x12 /* MII中断状态寄存器 */

/* BMCR位映射 */
#define PHY_BMCR_RESET      (uint16_t)(1 << 15)
#define PHY_BMCR_LPBK       (uint16_t)(1 << 14)
#define PHY_BMCR_SPD_100    (uint16_t)(1 << 13)
#define PHY_BMCR_ANEN       (uint16_t)(1 << 12)
#define PHY_BMCR_PWRDN      (uint16_t)(1 << 11)
#define PHY_BMCR_ISOLATE    (uint16_t)(1 << 10)
#define PHY_BMCR_ANRESTART  (uint16_t)(1 << 9)
#define PHY_BMCR_FULL_DPLX  (uint16_t)(1 << 8)
#define PHY_BMCR_COL_TEST   (uint16_t)(1 << 7)

/* BMSR位映射 */
#define PHY_BMSR_AN_OVER    (uint16_t)(1 << 5) /* 自动协商完成 */
#define PHY_BMSR_LINK_VALID (uint16_t)(1 << 2) /* 连接状态有效 */

/* ANAR位映射 */
#define PHY_ANAR_T4         (uint16_t)(1 << 9)
#define PHY_ANAR_100_FD     (uint16_t)(1 << 8)
#define PHY_ANAR_100        (uint16_t)(1 << 7)
#define PHY_ANAR_10_FD      (uint16_t)(1 << 6)
#define PHY_ANAR_10         (uint16_t)(1 << 5)

#define PHY_ANAR_ALL        (uint16_t)(PHY_ANAR_100_FD | PHY_ANAR_100 | PHY_ANAR_10_FD | PHY_ANAR_10)
#define PHY_ANAR_8023U      (uint16_t)(0x01 << 0) /* 协议：IEEE802.3u */

/* STSR位映射 */
#define PHY_STSR_MII_IRQ    (uint16_t)(1 << 7) /* MII中断挂起位 */
#define PHY_STSR_AN_OVER    (uint16_t)(1 << 4) /* 自动协商完成 */
#define PHY_STSR_LPBK       (uint16_t)(1 << 3) /* 环回（Loopback）模式启用位 */
#define PHY_STSR_FULL_DPLX  (uint16_t)(1 << 2) /* 全双工 */
#define PHY_STSR_SPD_10     (uint16_t)(1 << 1) /* 10Mbps */
#define PHY_STSR_LINK_VALID (uint16_t)(1 << 0) /* 连接状态有效 */

/* MICR位映射 */
#define PHY_MICR_INTEN      (uint16_t)(1 << 1) /* 中断使能 */
#define PHY_MICR_INTOE      (uint16_t)(1 << 0) /* 中断输出使能 */

/* MISR位映射 */
#define PHY_MISR_LINK_INT   (uint16_t)(1 << 13) /* 链路状态变化中断标志 */
#define PHY_MISR_SPD_INT    (uint16_t)(1 << 12) /* 速度状态变化中断标志 */
#define PHY_MISR_DUP_INT    (uint16_t)(1 << 11) /* 双工状态变化中断标志 */
#define PHY_MISR_ANC_INT    (uint16_t)(1 << 10) /* 自动协商完成中断标志 */
#define PHY_MISR_LINK_IE    (uint16_t)(1 << 5)  /* 链路状态变化中断使能 */
#define PHY_MISR_SPD_IE     (uint16_t)(1 << 4)  /* 速度状态变化中断使能 */
#define PHY_MISR_DUP_IE     (uint16_t)(1 << 3)  /* 双工状态变化中断使能 */
#define PHY_MISR_ANC_IE     (uint16_t)(1 << 2)  /* 自动协商完成中断使能 */

static enet_mediamode_enum spd_mode;
static uint8_t             mac_addr[] = {0x0A, 0x00, 0x27, 0x00, 0x00, 0x00};

static void (*rx_callback)(void *arg);   /* 回调函数接收中断调用 */
static void (*link_callback)(void *arg); /* 回调函数连接状态中断调用 */

/* 内部函数 */
static void enet_irq_handler(void);
static void timer6_irq_handler(void);

/*
 * 功能描述：此函数初始化以太网
 * 入参说明：mode --- 速度模式
 *           rx_cb --- 接收数据包时的事件回调函数
 *           link_cb --- 链路状态变化时的事件回调函数
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
int gd32_eth_init(enet_mediamode_enum mode, void (*rx_cb)(void *arg), void (*link_cb)(void *arg)) {
    uint16_t reg;
    int      scale;

    rx_callback   = rx_cb;
    link_callback = link_cb;
    spd_mode      = mode;
    mem_copy(mac_addr + 3, (uint8_t *)(GD32_UID_BASE + 8), 3); /* MAC */

    /* SYSCFG: MII/RMII */
    rcu_periph_clock_enable(RCU_SYSCFG);

#if ETH_RMII_MODE == 1
    syscfg_enet_phy_interface_config(SYSCFG_ENET_PHY_RMII);
#else
    syscfg_enet_phy_interface_config(SYSCFG_ENET_PHY_MII);
#endif

    /* ENET */
    eth_init_pin();
    rcu_periph_clock_enable(RCU_ENET);
    rcu_periph_clock_enable(RCU_ENETTX);
    rcu_periph_clock_enable(RCU_ENETRX);
    enet_deinit();
    enet_software_reset();
#ifdef CHECKSUM_BY_HARDWARE
    enet_init(spd_mode, ENET_AUTOCHECKSUM_DROP_FAILFRAMES, ENET_BROADCAST_FRAMES_PASS);
#else
    enet_init(spd_mode, ENET_NO_AUTOCHECKSUM, ENET_BROADCAST_FRAMES_PASS);
#endif /* CHECKSUM_BY_HARDWARE */

#ifdef SELECT_DESCRIPTORS_ENHANCED_MODE
    enet_desc_select_enhanced_mode();
#endif /* SELECT_DESCRIPTORS_ENHANCED_MODE */

    /* IRQ */
    gd32_setup_isr(ENET_IRQn, enet_irq_handler, 0);
    gd32_setup_irq(ENET_IRQn, 15);
    enet_interrupt_enable(ENET_DMA_INT_NIE);
    enet_interrupt_enable(ENET_DMA_INT_RIE);

    /* PHY连接 */
    reg = PHY_MICR_INTEN | PHY_MICR_INTOE;
    enet_phy_write_read(ENET_PHY_WRITE, PHY_ADDRESS, PHY_MICR, &reg);
    reg = PHY_MISR_LINK_IE;
    enet_phy_write_read(ENET_PHY_WRITE, PHY_ADDRESS, PHY_MISR, &reg);

    /* 连接状态检查定时器 */
    scale = gd32_timer_clk_freq(TIMER6) / 10000u; /* per=0.1ms */
    gd32_timer_init(TIMER6, scale, ETH_LINK_CHK_PER * 10);
    timer_enable(TIMER6);
    timer_event_software_generate(TIMER6, TIMER_EVENT_SRC_UPG);
    gd32_setup_isr(TIMER6_IRQn, timer6_irq_handler, 0);
    gd32_setup_irq(TIMER6_IRQn, 15);
    timer_flag_clear(TIMER6, TIMER_FLAG_UP);
    timer_interrupt_enable(TIMER6, TIMER_INT_UP);

    return 0;
}

/*
 * 功能描述：此函数获取MAC地址
 * 入参说明：无
 * 返 回 值：指向MAC地址数组
 */
uint8_t *gd32_eth_get_mac(void) {
    return mac_addr;
}

/*
 * 功能描述：此函数从PHY获取接口模式（自动协商），然后设置给MAC
 * 入参说明：无
 * 返 回 值：0 --- 成功，-1 --- 失败
 */
int gd32_eth_setup_mode(void) {
    enet_mediamode_enum mode;
    uint16_t            reg;
    uint32_t            count;
    int                 ret;

    if (spd_mode != ENET_AUTO_NEGOTIATION) {
        return 0;
    }

    /* 广告能力 */
    reg = PHY_ANAR_ALL | PHY_ANAR_8023U;
    ret = enet_phy_write_read(ENET_PHY_WRITE, PHY_ADDRESS, PHY_ANAR, &reg);
    if (ret) {
        return -1;
    }

    /* 使能并且重启自动协商 */
    reg = PHY_BMCR_ANEN | PHY_BMCR_ANRESTART;
    ret = enet_phy_write_read(ENET_PHY_WRITE, PHY_ADDRESS, PHY_BMCR, &reg);
    if (ret) {
        return -1;
    }

    for (count = PHY_AN_TIMEO / 10; count != 0; count--) {
        ret = enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_BMSR, &reg);
        if ((ret == 0) && (reg & PHY_BMSR_AN_OVER)) {
            break;
        }

        gd32_delay_ms(10);
    }

    if (count == 0) /* 自动协商超时 */
    {
        mode = ENET_10M_FULLDUPLEX;
        dbg_printf(DBG_LEVEL_INFO, "NET : PHY Auto-Negotiation timeout, mode %d used.\r\n", (int)mode);
    } else {
        ret = enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_STSR, &reg);
        if (ret) {
            mode = ENET_10M_FULLDUPLEX;
        } else {
            if (reg & PHY_STSR_FULL_DPLX) {
                if (reg & PHY_STSR_SPD_10) {
                    mode = ENET_10M_FULLDUPLEX;
                } else {
                    mode = ENET_100M_FULLDUPLEX;
                }
            } else {
                if (reg & PHY_STSR_SPD_10) {
                    mode = ENET_10M_HALFDUPLEX;
                } else {
                    mode = ENET_100M_HALFDUPLEX;
                }
            }
        }

        dbg_printf(DBG_LEVEL_INFO, "NET: PHY Auto-Negotiation success, mode=%d\r\n", mode);
    }

    /* 设置模式到MAC */
    count = ENET_MAC_CFG;
    count &= (~(ENET_MAC_CFG_SPD | ENET_MAC_CFG_DPM | ENET_MAC_CFG_LBM));
    count |= mode;
    ENET_MAC_CFG = count;

    return 0;
}

/*
 * 功能描述：此函数从PHY获取连接状态
 * 入参说明：无
 * 返 回 值：1 --- 连接，0 --- 未连接
 */
int gd32_eth_link_up(void) {
    uint16_t reg;

    enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_STSR, &reg);

    return (reg & PHY_STSR_LINK_VALID) ? 1 : 0;
}

/*
 * 功能描述：此函数是ENET中断
 * 入参说明：无
 * 返 回 值：无
 */
static void enet_irq_handler(void) {
    portBASE_TYPE xTaskWoken = pdFALSE;

    /* 接收到帧 */
    if (enet_interrupt_flag_get(ENET_DMA_INT_FLAG_RS)) {
        rx_callback((void *)&xTaskWoken);
    }

    /* 清除以太网DMA接收中断挂起位 */
    enet_interrupt_flag_clear(ENET_DMA_INT_FLAG_RS_CLR);
    enet_interrupt_flag_clear(ENET_DMA_INT_FLAG_NI_CLR);

    /* 如有必要，切换任务 */
    if (xTaskWoken != pdFALSE) {
        portEND_SWITCHING_ISR(xTaskWoken);
    }
}

/*
 * 功能描述：此函数是定时器6中断
 * 入参说明：无
 * 返 回 值：无
 */
static void timer6_irq_handler(void) {
    portBASE_TYPE xTaskWoken = pdFALSE;
    uint16_t      reg;

    if (timer_flag_get(TIMER6, TIMER_FLAG_UP) == SET) {
        timer_flag_clear(TIMER6, TIMER_FLAG_UP);

        /* 读PHY_MISR */
        enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_MISR, &reg);
        if (reg & PHY_MISR_LINK_INT) {
            link_callback((void *)&xTaskWoken);
        }
    }

    /* 如有必要，切换任务 */
    if (xTaskWoken != pdFALSE) {
        portEND_SWITCHING_ISR(xTaskWoken);
    }
}
