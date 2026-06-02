/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：arch_cfg.h
 * 文件描述：应用架构层配置文件
 * 作    者：和其光电嵌软团队
 * 备    注：GD32F427最大7扇区512KB
 * 更新记录：
 *           V1.0 2024/01/26 陈  军  初始版本
 *           V1.1 2024/12/11 李兆越  串口配置优化
 *           V1.2 2025/02/13 李兆越  优化EEPROM日志存储分区
 */

#ifndef _ARCH_CFG_H_
#define _ARCH_CFG_H_

#include "FreeRTOS.h"
#include "semphr.h"

/***********************************************************************************************************************
 *   串口配置项
 **********************************************************************************************************************/
#define COM_DMA_XFER_SIZE 256 /* DMA传输大小 */

#define COM2              0 /* USART2 */
#define COM_CNT           1

/* COM2(UART2 RS422): TXD = PD8  RXD = PD9  TXE=PB14  nRXE=PB15 */
#define COM2_RX_TIMER     TIMER4
#define COM2_TX_BUFF_SIZE 768
#define COM2_RX_BUFF_SIZE 1056

#define COM2_RE_PORT      GPIOB
#define COM2_RE_CLK       RCU_GPIOB
#define COM2_DE_PORT      GPIOB
#define COM2_DE_CLK       RCU_GPIOB
#define COM2_RE_PIN       GPIO_PIN_15
#define COM2_DE_PIN       GPIO_PIN_14

#define COM2_IO_PORT      GPIOD
#define COM2_IO_CLK       RCU_GPIOD
#define COM2_IO_AF        GPIO_AF_7
#define COM2_IO_TX_PIN    GPIO_PIN_8
#define COM2_IO_RX_PIN    GPIO_PIN_9

#define com2_pin_init()                                                                                                                              \
    {                                                                                                                                                \
        rcu_periph_clock_enable(COM2_IO_CLK);                                                                                                        \
        rcu_periph_clock_enable(RCU_GPIOC);                                                                                                          \
        gpio_af_set(GPIOD, COM2_IO_AF, COM2_IO_TX_PIN | COM2_IO_RX_PIN);                                                                             \
        gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, COM2_IO_TX_PIN | COM2_IO_RX_PIN);                                                       \
        gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, COM2_IO_TX_PIN | COM2_IO_RX_PIN);                                           \
        gpio_mode_set(COM2_RE_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, COM2_RE_PIN);                                                                  \
        gpio_output_options_set(COM2_RE_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, COM2_RE_PIN);                                                        \
        gpio_mode_set(COM2_DE_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, COM2_DE_PIN);                                                                  \
        gpio_output_options_set(COM2_DE_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, COM2_DE_PIN);                                                        \
        gpio_bit_reset(COM2_RE_PORT, COM2_RE_PIN);                                                                                                   \
        gpio_bit_set(COM2_DE_PORT, COM2_DE_PIN);                                                                                                     \
    }

/***********************************************************************************************************************
 *   任务相关配置项
 **********************************************************************************************************************/
#if 1
#define START_TASK_PRIO      22   /* 实时系统起始任务优先级 */
#define START_TASK_STKSIZE   1024 /* 实时系统起始任务堆栈大小 */
#define SAMPLE_TASK_PRIO     23   /* 实时系统采样任务优先级 */
#define SAMPLE_TASK_STKSIZE  1024 /* 实时系统采样任务堆栈大小 */
#define OSTIMER_TASK_PRIO    22   /* 软件定时器任务优先级 */
#define OSTIMER_TASK_STKSIZE 2048 /* 软件定时器任务堆栈大小 */

#define LWIP_TASK_START_PRIO 31 /* LWIP协议栈任务优先级  32*/
#define LWIP_TASK_END_PRIO   28 /* LWIP协议栈任务堆栈大小 30 */
#else
#define START_TASK_PRIO      23   /* 实时系统起始任务优先级 */
#define START_TASK_STKSIZE   1024 /* 实时系统起始任务堆栈大小 */
#define SAMPLE_TASK_PRIO     22   /* 实时系统采样任务优先级 */
#define SAMPLE_TASK_STKSIZE  1024 /* 实时系统采样任务堆栈大小 */
#define OSTIMER_TASK_PRIO    22   /* 软件定时器任务优先级 */
#define OSTIMER_TASK_STKSIZE 2048 /* 软件定时器任务堆栈大小 */

#define LWIP_TASK_START_PRIO 21 /* LWIP协议栈任务优先级 */
#define LWIP_TASK_END_PRIO   18 /* LWIP协议栈任务堆栈大小 */
#endif
/* 代理任务系统优先级堆栈配置 */
#define AGENT_HI_TASK_PRIO     (START_TASK_PRIO - 1) /* 高代理任务优先级 */
#define AGENT_HI_TASK_STKSIZE  1024                  /* 高代理任务堆栈大小 */

#define AGENT_MD_TASK_PRIO     (START_TASK_PRIO - 2) /* 中代理任务优先级 */
#define AGENT_MD_TASK_STKSIZE  512                   /* 中代理任务堆栈大小 */

#define AGENT_LO_TASK_PRIO     (START_TASK_PRIO - 3) /* 低代理任务优先级 */
#define AGENT_LO_TASK_STKSIZE  512                   /* 低代理任务堆栈大小 */

/***********************************************************************************************************************
 *   用户线程配置项
 **********************************************************************************************************************/
#define USER_THREAD_CNT        13                       /* 线程个数 */
#define USER_THREAD_START_PRIO (LWIP_TASK_END_PRIO - 4) /* 线程起始优先级 */

/***********************************************************************************************************************
 *   代理任务配置项
 **********************************************************************************************************************/
/* 代理队列大小 */
#define AGENT_QUEUE_SIZE       32

/* 代理任务优先级 */
#define AGENT_PRIO_HI          0 /* 高 */
#define AGENT_PRIO_MD          1 /* 中 */
#define AGENT_PRIO_LO          2 /* 低 */
#define AGENT_PRIO_CNT         3

/* 代理是否来自中断？ */
#define NOT_FROM_ISR           0
#define FROM_ISR               1

/***********************************************************************************************************************
 *   存储配置项
 **********************************************************************************************************************/
#define MEM_TEST_SIZE          64 /* 存储测试大小 */

#define SUPPORT_GDFLASH        1 /* 支持GS32内部FLASH */
#define SUPPORT_EEPROM         1 /* 支持EEPROM存储 */
#define SUPPORT_SPIFLASH       0 /* 支持SPI-FLASH存储 */

#if SUPPORT_GDFLASH
/********************* GD32内部FLASH分区 *********************/
/* BOOT存储地址（16KB）: 扇区0 */

/* 升级信息存储地址（16KB）: 扇区1 */
#define FW_INFO_ADDR      (uint32_t)(FLASH_BASE + 0x04000) /* 升级固件信息存储地址（4KB） */

/* 参数相关存储地址（16KB）: 扇区2 */
#define USER_PARA_ADDR    (uint32_t)(FLASH_BASE + 0x08000) /* 用户参数存储地址（4KB） */
// #define USER_PARA_ADDR    (uint32_t)(BACKUP_PARA_ADDR + sizeof(sys_para_t)) /* 用户参数存储地址（4KB） */
#define QUICK_SAVE_ADDR   (uint32_t)(FLASH_BASE + 0x08000 + 0x02000) /* 快速保存参数存储地址（4KB） */

/* 扇区3 */
#define FACTORY_PARA_ADDR (uint32_t)(FLASH_BASE + 0x08000 + 0x04000) /* 工厂参数存储地址（4KB） */
#define RSVD_ADDR3        (uint32_t)(FLASH_BASE + 0x0C000)
#define RSVD_SECT3        CTL_SECTOR_NUMBER_3

/* APP存储地址（64K+128K=192KB）: 扇区4-扇区5 */
#define APP_ENTRY_ADDR    (uint32_t)(FLASH_BASE + 0x10000)
#define APP_ENTRY_SECT_1  CTL_SECTOR_NUMBER_4
#define APP_ENTRY_SECT_2  CTL_SECTOR_NUMBER_5

/* 升级数据存储地址（128K+128K=256KB）: 扇区6-扇区7 */
#define FW_DATA_ADDR      (uint32_t)(FLASH_BASE + 0x40000)
#define FW_DATA_SECT_1    CTL_SECTOR_NUMBER_6
#define FW_DATA_SECT_2    CTL_SECTOR_NUMBER_7

/* 数字签名地址 */
#define SIGNATURE_ADDRESS 0x40023D00
/*************************************************************/
#endif /* SUPPORT_GDFLASH */

#if SUPPORT_EEPROM
/*** EEPROM型号：AT24CM01  ***/
#define EEPROM_I2C_BUS            I2C_BUS_EEPROM      /* I2C总线0 */
#define EEPROM_I2C_ADDR           I2C_ADDR_EEPROM     /* I2C地址：7-bit */
#define EEPROM_MEM_SIZE           ((uint32_t)0x20000) /* 容量：1-Mbit(131072字节) */
#define EEPROM_MUTEX_TIMEOUT      (configTICK_RATE_HZ / 2)
/* EEPROM存储分区 */
#define BACKUP_PARA_ADDR          0    /* 4096 字节 */
#define EEPROM_ADDR_REC_INDEX     4096 /* 256 字节 */
#define EEPROM_ADDR_FREE          4352
#define EEPROM_ADDR_REC_BBOX_ON   4352
#define EEPROM_ADDR_REC_BBOX_OFF  63360
#define EEPROM_ADDR_REC_STATE_ON  63360
#define EEPROM_ADDR_REC_STATE_OFF 131008
#define EEPROM_TEST_ADDR          131008 /* EEPROM存储测试地址,64字节 */
#define EEPROM_ADDR_END           131072
#endif /* SUPPORT_EEPROM */

#if SUPPORT_SPIFLASH
/*** SPI FLASH ***/
#define SF_MUTEX_TIMEOUT        (configTICK_RATE_HZ / 2)
/* sect：扇区号（包括所有芯片），off：扇区内偏移量 */
#define SF_MAKE_ADDR(sect, off) (((sect) << W25_SECT_SHIFT) + (off))
/* SPIFLASH分区（依据需求进行分区） */
#define SF_ADDR_RSVD1           SF_MAKE_ADDR(0, 0)   /* 分区1：扇区0（4KB） */
#define SF_ADDR_RSVD2           SF_MAKE_ADDR(1, 0)   /* 分区2：扇区1~255（1MB） */
#define SF_ADDR_RSVD3           SF_MAKE_ADDR(256, 0) /* 分区3：扇区256-2047（7MB） */
#define SF_ADDR_RSVD4           SF_MAKE_ADDR(2048, 0)
#define SF_ADDR_END             SF_MAKE_ADDR(8192, 0)
#define SF_TEST_ADDR            SF_ADDR_RSVD1 /* SPIFLASH测试地址 */
#endif                                        /* SUPPORT_SPIFLASH */

/***********************************************************************************************************************
 *   邮箱配置项
 **********************************************************************************************************************/
#define MBOX_MAX_COUNT 8 /* 邮箱个数 */
#define MBOX_CAPACITY  8 /* 每个邮箱的容量 */

/***********************************************************************************************************************
 *   消息队列配置项
 **********************************************************************************************************************/
#define MSG_QUEUE_SIZE 50 /* 消息队列大小 */

/* 支持一字节 */
#define MSG_NONE       0      /* 无消息 */
#define MSG_KEY_PRESS  0x0001 /* 按键按下 */
#define MSG_KEY_LPRESS 0x0002 /* 按键长按 */

/***********************************************************************************************************************
 *   操作系统定时器配置项
 **********************************************************************************************************************/
/* 定时器类型 */
typedef enum {
    OSTIMER_SYS_SERVICE,
    OSTIMER_CAMERA_TASK,
    OSTIMER_UI_TASK,
    OSTIMER_BURST_TASK,
    OSTIMER_KEY_SCAN_TASK,
    OSTIMER_RECORD_TASK,
    OSTIMER_ALARM_TASK,
    OSTIMER_USB_UVC_TASK,
    OSTIMER_DATA_SEND_TASK,
    OSTIMER_UVC_CTL_TASK,
    OSTIMER_HEATER_TASK,
    OSTIMER_COUNT,
} os_timer_t;

#define OSTIMER_EXECUT_FREQ_MS 200 /* 5ms */

#endif /* _ARCH_CFG_H_ */
