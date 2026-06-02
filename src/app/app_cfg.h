/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：app_cfg.h
 * 文件描述：应用配置文件
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#ifndef _APP_CFG_H_
#define _APP_CFG_H_

/********************************************************************************************************
 *   产品基本信息
 ********************************************************************************************************/
#define COMPANY_LOGO       "Company: Optsensor"
#define PRODUCT_NAME       "Product: LED102-v3"
#define SPECIAL_NAME       "LED102-v3"

#define BOOT_FW_DESC       "Bootloader:HQ-BOOT"
#define BOOT_FW_VER        "V1.0.4"

#define BOOT_VER_MAJOR     1 /* BootLoader主版本号 */
#define BOOT_VER_MINOR     0 /* BootLoader子版本号 */
#define BOOT_VER_SERIAL    4 /* BootLoader修正版本号 */

#define HW_VER_MAJOR       1 /* 硬件主版本号 */
#define HW_VER_MINOR       0 /* 硬件子版本号 */
#define HW_VER_SERIAL      1 /* 硬件修正版本号 */

#define FW_VER_MAJOR       1  /* 软件主版本号 */
#define FW_VER_MINOR       0  /* 软件子版本号 */
#define FW_VER_SERIAL      50 /* 软件编译版本号 */

/********************************************************************************************************
 *   硬件相关配置
 ********************************************************************************************************/
#define DMA_PRIO_COM2_TX   DMA_PRIORITY_LOW /* DMA优先级 : 0(最低) ~ 3(最高) */
#define DMA_PRIO_COM2_RX   DMA_PRIORITY_ULTRA_HIGH
#define DMA_PRIO_COM3_TX   DMA_PRIORITY_LOW
#define DMA_PRIO_COM3_RX   DMA_PRIORITY_ULTRA_HIGH
#define DMA_PRIO_COM6_TX   DMA_PRIORITY_LOW
#define DMA_PRIO_COM6_RX   DMA_PRIORITY_HIGH

/********************************************************************************************************
 *   系统相关配置
 ********************************************************************************************************/
#define VECTOR_TAB_OFFSET  0x10000
#define FIRMWARE_INFO_ADDR (FLASH_BASE + VECTOR_TAB_OFFSET + 0x400) /* 固件信息在bin文件中存放地址 */

#define SYS_TICKS_PER_SEC  4 /* 250ms */

#define WARM_START_MAGIC   0xA55AB66B /* 热启动密钥 */

#define DEV_ADDR_MAX       100 /* 设备最大支持地址 */
#define DEV_ID_SIZE        10  /* 设备SN码长度 */
#define DEV_INFO_SIZE      128 /* 设备信息长度 */
#define DEV_NAME_SIZE      16  /* 设备名字长度 */

#define BURST_LIST_SIZE    40 /* 突发字符串最大长度 */

#define DEV_MIN_ADDR       0
#define DEV_MAX_ADDR       32

#define TEMP_UPPER         3500 /* 设备测温上限 */
#define TEMP_LOWER         100  /* 设备测温下限 */

#define HOLDING_TIME_LOWER 0.0f   /* 滤波保持持续时间下限 */
#define HOLDING_TIME_UPPER 300.0f /* 滤波保持持续时间上限 */

#define SENS_CH_SPEC_WB    0 /* 宽带光谱通道 */
#define SENS_CH_SPEC_NB    1 /* 窄带光谱通道 */
#define SENS_CH_SPEC_CNT   2 /* 传感器检测光谱通道个数 */

#define SENS_CH_NTC_SPEC   0 /* 监测光电二极管部分温度（NTC）通道 */
#define SENS_CH_NTC_TIA    1 /* 监测跨阻放大器部分温度（NTC）通道 */
#define SENS_CH_NTC_CNT    2 /* 传感器检测NTC通道个数 */

#define SENS_CH_CNT        4 /* 传感器检测通道总个数 */

#define ANA_0_20MA         '0' /* 模拟输出电流范围0~20mA */
#define ANA_4_20MA         '4' /* 模拟输出电流范围4~20mA */
#define ANA_0_10V          '5' /* 模拟输出电压范围0~10V */

#define BURST_MOD_P        'P' /* 轮询 */
#define BURST_MOD_B        'B' /* 突发 */

#define COM_HALF_DUPLEX    '2' /* 半双工通信模式 （2线制）*/
#define COM_FULL_DUPLEX    '4' /* 全双工通信模式 （4线制）*/

#endif /* _APP_CFG_H_ */
