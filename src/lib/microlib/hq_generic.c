/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：hq_generic.c
 * 文件描述：通用例程库
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：V1.0 2024/01/26 陈  军  初始版本
 *           V1.1 2025/02/21 李兆越  加入温度单位转换函数
 */

#include "hq_generic.h"
#include "hq_io.h"
#include "hq_string.h"

/* CRC16-CCITT查找表：Poly = X^16 + X^12 + X^5 + 1(0x11021) */
static const unsigned short CRC16_CCITT_TABLE[] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
};

/* CRC16-IBM查找表：Poly = X^16 + X^15 + X^2 + 1(0x18005) */
static const unsigned short CRC16_IBM_TABLE[] = {
    0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011, 0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027, 0x0022,
};

/* CRC16-RTU查找表：Poly = X^16 + X^15 + X^13 + 1(0x1A001) */
static const unsigned short CRC16_RTU_TABLE[] = {
    0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401, 0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400,
};

/*
 * 功能描述：此函数反转一个字节的位顺序
 * 入参说明：data --- 要反转的数据
 * 返 回 值：结果
 */
unsigned char reverse_u8(unsigned char data) {
    unsigned char ret = 0;
    int           i;

    for (i = 8; i != 0; i--) {
        ret <<= 1;

        if (data & 0x01) { /* data[0]为"1" */
            ret |= 0x01;   /* 设置ret[0]为"1" */
        }

        data >>= 1;
    }

    return ret;
}

/*
 * 功能描述：此函数反转一个字的位顺序
 * 入参说明：data --- 要反转的数据
 * 返 回 值：结果
 */
unsigned short reverse_u16(unsigned short data) {
    unsigned int ret = 0;
    int          i;

    for (i = 16; i != 0; i--) {
        ret <<= 1;

        if (data & 0x01) { /* data[0]为"1" */
            ret |= 0x01;   /* 设置ret[0]为"1" */
        }

        data >>= 1;
    }

    return ret;
}

/*
 * 功能描述：计算无符号字符数组的校验和
 * 入参说明：data --- 指向数组的指针
 *           len --- 数组的长度
 * 返 回 值：校验和
 */
unsigned char checksum_u8(const void *data, int len) {
    unsigned int         sum = 0;
    const unsigned char *src = (const unsigned char *)data;

    while (len != 0) {
        sum += (unsigned int)(*src++);
        len--;
    }

    return sum;
}

/*
 * 功能描述：计算无符号短整型数组的校验和
 * 入参说明：data --- 指向数组的指针
 *           len --- 数组的长度
 * 返 回 值：校验和
 */
unsigned short checksum_u16(const void *data, int len) {
    unsigned int          sum = 0;
    const unsigned short *src = (const unsigned short *)data;

    while (len != 0) {
        sum += (unsigned int)(*src++);
        len -= 2;
    }

    return (unsigned short)sum;
}

/*
 * 功能描述：计算字节流的CRC16(CCITT)校验和
 * 入参说明：buff --- 指向数组的指针
 *           len --- 数组的长度
 * 返 回 值：校验和
 * 备    注：Poly = X^16 + X^12 + X^5 + 1(0x11021)
 */
unsigned short crc16_CCITT(const void *buff, int len) {
    unsigned short       crc  = 0;
    const unsigned char *data = (const unsigned char *)buff;

    while (len-- != 0) {
        crc = (crc << 4) ^ CRC16_CCITT_TABLE[(crc >> 12) ^ ((*data >> 4) & 0x0f)];
        crc = (crc << 4) ^ CRC16_CCITT_TABLE[(crc >> 12) ^ (*data & 0x0f)];
        data++;
    }

    return crc;
}

/*
 * 功能描述：计算字节流的CRC16(IBM)校验和
 * 入参说明：buff --- 指向数组的指针
 *           len --- 数组的长度
 * 返 回 值：校验和
 * 备 注：Poly = X^16 + X^15 + X^2 + 1(0x18005)
 */
unsigned short crc16_IBM(const void *buff, int len) {
    unsigned short       crc  = 0;
    const unsigned char *data = (const unsigned char *)buff;

    while (len-- != 0) {
        crc = (crc << 4) ^ CRC16_IBM_TABLE[(crc >> 12) ^ ((*data >> 4) & 0x0f)];
        crc = (crc << 4) ^ CRC16_IBM_TABLE[(crc >> 12) ^ (*data & 0x0f)];
        data++;
    }

    return crc;
}

/*
 * 功能描述：此函数计算字节流的CRC16（ModBus RTU, 0xA001）
 * 入参说明：buff --- 数据缓冲区
 *           len --- 数据长度（字节）
 * 返 回 值：CRC校验结果
 */
unsigned short crc16_RTU(const void *buff, int len) {
    const unsigned char *data = (const unsigned char *)buff;
    unsigned short       crc  = 0x0ffff;
    unsigned int         idx;

    while (len-- != 0) {
        idx = (crc & 0x0f) ^ (*data & 0x0f);
        crc = crc >> 4;
        crc ^= CRC16_RTU_TABLE[idx];
        idx = (crc & 0x0f) ^ ((*data >> 4) & 0x0f);
        crc = crc >> 4;
        crc ^= CRC16_RTU_TABLE[idx];
        data++;
    }

    return crc;
}

/*
 * 功能描述：此函数将内存块复制到另一个内存块
 * 入参说明：dest --- 目标指针
 *           src --- 源指针
 *           len --- 要复制的字节数
 * 返 回 值：指向目标末尾的指针
 * 备    注：不检查参数，因此dest和src必须是有效的指针
 */
void *mem_copy(void *dest, const void *src, int len) {
    const unsigned char *ps = (const unsigned char *)src;
    unsigned char       *pd = (unsigned char *)dest;

    while (len-- != 0) {
        *pd++ = *ps++;
    }

    return pd;
}

/*
 * 功能描述：此函数使用一个固定值填充内存块
 * 入参说明：dest --- 目标指针
 *           len --- 要填充的长度
 *           data --- 要填充的数据
 * 返 回 值：指向目标末尾的指针
 * 备    注：不检查参数，因此 dest 必须是有效的指针
 */
void *mem_set(void *dest, int len, unsigned char data) {
    unsigned char *pd = (unsigned char *)dest;

    while (len-- != 0) {
        *pd++ = data;
    }

    return pd;
}

/*
 * 功能描述：此函数比较两个内存区域
 * 入参说明：mem1, mem2 --- 要比较的两个内存区域
 *           len --- 比较的长度
 * 返 回 值：0 --- mem1 == mem2，-1 --- mem1 < mem2，1 --- mem1 > mem2
 */
int mem_comp(const void *mem1, const void *mem2, int len) {
    const unsigned char *ptr1 = (const unsigned char *)mem1;
    const unsigned char *ptr2 = (const unsigned char *)mem2;

    while (len-- != 0) {
        if (*ptr1 < *ptr2) {
            return -1;
        } else if (*ptr1 > *ptr2) {
            return 1;
        }

        ptr1++;
        ptr2++;
    }

    return 0;
}

/*
 * 功能描述：此函数在内存区域中搜索一个固定的字节
 * 入参说明：start --- 内存区域的起始地址
 *           len --- 要搜索的长度
 *           data --- 要搜索的字节数据
 * 返 回 值：指向第一个出现的搜索字节的指针，如果未找到则返回 0
 */
void *mem_search(void *start, int len, unsigned char data) {
    unsigned char *pos = (unsigned char *)start;

    while (len != 0) {
        if (*pos == data) {
            break;
        }
        pos++;
        len--;
    }

    return (len == 0) ? 0 : pos;
}

/*
 * 功能描述：此函数在缓冲区中搜索一个字符串
 * 入参说明：buff --- 缓冲区的起始地址
 *           len --- 搜索的长度（字节）
 *           str --- 子字符串，以零结尾
 * 返 回 值：指向字符串第一个出现的位置的指针，如果未找到则返回0
 */
void *mem_search_s(void *buff, int len, const char *str) {
    int   str_len;
    char *pos;

    str_len = string_length(str);
    pos     = (char *)buff;

    while (len >= str_len) {
        if (mem_comp(pos, str, str_len) == 0) {
            break;
        }
        pos++;
        len--;
    }

    return (len < str_len) ? 0 : pos;
}

/*
 * 功能描述：此函数在 cmd_table 中搜索，然后执行与命令表中任何条目匹配的命令。
 * 入参说明：cmd_buffer --- 命令缓冲区，格式："command_id<SPACE>command_parameter ..."
 *           cmd_table --- 包含所有命令的命令表。
 * 返 回 值：COMMAND_SUCCESS(0) ---- 找到并成功执行命令
 *           COMMAND_FAILED(1) ---- 命令执行失败
 *           COMMAND_NOT_FOUND(3) ---- 未找到命令
 */
int command_execute(char *cmd_buffer, const struct command_entry_t *cmd_table) {
    char *start;
    char *pos;
    int   argc;
    char *argv[CMD_ARG_MAX_CNT];
    int   ret;

    /* parse cmd_buffer */
    argc  = 0;
    start = cmd_buffer;
    pos   = start;
    for (;;) {
        /* search separator(' ' or  '\t') or terminator ('\r' or '\n') */
        while ((*pos != '\0') && (*pos != ' ') && (*pos != '\t') && (*pos != '\r') && (*pos != '\n')) {
            pos++;
        }
        if (pos != start) {
            argv[argc++] = start;
        }

        if ((*pos == '\0') || (*pos == '\r') || (*pos == '\n') || (argc == CMD_ARG_MAX_CNT)) { /* terminator */
            *pos = '\0';
            break;
        }

        if ((*pos == ' ') || (*pos == '\t')) { /* separator */
            *pos++ = '\0';
            while ((*pos == ' ') || (*pos == '\t')) {
                pos++;
            }
            start = pos;
        }
    }

    /* search all entries in command table */
    ret = CMD_NOT_FOUND;
    while (cmd_table->id != 0) {
        if (string_comp(cmd_table->id, argv[0]) == 0) { /* id matched,command founded */
            if (cmd_table->function) {
                ret = cmd_table->function(argc, argv);
                break;
            }
        }

        cmd_table++;
    }

    return ret;
}

/*
 * 功能描述：计算整型数组的平均值
 * 入参说明：array --- 整型数组的指针
 *           cnt --- 数组的元素数量
 * 返 回 值：平均值（整型）
 */
int average(int *array, unsigned int cnt) {
    long long int sum = 0;

    for (int i = 0; i < cnt; i++) {
        sum += array[i];
    }

    return (int)(sum / cnt);
}

/*
 * 功能描述：计算浮点数组的平均值
 * 入参说明：array --- 浮点数组的指针
 *           cnt --- 数组中的元素数量
 * 返 回 值：平均值（浮点型）
 */
float average_f(float *array, unsigned int cnt) {
    double sum;
    int    i;

    if (cnt == 0) {
        return 0;
    }

    /* 计算平均值 */
    sum = 0;
    for (i = 0; i < cnt; i++) {
        sum += *array++;
    }

    return (float)(sum / cnt);
}

#include <stdlib.h>
#include <time.h>

/*
 * 功能描述：返回指定范围内的随机浮动值
 * 入参说明：min  --- 随机值的下限
 *           max  --- 随机值的上限
 * 返 回 值：在[min, max]范围内的随机浮动值（浮点型）
 * 备    注：耗时420ns==0.42us
 */
float get_random_value(float min, float max) {
    /* 生成一个0到1之间的随机数 */
    float rand_fraction = (float)rand() / (float)RAND_MAX;

    /* 将随机数映射到[min, max]范围内 */
    return min + rand_fraction * (max - min);
}

/*
 * 功能描述：将摄氏度转换为指定单位（华氏度或开尔文）
 * 入参说明：c - 摄氏度值
 *           unit - 转换目标单位，'C' 为摄氏度，'F' 为华氏度，'K' 为开尔文
 * 返 回 值：转换后的温度值
 */
float convert_celsius(float c, char unit) {
    float result = c; // 默认返回摄氏度

    /* 根据目标单位转换温度 */
    if (unit == 'F') {
        /* 摄氏度转华氏度 */
        result = C_TO_F(c);
    } else if (unit == 'K') {
        /* 摄氏度转开尔文 */
        result = C_TO_K(c);
    }

    return result;
}
/*
 * 功能描述：将摄氏度转换为指定单位（华氏度或开尔文）
 * 入参说明：c - 摄氏度值
 *           unit - 转换目标单位，'C' 为摄氏度，'F' 为华氏度，'K' 为开尔文
 * 返 回 值：转换后的温度值
 */
float convert_uint(float c, char unit_src, char uint_target) {
    float result = c; // 默认返回自身

    /* 根据目标单位转换温度 */
    if (unit_src == 'C') {
        if (uint_target == 'F') {
            result = C_TO_F(c);
        } else if (uint_target == 'K') {
            result = C_TO_K(c);
        }

    } else if (unit_src == 'F') {
        if (uint_target == 'C') {
            result = F_TO_C(c);
        } else if (uint_target == 'K') {
            result = F_TO_K(c);
        }

    } else if (unit_src == 'K') {
        if (uint_target == 'F') {
            result = K_TO_F(c);
        } else if (uint_target == 'C') {
            result = K_TO_C(c);
        }
    }

    return result;
}
