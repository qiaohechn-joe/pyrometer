/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：lwip.c
 * 文件描述：LWIP模块顶层代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#include "platform.h"
#include "gd32_eth.h"
#include "agent.h"
#include "arch/ethernetif.h"
#include "arch/sys_arch.h"
#include "para.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip.h"
#include "system.h"

/* 以太网使用的消息 */
#define ETH_MSG_RX   1 /* 接收到帧 */
#define ETH_MSG_LINK 2 /* 链路状态改变 */

/* 以太网线程：处理IRQ事件 */
static void eth_thread(void *arg);
/* DHCP */
#if LWIP_DHCP
static void dhcp_task(void *arg);
#endif
/* IRQ回调函数 */
static void rx_irq_callback(void *arg);
static void link_irq_callback(void *arg);
/* 链路状态回调函数 */
static void link_callback(struct netif *netif);
/* 网络接口结构 */
static struct netif netif0;
/* 以太网接收 & 链路的邮箱 */
static sys_mbox_t eth_mb;

/*
 * 功能描述：该函数初始化LWIP模块
 * 入参说明：无
 * 返 回 值：0 --- 成功， 其它 --- 失败
 * 备    注：在服务器、客户端init之后初始化
 */
int lwip_stack_init(void) {
    ip_addr_t ip;   /* ip地址 */
    ip_addr_t mask; /* 子网掩码 */
    ip_addr_t gw;   /* 默认网关 */
    uint8_t  *addr;
    err_t     err;

    /* 以太网 */
    if (gd32_eth_init(ETH_SPEED_MODE, rx_irq_callback, link_irq_callback)) {
        dbg_net("NET: %s\r\n", "Ethernet initialize failed!");
        return -1;
    }

    /* 创建tcp_ip堆栈线程 */
    tcpip_init(NULL, NULL);

    /* IP地址设置 */
#if LWIP_DHCP
    ip.addr   = 0;
    mask.addr = 0;
    gw.addr   = 0;
#else
    addr = sys_state.ip_addr;
    IP4_ADDR(&ip, addr[0], addr[1], addr[2], addr[3]);
    addr = sys_state.net_mask;
    IP4_ADDR(&mask, addr[0], addr[1], addr[2], addr[3]);
    addr = sys_state.gateway;
    IP4_ADDR(&gw, addr[0], addr[1], addr[2], addr[3]);
#endif

    /* 注册默认网络接口 */
    addr = gd32_eth_get_mac();
    snprintf(sys_state.macAddr, sizeof(sys_state.macAddr), "%02x%02x%02x%02x%02x%02x", 
        addr[0], addr[1], addr[0], addr[3], addr[0], addr[5]);
    netif_add(&netif0, &ip, &mask, &gw, (void *)addr, &ethernetif_init, &tcpip_input);
    netif_set_default(&netif0);
    netif_set_up(&netif0);

    /* 设置链路回调函数，当链路状态发生变化时调用此函数 */
    netif_set_link_callback(&netif0, link_callback);

    /* 创建邮箱 */
    err = sys_mbox_new(&eth_mb, 8); /* 可以存放4条消息 */
    if (err != ERR_OK) {
        dbg_net("NET: %s\r\n", "eth_mb create failed!");
    }

    /* 创建以太网线程 */
    sys_thread_new(ETH_THREAD_NAME, eth_thread, (void *)&netif0, ETH_THREAD_STACKSIZE, ETH_THREAD_PRIO);

#if LWIP_DHCP
    agent_post(0, AGENT_PRIO_HI, dhcp_task, (void *)0);
//  agent_post(0, AGENT_PRIO_HI, dhcp_task, (void *)&netif0);
#endif
    return 0;
}

/*
 * 功能描述：该函数是以太网线程入口
 * 入参说明：arg --- 入参
 * 返 回 值：无
 */
static void eth_thread(void *arg) {
    struct netif *netif;
    void         *temp;
    int           msg;

    netif = (struct netif *)arg;

    for (;;) {
        /* 等待消息 */
        sys_arch_mbox_fetch(&eth_mb, &temp, 0);
        msg = (int)temp;

        /* 处理消息 */
        switch (msg) {
            case ETH_MSG_RX: /* 接收帧 */
            {
                ethernetif_input(netif);
            } break;

            case ETH_MSG_LINK: /* 连接状态 */
            {
                if (gd32_eth_link_up()) {
                    netif_set_link_up(netif);
                } else {
                    netif_set_link_down(netif);
                }
            } break;

            default: break;
        }
    }
}

/*
 * 功能描述：这个函数是DHCP任务，以SCHED_PRIO_VH优先级调用
 * 入参说明：arg --- 入参
 * 返 回 值：无
 */
#if LWIP_DHCP
static void dhcp_task(void *arg) {
    struct netif *netif;
    unsigned int  dhcp_addr;
    ip_addr_t     ip;
    ip_addr_t     mask;
    ip_addr_t     gw;
    uint8_t      *addr;
    int           retry;
    int           count;

    netif = (struct netif *)arg;

    ip_addr_set_zero_ip4(&netif->ip_addr);
    ip_addr_set_zero_ip4(&netif->netmask);
    ip_addr_set_zero_ip4(&netif->gw);

    dbg_net("\r\n");
    for (retry = LWIP_MAX_DHCP_TRIES; retry != 0; retry--) {
        dhcp_addr = 0;
        dhcp_start(netif);
        dbg_net("NET: %s\r\n", "DHCP start...");

        for (count = DHCP_TIMEOUT * 5; count != 0; count--) {
            dhcp_addr = netif->ip_addr.addr;
            if (dhcp_addr != 0) {
                goto success;
            }

            vTaskDelay(configTICK_RATE_HZ / 5);
        }

        dhcp_stop(netif);
    }

    dbg_net("NET: %s\r\n", "DHCP timeout, use static IP!");

    ENTER_CRITICAL();
    addr = sys_state.net.ip_addr;
    IP4_ADDR(&ip, addr[0], addr[1], addr[2], addr[3]);
    addr = sys_state.net.net_mask;
    IP4_ADDR(&mask, addr[0], addr[1], addr[2], addr[3]);
    addr = sys_state.net.gateway;
    IP4_ADDR(&gw, addr[0], addr[1], addr[2], addr[3]);
    EXIT_CRITICAL();
    netif_set_addr(netif, &ip, &mask, &gw);

    return;

success:
    dhcp_stop(netif);
    dbg_net("NET: DHCP success, ip_addr = %d.%d.%d.%d\r\n", (dhcp_addr >> 24) & 0x0ff, (dhcp_addr >> 16) & 0x0ff, (dhcp_addr >> 8) & 0x0ff,
            dhcp_addr & 0x0ff);
}
#endif /* LWIP_DHCP */

/*
 * 功能描述：这个函数是在以太网接收中断请求时调用的回调函数
 * 入参说明：无
 * 返 回 值：无
 */
static void rx_irq_callback(void *arg) {
    portBASE_TYPE *sched = (portBASE_TYPE *)arg;
    err_t          err;

    err    = sys_mbox_trypost_fromisr(&eth_mb, (void *)ETH_MSG_RX);
    *sched = (err == ERR_NEED_SCHED) ? pdTRUE : pdFALSE;
}

/*
 * 功能描述：这个函数是在以太网链路状态中断请求时调用的回调函数
 * 入参说明：无
 * 返 回 值：无
 */
static void link_irq_callback(void *arg) {
    portBASE_TYPE *sched = (portBASE_TYPE *)arg;
    err_t          err;

    err    = sys_mbox_trypost_fromisr(&eth_mb, (void *)ETH_MSG_LINK);
    *sched = (err == ERR_NEED_SCHED) ? pdTRUE : pdFALSE;
}

/*
 * 功能描述：这个函数是当网络接口(link-status)链路状态发生变化时调用的回调函数
 * 入参说明：无
 * 返 回 值：无
 */
static void link_callback(struct netif *netif) {
    DEFINE_CPU_SR;
    ip_addr_t ip;
    ip_addr_t mask;
    ip_addr_t gw;
    uint8_t  *addr;

    if (netif_is_link_up(netif)) {
        /* 配置接口模式 */
        if (gd32_eth_setup_mode()) {
//            printf("NET: Ethernet setup_mode failed!\r\n");
        }

        /* 启动MAC */
        enet_enable();

        /* 设置IP地址 */
#ifdef USE_DHCP
        ip.addr   = 0;
        mask.addr = 0;
        gw.addr   = 0;
        agent_post(0, AGENT_PRIO_HI, dhcp_task, (void *)netif);
#else
        ENTER_ISR();
        addr = sys_state.ip_addr;
        IP4_ADDR(&ip, addr[0], addr[1], addr[2], addr[3]);
        addr = sys_state.net_mask;
        IP4_ADDR(&mask, addr[0], addr[1], addr[2], addr[3]);
        addr = sys_state.gateway;
        IP4_ADDR(&gw, addr[0], addr[1], addr[2], addr[3]);
        EXIT_ISR();
#endif /* USE_DHCP */

        netif_set_addr(netif, &ip, &mask, &gw);
        netif_set_up(netif);
        dbg_net("NET: %s\r\n", "Ethernet Link UP!");
    } else {
        enet_disable();
        netif_set_down(netif);
        dbg_net("NET: %s\r\n", "Ethernet Link DOWN!");
    }
}
