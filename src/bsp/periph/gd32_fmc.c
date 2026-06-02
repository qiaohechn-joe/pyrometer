/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_fmc.c
 * 文件描述：FMC-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 李兆越 初始版本
 *           V1.1 2024/11/25 李超   添加页擦除功能
 *           V1.2 2025/03/27 李兆越 优化最后一字写入bug
 *           V1.3 2025/05/06 李兆越 加入快速写入函数
 */

#include "platform.h"
#include "gd32_fmc.h"
#include "hq_generic.h"
/*
 * 功能描述：擦除给定地址起始的多页
 * 入参说明：addr：起始地址，应该是页的整数倍
 *           num : 擦除总的页数，不能超过总的页数
 * 返 回 值：0 --- 擦除成功  -1 --- 参数addr有误   -2 擦除的页数超出范围
 *
 */
int gdf_erase_pages(uint32_t addr, uint32_t num) {
    uint16_t page_start = (addr - FLASH_BASE) / FMC_PAGE_SIZE; // 计算起始页数
    /* 判断参数是否合法 */
    if (addr % FMC_PAGE_SIZE != 0) {
        return -1;
    } else {
        if ((page_start + num) > FMC_PAGE_MAX) return -2;
    }

    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    for (uint32_t i = 0; i < num; i++) {
        fmc_page_erase(addr + FMC_PAGE_SIZE * i);
        fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    }
    fmc_lock();
    return 0;
}

/*
 * 功能描述：擦除给定扇区号的扇区
 * 入参说明：fmc_sector：给定的扇区号
 *           - CTL_SECTOR_NUMBER_0: 扇区 0
 *           - CTL_SECTOR_NUMBER_1: 扇区 1
 *           - CTL_SECTOR_NUMBER_2: 扇区 2
 *           - CTL_SECTOR_NUMBER_3: 扇区 3
 *           ……
 * 返 回 值：0 --- 擦除成功  其他 --- 擦除失败
 *
 */
int gdf_erase_sector(uint32_t fmc_sector) {
    fmc_unlock();
    /* 清除挂起标志 */
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    /* 等待擦除操作完成 */
    if (FMC_READY != fmc_sector_erase(fmc_sector)) {
        return -1;
    }
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    fmc_lock();

    return 0;
}

/*
 * 功能描述：向指定地址写入指定长度的数据（1字节方式）
 * 入参说明：addr --- 指定的地址（0x08000000~）
 *           len --- 数据长度
 *           buff --- 缓存指针
 * 返 回 值：0 --- 成功  其他 --- 失败
 *
 */
int gdf_write_data_by_byte(uint32_t addr, uint32_t len, uint8_t *buff) {
    int      i = 0;
    uint32_t new_addr;
    uint32_t data = 0;

    fmc_unlock();

    for (i = 0; i < len; i++) {
        data     = *(buff + i);
        new_addr = addr + i * sizeof(uint8_t);
        /* 1字节到编程 */
        if (FMC_READY != fmc_byte_program(new_addr, data)) {
            return -1;
        }
        /* 清除所有相关的标志 */
        fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    }
    fmc_lock();

    return 0;
}

/*
 * 功能描述：向指定地址写入指定长度的数据（4字节方式）
 * 入参说明：addr --- FLASH的写起始地址
 *           len --- 写数据长度（字节），必须是4*N
 *           buff --- 缓存指针
 * 返 回 值：0 --- 成功  其他 --- 失败
 * 备    注：单词写入错误，没有纠错机制，todo 待整改
 */
int gdf_write_data_by_word(uint32_t addr, int len, void *buff) {
    uint32_t *src = (uint32_t *)buff;

    len = len / 4;

    /* 清除所有相关的标志 */
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    fmc_unlock();
    while (len-- != 0) {
        /* 4字节编程 */
        int ret = fmc_word_program(addr, *src);
        if (FMC_READY != ret) {
            return -1;
        }
        src++;
        addr += 4;
    }
    fmc_lock();

    return 0;
}

/*
 * 功能描述：向指定地址写入指定长度的数据
 * 入参说明：addr --- FLASH的写起始地址
 *           len --- 写数据长度（字节），不限制长度是4的倍数
 *           buff --- 缓存指针
 * 返 回 值：0 --- 成功  其他 --- 失败
 *
 */
int gdf_write_data_by_page(uint32_t addr, int len, uint8_t *buff) {
    uint32_t *src  = (uint32_t *)buff;
    uint32_t  cnt  = 0;
    uint32_t  data = 0;
    uint32_t  num  = 0;

    num = len / FMC_PAGE_SIZE;
    if (len % FMC_PAGE_SIZE) {
        num++;
    }
    gdf_erase_pages(addr, num);

    cnt = len / 4;
    fmc_unlock();
    while (cnt-- != 0) {
        /* 4字节编程 */
        if (FMC_READY != fmc_word_program(addr, *src)) {
            return -1;
        }
        src++;
        addr += 4;
        /* 清除所有相关的标志 */
        fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    }
    buff += (len / 4) * 4;

    cnt = len % 4;
    for (uint8_t i = 0; i < cnt; i++) {
        data = *buff;
        /* 1字节到编程 */
        if (FMC_READY != fmc_byte_program(addr, data)) {
            return -1;
        }
        buff++;
        addr++;
        /* 清除所有相关的标志 */
        fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    }
    fmc_lock();

    return 0;
}

/*
 * 功能描述：向指定地址写入指定长度的数据
 * 入参说明：addr --- FLASH的写起始地址
 *           len --- 写数据长度（字节），不限制长度是4的倍数
 *           buff --- 缓存指针
 * 返 回 值：0 --- 成功  其他 --- 失败
 *
 */
int gdf_write_data_by_page_fast(uint32_t addr, int len, uint8_t *buff) {
    uint32_t *src  = (uint32_t *)buff;
    uint32_t  cnt  = 0;
    uint32_t  data = 0;

    cnt = len / 4;
    fmc_unlock();
    while (cnt-- != 0) {
        /* 4字节编程 */
        if (FMC_READY != fmc_word_program(addr, *src)) {
            return -1;
        }
        src++;
        addr += 4;
        /* 清除所有相关的标志 */
        fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    }
    buff += (len / 4) * 4;

    cnt = len % 4;
    for (uint8_t i = 0; i < cnt; i++) {
        data = *buff;
        /* 1字节到编程 */
        if (FMC_READY != fmc_byte_program(addr, data)) {
            return -1;
        }
        buff++;
        addr++;
        /* 清除所有相关的标志 */
        fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    }
    fmc_lock();

    return 0;
}

/*
 * 功能描述：从指定地址读取指定长度的数据
 * 入参说明：addr --- 指定的地址（0x08000000~）
 *           len --- 数据长度
 *           buff --- 缓存指针
 * 返 回 值：无
 *
 */
void gdf_read_data(uint32_t addr, uint32_t len, uint8_t *buff) {
    int      i;
    uint32_t new_addr;

    for (i = 0; i < len; i++) {
        new_addr = addr + i * sizeof(uint8_t); /* 计算新的读取地址 */
        buff[i]  = *(__IO uint8_t *)new_addr;  /* 从内存地址读取数据到缓冲区 */
    }
}

/*
 * 功能描述：从指定的Flash地址读取数据到目标缓冲区
 * 入参说明：addr --- Flash源地址
 *           buff --- 目标缓冲区指针
 *           len --- 读取字节数
 * 返 回 值：无
 */
int flash_read(uint32_t addr, void *buff, int len) {
    mem_copy(buff, (const void *)addr, len);

    return 0;
}

/*
 * 功能描述：从指定的Flash地址读取数据到目标缓冲区
 * 入参说明：addr --- Flash源地址
 *           buff --- 目标缓冲区指针
 *           len --- 读取字节数
 * 返 回 值：无
 */
int flash_write(uint32_t addr, void *buff, int len) {
    if (gdf_write_data_by_page(addr, len, buff)) {
        dbg_error("FMC:read error!");
        return -1;
    }

    return 0;
}

/*
 * 功能描述：根据输入的地址给出它所在的扇区
 * 入参说明：addr --- 给出的地址
 * 返 回 值：扇区号
 *
 */
uint32_t gdf_get_sector(uint32_t addr) {
    uint32_t sector = 0;

    if ((addr < FLASH_SECT1_BASE) && (addr >= FLASH_SECT0_BASE)) {
        sector = CTL_SECTOR_NUMBER_0;
    } else if ((addr < FLASH_SECT2_BASE) && (addr >= FLASH_SECT1_BASE)) {
        sector = CTL_SECTOR_NUMBER_1;
    } else if ((addr < FLASH_SECT3_BASE) && (addr >= FLASH_SECT2_BASE)) {
        sector = CTL_SECTOR_NUMBER_2;
    } else if ((addr < FLASH_SECT4_BASE) && (addr >= FLASH_SECT3_BASE)) {
        sector = CTL_SECTOR_NUMBER_3;
    } else if ((addr < FLASH_SECT5_BASE) && (addr >= FLASH_SECT4_BASE)) {
        sector = CTL_SECTOR_NUMBER_4;
    } else if ((addr < FLASH_SECT6_BASE) && (addr >= FLASH_SECT5_BASE)) {
        sector = CTL_SECTOR_NUMBER_5;
    } else if ((addr < FLASH_SECT7_BASE) && (addr >= FLASH_SECT6_BASE)) {
        sector = CTL_SECTOR_NUMBER_6;
    } else if ((addr < FLASH_SECT8_BASE) && (addr >= FLASH_SECT7_BASE)) {
        sector = CTL_SECTOR_NUMBER_7;
    } else if ((addr < FLASH_SECT9_BASE) && (addr >= FLASH_SECT8_BASE)) {
        sector = CTL_SECTOR_NUMBER_8;
    } else if ((addr < FLASH_SECT10_BASE) && (addr >= FLASH_SECT9_BASE)) {
        sector = CTL_SECTOR_NUMBER_9;
    } else if ((addr < FLASH_SECT11_BASE) && (addr >= FLASH_SECT10_BASE)) {
        sector = CTL_SECTOR_NUMBER_10;
    } else /*((addr < FLASH_END_ADDR) && (addr >= ADDR_FLASH_SECTOR_11))*/
    {
        sector = CTL_SECTOR_NUMBER_11;
    }

    return sector;
}
