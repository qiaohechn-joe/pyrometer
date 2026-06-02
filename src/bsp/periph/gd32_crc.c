/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_crc.c
 * 文件描述：ADC-CRC层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/01/03 李兆越  初始版本
 */

#include "platform.h"
#include "gd32_ll.h"
#include "gd32_crc.h"

/* 字节序转换 */
uint32_t reverse_word(uint32_t value) {
    return __REV(value);
}

/*
 * 功能描述：计算固件包的 CRC32 值，支持大小端模式。该函数将从指定地址开始的内存区域按字节长度计算 CRC32 校验值，
 *           支持小端（低地址存低位字节）和大端（高地址存高位字节）两种数据排列方式，并在不足 4 字节时自动补零。
 * 入参说明：
 *   addr     --- 固件存储起始地址（Flash 指针），指向待计算 CRC 的原始数据缓冲区
 *   len      --- 数据长度（单位：字节），允许非 4 字节对齐长度
 *   endian   --- 大小端模式（ENDIAN_LITTLE / ENDIAN_BIG）
 * 返 回 值：CRC32 值，即经过硬件 CRC 模块处理后得到的 32 位校验结果
 */
unsigned long calculate_crc32(uint8_t *addr, int len, EndianMode endian) {
    const uint32_t *addr32;

    // 初始化 CRC 单元
    crc_data_register_reset();

    // 主循环：每次处理 4 字节
    while (len >= 4) {
        addr32        = (const uint32_t *)addr;
        uint32_t word = *addr32;

        if (endian == ENDIAN_BIG) {
            word = reverse_word(word); // 使用 __REV 反转字节序
        }

        CRC_DATA = word;

        addr += 4;
        len -= 4;
    }

    // 处理剩余不足 4 字节部分，补零
    if (len > 0) {
        uint32_t word = *(const uint32_t *)addr;

        switch (len) {
            case 1: word &= 0xFF000000; break;
            case 2: word &= 0xFFFF0000; break;
            case 3: word &= 0xFFFFFF00; break;
        }

        if (endian == ENDIAN_BIG) {
            word = reverse_word(word); // 使用 __REV 反转字节序
        }

        CRC_DATA = word;
    }

    return CRC_DATA;
}
