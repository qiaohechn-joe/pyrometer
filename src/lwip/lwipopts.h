/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：lwipopts.h
 * 文件描述：LwIP选项配置文件（基于GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2025/03/12 陈军  新增TCP连接保持选项
 */

#ifndef _LWIPOPTS_H_
#define _LWIPOPTS_H_

#include "arch_cfg.h"

/********************************************************************************************************
 *   系统与兼容性配置
 ********************************************************************************************************/
/* 系统配置 */
#define SYS_LIGHTWEIGHT_PROT                      1 /* 为1时使用实时操作系统的轻量级保护，保护关键代码不被中断打断 */
#define NO_SYS                                    0 /* 定义为0时，使用FreeRTOS系统 */

/* 兼容互斥锁选项 */
#define LWIP_COMPAT_MUTEX                         1 /* 启用与操作系统兼容的互斥锁 */
#define LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX 1 /* 在FreeRTOS下，使用互斥锁来保护系统架构 */

/********************************************************************************************************
 *   线程与任务配置
 ********************************************************************************************************/
/* 以太网线程配置 */
#define ETH_THREAD_NAME                           "ETH"                /* 以太网线程名称 */
#define ETH_THREAD_STACKSIZE                      512                  /* 以太网线程堆栈大小 */
#define ETH_THREAD_PRIO                           LWIP_TASK_START_PRIO /* 以太网线程优先级 */
#define ETH_TX_GUARD_TIMEO                        3000                 /* Tx互斥锁保护超时时间，单位为毫秒 */

/* TCP/IP线程配置 */
#define TCPIP_THREAD_NAME                         "TCP/IP"                   /* TCP/IP线程名称 */
#define TCPIP_THREAD_STACKSIZE                    1024                       /* TCP/IP线程堆栈大小 */
#define TCPIP_THREAD_PRIO                         (LWIP_TASK_START_PRIO - 1) /* TCP/IP线程优先级 */
#define TCPIP_MBOX_SIZE                           8                          /* TCP/IP消息队列大小 */

/* 默认线程配置 */
#define DEFAULT_THREAD_STACKSIZE                  512 /* 默认线程堆栈大小 */
#define DEFAULT_TCP_RECVMBOX_SIZE                 8   /* 默认TCP接收消息队列大小 */
#define DEFAULT_UDP_RECVMBOX_SIZE                 0   /* 默认UDP接收消息队列大小 */
#define DEFAULT_ACCEPTMBOX_SIZE                   8   /* 默认接受连接消息队列大小 */

/********************************************************************************************************
 *   网络协议配置
 ********************************************************************************************************/
/* TCP/IP配置 */
#define ETHARP_TRUST_IP_MAC                       0 /* 定义为0时，不自动信任从IP到MAC的映射，增强安全性 */
#define IP_REASSEMBLY                             0 /* 定义为0时，禁用IP重组，适用于内存资源受限的系统 */
#define IP_FRAG                                   0 /* 定义为0时，禁用IP分片，要求发送的IP包必须符合MTU限制 */
#define ARP_QUEUEING                              0 /* 定义为0时，禁用ARP队列，当ARP表中查不到时，相关的IP包将被丢弃 */
#define TCP_LISTEN_BACKLOG                        1 /* 定义为1时，启用TCP监听回溯队列，允许在接受连接前缓存连接请求 */

/* TCP选项 */
#define LWIP_TCP                                  1                             /* 定义为1时，启用TCP协议支持 */
#define TCP_TTL                                   255                           /* TCP数据包的生存时间，最大值255 */
#define TCP_QUEUE_OOSEQ                           0                             /* 定义为0时，禁用对超序列TCP段的队列处理，适用于内存受限环境 */
#define TCP_MSS                                   (1500 - 40)                   /* 定义TCP最大分段大小，计算方法为：MTU - IP报头大小 - TCP报头大小 */
#define TCP_SND_BUF                               (32 * TCP_MSS)                 /* TCP发送缓冲区大小，单位为字节 */
#define TCP_SND_QUEUELEN                          ((12 * TCP_SND_BUF) / TCP_MSS) /* 根据TCP发送缓冲区大小计算的发送队列长度 */
#define TCP_WND                                   (32 * TCP_MSS)                 /* TCP窗口大小，影响数据传输效率和速度 */
#define LWIP_DISABLE_TCP_SANITY_CHECKS             1                            /* 禁用TCP健壮性检查，提升性能 */

#define LWIP_WND_SCALE                             1 
#define TCP_RCV_SCALE                              4  // 或者其他值，取决于你的需求

/* TCP连接保持选项 */
#define LWIP_TCP_KEEPALIVE                        1    /* TCP连接保持选项 */
#define TCP_KEEPIDLE_DEFAULT                      3000 /* 3秒内双方无数据则发起保活探测 */
#define TCP_KEEPINTVL_DEFAULT                     1000 /* 1秒发送一次保活探测 */
#define TCP_KEEPCNT_DEFAULT                       3    /* 3次探测无响应则断开 */

/* ICMP选项 */
#define LWIP_ICMP                                 1 /* 定义为1时，启用ICMP协议支持，允许发送和接收ping请求等 */

/* DHCP选项 */
#define LWIP_DHCP                                 0 /* 定义为0时，不使用DHCP动态主机配置协议 */
#define DHCP_TIMEOUT                              5 /* DHCP请求超时时间，单位为秒 */

/* UDP选项 */
#define LWIP_UDP                                  0   /* 定义为1时，启用UDP协议支持 */
#define UDP_TTL                                   255 /* UDP数据包的生存时间，最大值255 */

/********************************************************************************************************
 *   内存管理
 ********************************************************************************************************/
/* 内存选项 */
#define MEM_ALIGNMENT                             4           /* 使用4字节对齐模式，提高存取效率 */
#define MEM_SIZE                                  (1024 * 80) /* 内存堆heap大小，设置为20KB */

/* MEMP：内存池配置 */
#define MEMP_NUM_PBUF                             120 /* memp结构的pbuf数量，适用于从ROM或静态存储区发送大量数据 */
#define MEMP_NUM_UDP_PCB                          0   /* UDP协议控制块(PCB)数量，每个活动的UDP"连接"需要一个PCB */
#define MEMP_NUM_TCP_PCB                          20  /* 同时建立激活的TCP数量 */
#define MEMP_NUM_TCP_PCB_LISTEN                   20   /* 能够监听的TCP连接数量 */
#define MEMP_NUM_TCP_SEG                          60  /* 最多同时在队列中的TCP段数量 */
#define MEMP_NUM_SYS_TIMEOUT                      10  /* 能够同时激活的timeout个数 */
#define MEMP_NUM_NETBUF                           16   /* 网络缓冲区的数量 */

/* PBUF：协议缓冲区配置 */
#define PBUF_POOL_SIZE                            80   /* pbuf内存池个数，定义了系统中可用的pbuf数量 */
#define PBUF_POOL_BUFSIZE                         1526 /* 每个pbuf内存池大小，通常设置为最大以太网帧大小 1514 */

/* IP重组配置 */
#define IP_REASS_MAX_PBUFS                        20 /* IP重组过程中，最大的pbuf数量 */

/********************************************************************************************************
 *   高级选项与调试
 ********************************************************************************************************/
/* 统计选项 */
#define LWIP_STATS                                1 /* 定义为0时，禁用LwIP统计功能，可以节省内存 */
#define LWIP_STATS_DISPLAY                        1 /* 定义为0时，禁用统计信息的显示功能，减少代码体积 */
#define LWIP_PROVIDE_ERRNO                        1 /* 定义为1时，LwIP提供错误号定义，用于兼容POSIX错误处理 */
/* 链接回调选项 */
#define LWIP_NETIF_LINK_CALLBACK                  1 /* 定义为1时，支持在接口链接状态改变时（例如，链接断开）回调函数 */

/* 顺序层选项 */
#define LWIP_NETCONN                              1 /* 定义为1时，启用NETCONN函数（需要使用api_lib.c） */
#define MEMP_NUM_NETCONN                          20 /* struct netconns的数量 */

/* 套接字选项 */
#define LWIP_SOCKET                               0 /* 定义为0时，禁用Socket API（如果需要使用，要求使用sockets.c） */
#define LWIP_SO_RCVTIMEO                          1 /* 定义为1时，启用套接字/NETCONN的接收超时设置和SO_RCVTIMEO处理 */

/* LwIP调试选项和各种调试模块的开关 */
#define LWIP_DEBUG                                LWIP_DBG_OFF  //LWIP_DBG_OFF /* 关闭LwIP的调试功能 0x80 */
#define ICMP_DEBUG                                LWIP_DBG_OFF /* ICMP模块调试 */
#define NETIF_DEBUG                               LWIP_DBG_OFF //LWIP_DBG_OFF /* 网络接口模块调试 */
#define PBUF_DEBUG                                LWIP_DBG_OFF /* 数据包缓冲区管理模块调试 */
#define ETHARP_DEBUG                              LWIP_DBG_OFF /* ARP协议模块调试 */
#define API_LIB_DEBUG                             LWIP_DBG_OFF /* API库调试 */
#define API_MSG_DEBUG                             LWIP_DBG_OFF /* API消息处理调试 */
#define SOCKETS_DEBUG                             LWIP_DBG_OFF /* 套接字API调试 */
#define IGMP_DEBUG                                LWIP_DBG_OFF /* IGMP协议调试 */
#define INET_DEBUG                                LWIP_DBG_OFF /* INET协议族调试 */
#define IP_DEBUG                                  LWIP_DBG_OFF /* IP协议调试 */
#define IP_REASS_DEBUG                            LWIP_DBG_OFF /* IP重组模块调试 */
#define RAW_DEBUG                                 LWIP_DBG_OFF /* 原始IP协议调试 */
#define MEM_DEBUG                                 LWIP_DBG_OFF /* 内存管理调试 */
#define MEMP_DEBUG                                LWIP_DBG_OFF //LWIP_DBG_OFF /* 内存池管理调试 */
#define SYS_DEBUG                                 LWIP_DBG_OFF /* 系统层调试 */
#define TIMERS_DEBUG                              LWIP_DBG_OFF /* 定时器模块调试 */
#define TCP_DEBUG                                 LWIP_DBG_OFF /* TCP协议调试 */
#define TCP_INPUT_DEBUG                           LWIP_DBG_OFF /* TCP输入处理调试 */
#define TCP_FR_DEBUG                              LWIP_DBG_OFF /* TCP快速重传调试 */
#define TCP_RTO_DEBUG                             LWIP_DBG_OFF /* TCP重传超时调试 */
#define TCP_CWND_DEBUG                            LWIP_DBG_OFF /* TCP拥塞窗口调试 */
#define TCP_WND_DEBUG                             LWIP_DBG_OFF /* TCP窗口管理调试 */
#define TCP_OUTPUT_DEBUG                          LWIP_DBG_OFF /* TCP输出处理调试 */
#define TCP_RST_DEBUG                             LWIP_DBG_OFF /* TCP重置处理调试 */
#define TCP_QLEN_DEBUG                            LWIP_DBG_OFF /* TCP队列长度调试 */
#define UDP_DEBUG                                 LWIP_DBG_OFF /* UDP协议调试 */
#define TCPIP_DEBUG                               LWIP_DBG_OFF /* TCP/IP核心管理调试 */
#define DHCP_DEBUG                                LWIP_DBG_OFF /* DHCP协议调试 */
#define AUTOIP_DEBUG                              LWIP_DBG_OFF /* AUTOIP协议调试 */
#define DNS_DEBUG                                 LWIP_DBG_OFF /* DNS协议调试 */
#define SLIP_DEBUG                                LWIP_DBG_OFF /* SLIP协议调试 */

/********************************************************************************************************
 *   性能与安全
 ********************************************************************************************************/
/* 校验和选项 */
#define CHECKSUM_BY_HARDWARE                      /* 通过硬件计算和验证IP、UDP、TCP和ICMP的校验和 */

/* 帧校验和选项，GD32F427允许通过硬件识别和计算IP,UDP和ICMP的帧校验和 */
#ifdef CHECKSUM_BY_HARDWARE  /* 使用硬件帧校验 */
#define CHECKSUM_GEN_IP    0 /* IP数据包的帧校验和 */
#define CHECKSUM_GEN_UDP   0 /* UDP数据包的帧校验和 */
#define CHECKSUM_GEN_TCP   0 /* TCP数据包的帧校验和 */
#define CHECKSUM_CHECK_IP  0 /* 输入的IP数据包的帧校验和 */
#define CHECKSUM_CHECK_UDP 0 /* 输入的UDP数据包的帧校验和 */
#define CHECKSUM_CHECK_TCP 0 /* 输入的TCP数据包的帧校验和 */
#define CHECKSUM_GEN_ICMP  0 /* 输入的ICMP数据包的帧校验和 */
#else                        /* 使用软件帧校验 */
#define CHECKSUM_GEN_IP    1
#define CHECKSUM_GEN_UDP   1
#define CHECKSUM_GEN_TCP   1
#define CHECKSUM_CHECK_IP  1
#define CHECKSUM_CHECK_UDP 1
#define CHECKSUM_CHECK_TCP 1
#define CHECKSUM_GEN_ICMP  1
#endif

#endif /* _LWIPOPTS_H_ */
