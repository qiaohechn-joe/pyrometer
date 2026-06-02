/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：hq_generic.h
 * 文件描述：通用例程库
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2025/02/21 李兆越  加入温度单位转换函数
 */

#ifndef _HQ_GENERIC_H_
#define _HQ_GENERIC_H_

/* ASCII码 */
#define CH_BKSPACE                0x08
#define CH_DEL                    0x7F
#define CH_ESC                    0x1B

/* 命令参数计数 */
#define CMD_ARG_MAX_CNT           8

/* 命令结果 */
#define CMD_SUCCESS               0
#define CMD_PARAMETER             1
#define CMD_FAILED                2
#define CMD_NOT_FOUND             3
#define CMD_NO_RESP               4

/* b0为最低字节，b3为最高字节 */
#define byte_0(n)                 ((n) & 0x0FF)         /* 最低字节 */
#define byte_1(n)                 (((n) >> 8) & 0x0FF)  /* 次低字节 */
#define byte_2(n)                 (((n) >> 16) & 0x0FF) /* 次高字节 */
#define byte_3(n)                 (((n) >> 24) & 0x0FF) /* 最高字节 */

#define min(a, b)                 (((a) < (b)) ? (a) : (b))
#define max(a, b)                 (((a) > (b)) ? (a) : (b))

#define make_u16(b0, b1)          ((unsigned short)(b0) | ((unsigned short)(b1) << 8))
#define make_u32(b0, b1, b2, b3)  ((unsigned int)(b0) | ((unsigned int)(b1) << 8) | ((unsigned int)(b2) << 16) | ((unsigned int)(b3) << 24))

/* BCD：范围0~99 */
#define to_BCD(n)                 ((((n) / 10) << 4) | ((n) % 10))
#define from_BCD(n)               (((((n) >> 4) & 0x0f) * 10) + ((n) & 0x0f))

#define hex_to_char(n)            ((n) > 9) ? ((n) - 0x0A + 'A') : ((n) + '0')

/* 结构体成员的偏移量：m=成员，s=结构体 */
// #define offset_of(m, s)          ((unsigned int)&(((s *)0)->m))
#define offset_of(m, s)           offsetof(s, m)

/* 从成员地址获取结构体地址：p=指针，s=结构体，m=成员 */
#define container_of(p, m, s)     (s *)((unsigned int)p - offset_of(m, s))

/* 通用读写操作 */
#define readl(addr)               (*((volatile const unsigned int *)(addr)))
#define reads(addr)               (*((volatile const unsigned short *)(addr)))
#define readb(addr)               (*((volatile const unsigned char *)(addr)))
#define writel(addr, val)         (*((volatile unsigned int *)(addr))) = (val)
#define writes(addr, val)         (*((volatile unsigned short *)(addr))) = (val)
#define writeb(addr, val)         (*((volatile unsigned char *)(addr))) = (val)

/* 位映射操作 */
/* bm --- 位映射数组，bit --- 位数 */

/* 8位位映射，bm: 无符号字符数组 */
#define bm8_set_bit(bm, bit)      (bm)[(bit) >> 3] |= (1 << ((bit) & 0x07))
#define bm8_clear_bit(bm, bit)    (bm)[(bit) >> 3] &= ~(1 << ((bit) & 0x07))
#define bm8_bit_set(bm, bit)      ((bm)[(bit) >> 3] & (1 << ((bit) & 0x07)))
#define bm8_bit_clear(bm, bit)    (!bm8_bit_set(bm, bit))

/* 16位位映射，bm: 无符号短整型数组 */
#define bm16_set_bit(bm, bit)     (bm)[(bit) >> 4] |= (1 << ((bit) & 0x0F))
#define bm16_clear_bit(bm, bit)   (bm)[(bit) >> 4] &= ~(1 << ((bit) & 0x0F))
#define bm16_bit_set(bm, bit)     ((bm)[(bit) >> 4] & (1 << ((bit) & 0x0F)))
#define bm16_bit_clear(bm, bit)   (!bm16_bit_set(bm, bit))

/* 32位位映射，bm: 无符号长整型数组 */
#define bm32_set_bit(bm, bit)     (bm)[(bit) >> 5] |= (1 << ((bit) & 0x1F))
#define bm32_clear_bit(bm, bit)   (bm)[(bit) >> 5] &= ~(1 << ((bit) & 0x1F))
#define bm32_bit_set(bm, bit)     ((bm)[(bit) >> 5] & (1 << ((bit) & 0x1F)))
#define bm32_bit_clear(bm, bit)   (!bm32_bit_set(bm, bit))

/* 温度单位转换 */
#define C_TO_K(c)                 ((float)((c) * 100 + 27315) / 100)
#define C_TO_F(c)                 ((float)((c) * 1800 + 32000) / 1000)

#define F_TO_C(f)                 ((float)((f * 1000) - 32000) / 1800)
#define F_TO_K(f)                 ((float)((((float)(f) - 32.0f) * 5.0f / 9.0f) + 273.15f))

#define K_TO_C(k)                 ((float)((k) * 100 - 27315) / 100)
#define K_TO_F(k)                 ((float)(((float)(k) - 273.15f) * 9.0f / 5.0f) + 32.0f)

/* 命令表定义 */
#define BEGIN_COMMAND_TABLE(name) const struct command_entry_t name[] = {
#define COMMAND_ENTRY(id, func)   {id, func},
#define END_COMMAND_TABLE                                                                                                                            \
    { 0, 0 }                                                                                                           \
    }                                                                                                                                                \
    ;

/* 命令表入口函数定义 */
struct command_entry_t {
    const char *id;                         /* 命令ID（名字） */
    int (*function)(int argc, char **argv); /* 命令功能函数 */
};

unsigned char  reverse_u8(unsigned char data);
unsigned short reverse_u16(unsigned short data);
unsigned char  checksum_u8(const void *data, int len);
unsigned short checksum_u16(const void *data, int len);
unsigned short crc16_CCITT(const void *buff, int len);
unsigned short crc16_IBM(const void *buff, int len);
unsigned short crc16_RTU(const void *buff, int len);
void          *mem_copy(void *dest, const void *src, int len);
void          *mem_set(void *dest, int len, unsigned char data);
int            mem_comp(const void *mem1, const void *mem2, int len);
void          *mem_search(void *start, int len, unsigned char data);
void          *mem_search_s(void *buff, int len, const char *str);
int            command_execute(char *cmd_buffer, const struct command_entry_t *cmd_table);
int            average(int *array, unsigned int cnt);
float          average_f(float *array, unsigned int cnt);
float          get_random_value(float min, float max);
float          convert_celsius(float c, char unit);
float          convert_uint(float c, char unit_src, char uint_target);

#endif /* _HQ_GENERIC_H_ */
