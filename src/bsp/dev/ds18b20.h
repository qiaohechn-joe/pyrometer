/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ds18b20.h
 * 文件描述：DS18B20温度传感器驱动
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#ifndef _DS18B20_H_
#define _DS18B20_H_

#include "gd32_ll.h"

#define DS18B20_ROM_SIZE       8 /* ROM代码字节数 */
#define DS18B20_RAM_SIZE       9 /* RAM字节数 */

/* 传感器家族代码 */
#define DS18B20_FAMILY_CODE    0x28 /* DS18B20的家族代码 */

/* 失败值 */
#define DS18B20_FAIL_VALUE     850 /* 85度的失败值 */

/* 命令代码 */
#define DS18B20_CMD_SEARCH_ROM 0xF0 /* 搜索ROM（在总线上搜索所有设备） */
#define DS18B20_CMD_READ_ROM   0x33 /* 读取ROM（读取总线上设备的ID） */
#define DS18B20_CMD_MATCH_ROM  0x55 /* 匹配ROM（在总线上定址设备） */
#define DS18B20_CMD_SKIP_ROM   0xCC /* 跳过ROM（向总线上所有设备广播） */
#define DS18B20_CMD_ALM_SEARCH 0xEC /* 报警搜索（搜索总线上所有报警设备） */
#define DS18B20_CMD_CONVERT    0x44 /* 启动转换过程 */
#define DS18B20_CMD_WRITE_RAM  0x4E /* 写入RAM（Scratchpad） */
#define DS18B20_CMD_READ_RAM   0xBE /* 读取RAM（Scratchpad） */
#define DS18B20_CMD_COPY_RAM   0x48 /* 将RAM（Scratchpad）中的可配置寄存器复制到EEPROM */
#define DS18B20_CMD_RECALL_E2  0xB8 /* 将EEPROM中的可配置寄存器加载到RAM（Scratchpad） */
#define DS18B20_CMD_READ_PWR   0xB4 /* 读取设备的电源供应状态（寄生电源或外部电源） */

/* 分辨率 */
#define DS18B20_RES_BIT9       (0x00 << 5) /* 9位 */
#define DS18B20_RES_BIT10      (0x01 << 5) /* 10位 */
#define DS18B20_RES_BIT11      (0x02 << 5) /* 11位 */
#define DS18B20_RES_BIT12      (0x03 << 5) /* 12位 */

/* 时序参数：单位为微秒 */
#define DS18B20_RST_WIDTH      500 /* 复位脉冲宽度 */
#define DS18B20_RST_WAIT       50  /* 复位等待时间 */
#define DS18B20_SLOT_START     5   /* 开始时隙时间 */
#define DS18B20_SLOT_WIDTH     100 /* 时隙宽度 */
#define DS18B20_READ_WAIT      5   /* 读取等待时间 */
#define DS18B20_RECOVERY       10  /* 读/写恢复时间 */

#define DS18B20_CNT_PER_WIRE   1      /* 每条线上的传感器数量 */
#define DS18B20_RESULT_DP      1      /* 结果的小数点后位数 */
#define DS18B20_RES_BIT        12     /* 分辨率位数 */
#define DS18B20_ALM_HI         127    /* 高温报警阈值 */
#define DS18B20_ALM_LO         (-127) /* 低温报警阈值 */

/*
    单总线引脚：PA6
*/

#define DS18B20_WIRE_CNT       1
#define DS18B20_WIRE1_PORT     GPIOA
#define DS18B20_WIRE1_PIN      GPIO_PIN_6

#define ds18b20_pin_init()                                                                                                                           \
    {                                                                                                                                                \
        rcu_periph_clock_enable(RCU_GPIOA);                                                                                                          \
        gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_6);                                                                          \
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);                                                                \
    }

/* 延时 */
#define wire_delay_us(n) gd32_delay_us(n)

int  ds18b20_init(void);                   /* 初始化 */
void ds18b20_reset(void);                  /* 复位ds18b20 */
void ds18b20_select(int num);              /* 选择总线 */
int  ds18b20_search(uint8_t (*roms)[8]);   /* 搜索ROM */
int  ds18b20_config(uint8_t *rom);         /* 配置参数 */
int  ds18b20_start(uint8_t *rom);          /* 开始转换 */
int  ds18b20_read(uint8_t *rom, int *val); /* 读取结果 */

#endif /* _DS18B20_H_ */
