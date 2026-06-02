/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：eeprom.c
 * 文件描述：I2C接口EEPROM驱动
 * 作    者：和其光电嵌软团队
 * 备    注：AT24CM01-SSHD-T,工作电压2.5V-5.5V
 * 更新记录：
 *           V1.0 2024/01/26 陈军    初始版本
 *           V1.1 2024/06/05 陈军    移除EEPROM型号选择到dev_cfg文件
 *           V1.2 2024/06/12 张晓博  增加对AT24C02的支持
 *           V1.3 2025/03/10 陈军    统一为EEPROM，增加写保护（WP）逻辑
 */

#include "hq_generic.h"
#include "eeprom.h"

#define AT24C_CHIP_SIZE (1UL << AT24C_CHIP_SIZE_SHIFT)

/* 内部函数 */
static int write_in_page(int bus, uint32_t dev_addr, uint32_t mem_addr, void *buff, int len);

/*
 * 功能描述：该函数从EEPROM设备读取若干字节
 * 入参说明：bus --- 总线编号，从0开始
 *           dev_addr --- EEPROM设备的I2C从设备地址
 *           mem_addr --- 内存地址
 *           buff --- 存储数据的缓冲区
 *           len --- 数据长度（字节）
 * 返 回 值：成功读取的字节数
 */
int at24c_read(int bus, uint32_t dev_addr, uint32_t mem_addr, void *buff, int len) {
    uint8_t addr[AT24C_ADDR_LEN]; /* 地址缓存 */
    int     ret;

    /* 存储地址 */
#if AT24C_CHIP_TYPE == AT24C64 || AT24C_CHIP_TYPE == AT24C512 || AT24C_CHIP_TYPE == AT24CM01
    addr[0] = byte_1(mem_addr); /* MSB */
    addr[1] = byte_0(mem_addr); /* LSB */
#elif AT24C_CHIP_TYPE == AT24C02
    addr[0] = byte_0(mem_addr); /* LSB */
#elif AT24C_CHIP_TYPE == AT24C1024
    if (mem_addr >= 65536) {
        dev_addr++;
        mem_addr -= 65536;
    }

    addr[0] = byte_1(mem_addr); /* MSB */
    addr[1] = byte_0(mem_addr); /* LSB */
#endif

    /* 传输开始 */
    si2c_start(bus);

    /* 传输从设备地址以进行写操作 */
    if (si2c_select(bus, dev_addr, SI2C_OP_WRITE) != 0) {
        si2c_stop(bus);
        return 0;
    }

    /* 传输存储地址 */
    if (si2c_tx(bus, addr, sizeof(addr)) != sizeof(addr)) {
        si2c_stop(bus);
        return 0;
    }

    /* 传输开始 */
    si2c_start(bus);

    /* 传输从设备地址以进行读操作 */
    if (si2c_select(bus, dev_addr, SI2C_OP_READ) != 0) {
        si2c_stop(bus);
        return 0;
    }

    /* 接收数据 */
    ret = si2c_rx(bus, buff, len);

    /* 传输停止 */
    si2c_stop(bus);

    return ret;
}

/*
 * 功能描述：该函数向EEPROM设备写入若干字节
 * 入参说明：bus --- 总线编号，从0开始
 *           dev_addr --- EEPROM设备的I2C从设备地址
 *           mem_addr --- 内存地址
 *           buff --- 存储数据的缓冲区
 *           len --- 数据长度（字节）
 * 返 回 值：成功写入的字节数
 */
int at24c_write(int bus, uint32_t dev_addr, uint32_t mem_addr, const void *buff, int len) {
    uint8_t *data;
    uint32_t addr;
    int      size;
    int      remain;

    if (EEPROM_USED_WP == 1) eeprom_wp_unlock(); /* 解锁写保护 */

#if AT24C_CHIP_TYPE == AT24C1024
    if (mem_addr >= 65536) {
        dev_addr++;
        mem_addr -= 65536;
    }
#endif

    data = (uint8_t *)buff;
    addr = mem_addr;

    for (remain = len; remain != 0;) {
        size = AT24C_PAGE_SIZE - (addr & (AT24C_PAGE_SIZE - 1)); /* [地址, 页边界]的大小 */
        if (size > remain) {
            size = remain;
        }

        if (write_in_page(bus, dev_addr, addr, data, size) != size) {
            break;
        }

        gd32_delay_ms(AT24C_WR_DELAY); /* 写延时 */

        addr += size;
        data += size;
        remain -= size;
    }

    if (EEPROM_USED_WP == 1) eeprom_wp_lock(); /* 写保护上锁 */

    return (len - remain);
}

/*
 * 功能描述：该函数在一页中向EEPROM设备写入若干字节
 * 入参说明：bus --- 总线编号，从0开始
 *           dev_addr --- 芯片的I2C地址
 *           mem_addr --- 内存地址，从0到AT24C_CHIP_SIZE - 1
 *           buff --- 存储数据的缓冲区
 *           len --- 数据长度（字节）
 * 返 回 值：成功写入的字节数
 */
static int write_in_page(int bus, uint32_t dev_addr, uint32_t mem_addr, void *buff, int len) {
    uint8_t addr[AT24C_ADDR_LEN]; /* 地址缓存 */
    int     ret;

    /* 存储地址 */
#if AT24C_CHIP_TYPE == AT24C02
    addr[0] = byte_0(mem_addr); /* LSB */
#else
    addr[0] = byte_1(mem_addr); /* MSB */
    addr[1] = byte_0(mem_addr); /* LSB */
#endif
    /* 传输开始 */
    si2c_start(bus);

    /* 传输从设备地址以进行写操作 */
    if (si2c_select(bus, dev_addr, SI2C_OP_WRITE) != 0) {
        si2c_stop(bus);
        return 0;
    }

    /* 传输存储地址 */
    if (si2c_tx(bus, addr, sizeof(addr)) != sizeof(addr)) {
        si2c_stop(bus);
        return 0;
    }

    /* 传输地址 */
    ret = si2c_tx(bus, buff, len);

    /* 传输停止 */
    si2c_stop(bus);

    return ret;
}
