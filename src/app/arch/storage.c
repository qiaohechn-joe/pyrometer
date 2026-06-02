/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：storage.c
 * 文件描述：SPI-FLASH和EEPROM存储管理
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/05/10 陈军  增加SPI-FLASH支持
 *           V1.2 2024/05/30 陈军  去掉不必要的头文件
 *           V1.3 2024/06/05 陈军  对应存储设备头文件移入条件语句中
 */

#include "storage.h"

static uint8_t test_buffer[MEM_TEST_SIZE]; /* 存储测试缓存 */

#if SUPPORT_EEPROM
static SemaphoreHandle_t eeprom_mutex;
#endif /* SUPPORT_EEPROM */

#if SUPPORT_SPIFLASH
static SemaphoreHandle_t sf_mutex;
static struct w25_chip_t sf_chip[SF_CHIP_CNT];
static uint8_t           sect_buffer[W25_SECT_SIZE]; /* SPI-Flash sector buffer */
#endif                                               /* SUPPORT_SPIFLASH */

/*
 * 功能描述：此函数初始化内存单元（SPI-FLASH和EEPROM）
 * 入参说明：无
 * 返 回 值：0 --- 成功；其他 --- 失败
 */
int storage_init(void) {
#if SUPPORT_EEPROM
    eeprom_mutex = xSemaphoreCreateMutex();
    if (eeprom_mutex == NULL) {
        return -1;
    }

    si2c_init(EEPROM_I2C_BUS);

#endif /* SUPPORT_EEPROM */

#if SUPPORT_SPIFLASH
    int i;

    /* data structure */
    mem_set(sf_chip, sizeof(sf_chip), 0);
    sf_mutex = xSemaphoreCreateMutex();
    if (sf_mutex == NULL) {
        vSemaphoreDelete(eeprom_mutex);
        return -1;
    }

    /* hardware */
    sf_init_spi();
    sf_init_cs();

    /* probe SPI-FLASH */
    sf_chip[0].spi     = SF_SPI_DEV;
    sf_chip[0].cs_port = SF_CS1_PORT;
    sf_chip[0].cs_pin  = SF_CS1_PIN;

    for (i = 0; i < SF_CHIP_CNT; i++) {
        if (w25_probe(&sf_chip[i]) == 0) {
            dbg_info("Memory: SPI-Flash chip%d found, type=0x%04X(%s), JEDC-ID=0x%04X\r\n", i + 1, sf_chip[i].type, sf_chip[i].name, sf_chip[i].jid);
        }
    }
#endif /* SUPPORT_SPIFLASH */

    return 0;
}

#if SUPPORT_EEPROM
/*
 * 功能描述：此函数从EEPROM读取若干字节
 * 入参说明：addr --- 内存地址
 *           buff --- 存放数据的缓冲区
 *           len ---- 数据长度，以字节为单位
 * 返 回 值：成功读取的字节数
 */
int eeprom_read(uint32_t addr, void *buff, int len) {
    return at24c_read(EEPROM_I2C_BUS, EEPROM_I2C_ADDR, addr, buff, len);
}

/*
 * 功能描述：此函数是向EEPROM写入若干字节
 * 入参说明：addr---内存地址
 *           buff---存放数据的缓冲区
 *           len----数据长度（字节）
 * 返 回 值：成功写入的字节数
 */
int eeprom_write(uint32_t addr, void *buff, int len) {
    return at24c_write(EEPROM_I2C_BUS, EEPROM_I2C_ADDR, addr, buff, len);
}

/*
 * 功能描述：此函数获取EEPROM的互斥锁
 * 入参说明：无
 * 返 回 值：0 --- 成功；其他 --- 失败
 */
int eeprom_lock(void) {
    BaseType_t ret;

    /* 获取互斥锁 */
    ret = xSemaphoreTake(eeprom_mutex, EEPROM_MUTEX_TIMEOUT);

    return (ret == pdPASS) ? 0 : -1;
}

/*
 * 功能描述：此函数释放EEPROM的互斥锁
 * 入参说明：无
 * 返 回 值：0 --- 成功；其他 --- 失败
 */
int eeprom_unlock(void) {
    BaseType_t ret;

    /* 释放互斥锁 */
    ret = xSemaphoreGive(eeprom_mutex);

    return (ret == pdPASS) ? 0 : -1;
}

/*
 * 功能描述：此函数从EEPROM读取数据（受互斥锁保护）
 * 入参说明：addr --- 起始地址
 *           buff --- 数据缓冲区
 *           len --- 数据长度，以字节为单位
 * 返 回 值：成功返回0，失败返回-1
 */
int eeprom_read_lock(uint32_t addr, void *buff, int len) {
    int ret;

    if (PLATFORM_USE_OS == 1) {
        if (eeprom_lock()) {
            return 0;
        }
    }

    ret = eeprom_read(addr, buff, len);
    if (PLATFORM_USE_OS == 1) {
        eeprom_unlock(); // todo：加上异常判断
    }
    return (ret > 0) ? 0 : -1;
}

/*
 * 功能描述：此函数向EEPROM写入数据（受互斥锁保护）
 * 入参说明：addr --- 起始地址
 *           buff --- 数据缓冲区
 *           len --- 数据长度，以字节为单位
 * 返 回 值：成功返回0，失败返回-1
 */
int eeprom_write_lock(uint32_t addr, void *buff, int len) {
    int ret;
    if (PLATFORM_USE_OS == 1) {
        if (eeprom_lock()) {
            return 0;
        }
    }

    ret = eeprom_write(addr, buff, len);
    if (PLATFORM_USE_OS == 1) {
        eeprom_unlock();
    }
    return (ret > 0) ? 0 : -1;
}

/*
 * 功能描述：此函数测试EEPROM
 * 入参说明：无
 * 返 回 值：0 --- 成功；其他 --- 失败
 */
int eeprom_test(void) {
    int      len;
    uint8_t *data;
    uint32_t addr = EEPROM_TEST_ADDR;

    /* 填充测试缓冲区并写入 */
    mem_set(test_buffer, sizeof(test_buffer), 0x5A);

    eeprom_write(addr, test_buffer, sizeof(test_buffer));

    /* 读取并校验 */
    mem_set(test_buffer, sizeof(test_buffer), 0x00);

    len = eeprom_read(addr, test_buffer, sizeof(test_buffer));

    if (len != sizeof(test_buffer)) {
        return -1;
    }

    for (data = test_buffer; len != 0; len--) {
        if (*data++ != 0x5A) {
            break;
        }
    }

    return (len == 0) ? 0 : -1;
}
#endif /* SUPPORT_EEPROM */

#if SUPPORT_SPIFLASH
/*
 * 功能描述：此函数从SPI-Flash读取数据
 * 入参说明：addr --- 起始地址
 *           buff --- 数据缓冲区
 *           len  --- 数据长度，单位：字节
 * 返 回 值：成功读取的字节数
 */
int spif_read(uint32_t addr, void *buff, int len) {
    struct w25_chip_t *chip;

    /* 查找芯片 */
    chip = &sf_chip[0];
    while (addr >= chip->size) {
        addr -= chip->size;
        chip++;
    }

    w25_read(chip, addr, buff, len);

    return len;
}

/*
 * 功能描述：此函数将数据写入 SPI-Flash
 * 入参说明：addr --- 起始地址
 *           buff --- 数据缓冲区
 *           len  --- 数据长度，单位：字节
 * 返 回 值：成功写入的字节数
 */
int spif_write(uint32_t addr, const void *buff, int len) {
    struct w25_chip_t *chip;
    uint32_t           sect; /* 扇区起始地址 */
    uint32_t           off;  /* 扇区内偏移量 */
    uint32_t           size;
    uint32_t           pgm_addr; /* 页编程使用的地址 */
    const uint8_t     *data;
    const uint8_t     *pgm_ptr; /* 页编程使用的指针 */

    /* 查找芯片 */
    chip = &sf_chip[0];
    while (addr >= chip->size) {
        addr -= chip->size;
        chip++;
    }

    data = (const uint8_t *)buff;
    sect = addr & ~W25_SECT_MASK;
    off  = addr & W25_SECT_MASK;
    size = (len < (W25_SECT_SIZE - off)) ? len : (W25_SECT_SIZE - off); /* 首段数据 */

    while (len != 0) {
        if (size < W25_SECT_SIZE) /* 非完整扇区 */
        {
            w25_read(chip, sect, sect_buffer, W25_SECT_SIZE);
        }

        mem_copy(sect_buffer + off, data, size); /* 修改数据 */

        /* 擦除扇区 */
        if (w25_enable_write(chip)) {
            goto over;
        }
        w25_erase_sector(chip, sect);
        if (w25_wait_ready(chip, W25_ERASE_TIMEOUT)) {
            goto over; /* 超时 */
        }

        /* 编程扇区 */
        pgm_addr = sect;
        pgm_ptr  = sect_buffer;
        while (pgm_addr < (sect + W25_SECT_SIZE)) {
            if (w25_enable_write(chip)) {
                goto over;
            }
            w25_page_pgm(chip, pgm_addr, (void *)pgm_ptr, W25_PAGE_SIZE);
            if (w25_wait_ready(chip, W25_PGM_TIMEOUT)) {
                goto over; /* 超时 */
            }

            pgm_addr += W25_PAGE_SIZE;
            pgm_ptr += W25_PAGE_SIZE;
        }

        data += size;
        len -= size;
        sect += W25_SECT_SIZE;
        off  = 0;
        size = (len < W25_SECT_SIZE) ? len : W25_SECT_SIZE;
    }

over:

    return (data - (const uint8_t *)buff);
}

/*
 * 功能描述：此函数获取 SPI-Flash 的互斥锁
 * 入参说明：无
 * 返 回 值：0 --- 成功   其他 --- 失败
 */
int spif_lock(void) {
    BaseType_t ret;

    /* 获取互斥锁 */
    ret = xSemaphoreTake(sf_mutex, EEPROM_MUTEX_TIMEOUT);

    return (ret == pdPASS) ? 0 : -1;
}

/*
 * 功能描述：此函数释放 SPI-Flash 的互斥锁
 * 入参说明：无
 * 返 回 值：0 --- 成功   其他 --- 失败
 */
int spif_unlock(void) {
    BaseType_t ret;

    /* 释放互斥锁 */
    ret = xSemaphoreGive(sf_mutex);

    return (ret == pdPASS) ? 0 : -1;
}

/*
 * 功能描述：此函数从 AT45DBxxx 中读取数据（通过互斥锁保护）
 * 入参说明：addr --- 起始地址
 *           buff --- 数据缓冲区
 *           len  --- 数据长度，字节数
 * 返 回 值：成功读取的字节数
 */
int spif_read_lock(uint32_t addr, void *buff, int len) {
    int ret;

    /* 获取互斥锁 */
    if (spif_lock()) {
        return 0;
    }

    /* 从指定地址读取数据 */
    ret = spif_read(addr, buff, len);

    /* 释放互斥锁 */
    spif_unlock();

    return ret;
}

/*
 * 功能描述：此函数写入 SPI-Flash（通过互斥锁保护）
 * 入参说明：addr --- 起始地址
 *           buff --- 数据缓冲区
 *           len  --- 数据长度，字节数
 * 返 回 值：成功写入的字节数
 */
int spif_write_lock(uint32_t addr, const void *buff, int len) {
    int ret;

    /* 获取互斥锁 */
    if (spif_lock()) {
        return 0;
    }

    /* 写入指定地址的数据 */
    ret = spif_write(addr, buff, len);

    /* 释放互斥锁 */
    spif_unlock();

    return ret;
}

/*
 * 功能描述：此函数测试 SPI-Flash 存储器
 * 入参说明：无
 * 返 回 值：0 --- 成功  其它 --- 失败
 */
int spif_test(void) {
    int      len;
    uint8_t *data;
    uint32_t addr = SF_TEST_ADDR;

    /* 填充测试缓冲区并写入 */
    mem_set(test_buffer, sizeof(test_buffer), 0xA5);
    spif_write_lock(addr, test_buffer, sizeof(test_buffer));

    /* 读取并检查 */
    mem_set(test_buffer, sizeof(test_buffer), 0x00);
    len = spif_read_lock(addr, test_buffer, sizeof(test_buffer));
    if (len != sizeof(test_buffer)) {
        return -1;
    }

    for (data = test_buffer; len != 0; len--) {
        if (*data++ != 0xA5) {
            break;
        }
    }

    return (len == 0) ? 0 : -1;
}

#endif /* SUPPORT_SPIFLASH */
