/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：hq_io.c
 * 文件描述：输入输出库
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/06/05 陈军  调试等级迁移到lib_cfg.h
 */

#include <stdarg.h>
#include "gd32_uart.h"
#include "lib_cfg.h"
#include "hq_io.h"
#include "hq_string.h"
#include "hq_generic.h"
#include <string.h>
#if DBG_IO_RTT
#include "SEGGER_RTT.h"
#endif
static char dbg_level[DBG_LEN_MAX]; /* 调试级别 */
static char io_buffer[LIB_IO_BUFF_SIZE];
uint8_t     put_port = 1;

/* 将C库的printf函数重定向到串口设备 */
// int fputc(int ch, FILE *f) {
//     usart_data_transmit(LIB_IO_DEV, (uint8_t)ch);
//     while (RESET == usart_flag_get(LIB_IO_DEV, USART_FLAG_TBE));
//     return ch;
// }

/* 将C库的printf函数重定向到RTT */
int fputc(int ch, FILE *f) {
#if DBG_IO_RTT
    if (put_port < DBG_PORT_DIFF) {
        SEGGER_RTT_PutChar(0, ch);
    } else {
        usart_data_transmit(LIB_IO_DEV, (uint8_t)ch);
        while (RESET == usart_flag_get(LIB_IO_DEV, USART_FLAG_TBE));
    }
#else
    usart_data_transmit(LIB_IO_DEV, (uint8_t)ch);
    while (RESET == usart_flag_get(LIB_IO_DEV, USART_FLAG_TBE));
#endif
    return ch;
}

#if 0
#pragma import(__use_no_semihosting)
struct __FILE {
    int handle;
};
FILE __stdout;
/* 定义_sys_exit()函数，避免使用半主机模式 */
void _sys_exit(int x)
{
    x = x;
}
/* 重定义fputc() */
int fputc(int ch, FILE *f)
{
    /* 循环传输 */
    while ((USART_STAT0(LIB_IO_DEV) & USART_STAT0_TBE) == 0);
    USART_DATA(LIB_IO_DEV) = ch;

    return ch;
}
void _ttywrch(int ch)
{
    ch = ch;
}
#endif

/*
 * 功能描述：初始化调试单元
 * 入参说明：level --- 调试级别，字符格式的调试等级字符串，例如 "1,2,3"
 *           rate --- 波特率
 *           fmt --- UART 格式字符串，例如 "8N1"
 * 返 回 值：无
 */
void dbg_init(const char *level, unsigned int rate, const char *fmt) {
    /* 确保传入的 level 字符串不会溢出 */
    string_copy_n(dbg_level, level, sizeof(dbg_level) - 1);
    dbg_level[sizeof(dbg_level) - 1] = '\0'; /* 确保结尾 \0 */
#if 0
    gd32_uart_init(LIB_IO_DEV, rate, fmt); /* 初始化 UART */
    io_pin_init();                         /* 初始化 IO 引脚 */
#else
    gd32_uart_init(LIB_IO_DEV, rate, fmt); /* 初始化 UART */
    io_pin_init();                         /* 初始化 IO 引脚 */
#endif
}

/*
 * 功能描述：此函数获取调试级别
 * 入参说明：无
 * 返 回 值：调试级别，DBG_LEVEL_xxx
 */
char *dbg_get_level(void) {
    return dbg_level;
}

/*
 * 功能描述：设置调试等级字符串
 * 入参说明：str --- 以逗号分隔的等级字符串，如 "1,2,4"
 */
void dbg_set_level(const char *str) {
    string_copy_n(dbg_level, str, sizeof(dbg_level) - 1);
    dbg_level[sizeof(dbg_level) - 1] = '\0'; /* 确保以 \0 结尾 */
}

/*
 * 功能描述：此函数在调试模式下将格式化字符串打印到控制台设备
 * 入参说明：level --- 输出级别，DBG_LEVEL_xxx
 *           fmt --- 格式字符串
 * 返 回 值：输出字符串长度
 */
/*
 * 功能描述：此函数在调试模式下将格式化字符串打印到控制台设备
 * 入参说明：level_str --- 输出级别（如 "1", "UPG" 等）
 *           fmt --- 格式字符串
 * 返 回 值：输出字符串长度
 */
int dbg_printf(const char *level_str, const char *fmt, ...) {
    if (0 == dbg_level_enabled(level_str)) {
        return 0;
    }

    va_list args;
    int     len;

    va_start(args, fmt);
    len = vsnprintf(io_buffer, sizeof(io_buffer), fmt, args);
    va_end(args);

    puts(io_buffer);
    return len;
}

/*
 * 功能描述：判断某个字符调试等级是否启用
 * 入参说明：level_str --- 调试等级字符
 * 返 回 值：1 启用，0 不启用
 */
int dbg_level_enabled(const char *level_str) {
#if 1
    char tmp = *level_str;
    // printf("%d %d tmp = %d \r\n",dbg_level[0],*level_str,tmp);
    if ((dbg_level[0] == tmp) || (dbg_level[0] == 0x37))
        return 1;
    else
        return 0;

#else
    /* 如果 dbg_level 包含 DBG_LEVEL_NONE，则表示不打印任何信息 */
    if (strstr(dbg_level, DBG_LEVEL_NONE)) {
        return 0; /* 不打印任何信息 */
    }

    /* 如果 dbg_level 包含 DBG_LEVEL_ALL，则表示启用所有级别 */
    if (strstr(dbg_level, DBG_LEVEL_ALL)) {
        return 1; /* 所有级别启用 */
    }
    /* 如果 dbg_level 包含指定的级别字符，表示启用该级别 */
    return simple_strchr(dbg_level, *level_str) != NULL;
#endif
}

/*
 * 功能描述：此函数在调试模式下将缓冲区以十六进制格式打印到控制台设备
 * 入参说明：level_char --- 输出级别，调试等级字符（如 '1', '2', '3'）
 *           title --- 标题
 *           buff --- 指向缓冲区的指针
 *           len --- 缓冲区长度
 *           line_len --- 行长度（每行的十六进制字节数）
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int dbg_print_buffer(const char *level_char, const char *title, const void *buff, int len, int line_len) {
    if (dbg_level_enabled(level_char)) {
        return print_buffer(title, buff, len, line_len);
    }
    return 0;
}

/*
 * 功能描述：此函数从控制台设备读取一个字符
 * 入参说明：无
 * 返 回 值：返回的字符
 */
int getch(void) {
    return gd32_uart_getch(LIB_IO_DEV);
}

/*
 * 功能描述：此函数从控制台设备读取一个字符（带超时）
 * 入参说明：ch --- 指向接收字符的指针
 *           timeo --- 超时值，以毫秒为单位
 * 返 回 值：0 --- 成功 其他 --- 超时
 */
int getch_timeout(int *ch, unsigned long timeo) {
    return gd32_uart_getch_timeout(LIB_IO_DEV, ch, timeo);
}

/*
 * 功能描述：此函数将一个字符打印到控制台设备
 * 入参说明：ch --- 要打印的字符
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int putch(int ch) {
#if DBG_IO_RTT
    if (put_port < DBG_PORT_DIFF) {
        SEGGER_RTT_PutChar(0, ch);
    } else {
        gd32_uart_putch(LIB_IO_DEV, ch);
    }
#else
    gd32_uart_putch(LIB_IO_DEV, ch);
#endif
    return 0;
}

/*
 * 功能描述：此函数将一个以'\0'结尾的字符串打印到控制台设备
 * 入参说明：str --- 要打印的字符串
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int puts(const char *str) {
    while (*str) {
        putch(*str++);
    }

    return 0;
}

/*
 * 功能描述：此函数将一个十进制数打印到控制台设备
 * 入参说明：val --- 要打印的十进制数
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int putd(unsigned long val) {
    unsigned int prod, t;
    int          flag = 0;
    char         ch;
    int          i, j;

    if (val == 0) {
        putch('0');
        return 0;
    }

    for (i = 15; i >= 0; i--) {
        prod = 1;
        t    = val;
        for (j = 0; j < i; j++) {
            prod = prod * 10;
            t    = t / 10;
        }
        ch = (char)t;
        val -= prod * t;

        if (ch == 0 && flag == 0) {
            continue;
        }

        if (ch < 10) {
            putch(ch + '0');
        } else {
            putch('?');
        }

        flag = 1;
    }

    return 0;
}

/*
 * 功能描述：此函数将一个十六进制数打印到控制台设备
 * 入参说明：val --- 要打印的十六进制数
 *           digits --- 要打印的十六进制数的位数
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int putx(unsigned long val, int digits) {
    char ch;
    int  i;

    for (i = digits - 1; i >= 0; i--) {
        ch = (char)((val >> (i * 4)) & 0x0f);

        if (ch < 10) {
            putch(ch + '0');
        } else {
            putch(ch - 10 + 'A');
        }
    }

    return 0;
}

/*
 * 功能描述：此函数将一个浮点数打印到控制台设备
 * 入参说明：val --- 要打印的浮点数
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int putf(float val) {
    int int_part = (int)val;

    /* 整数部分 */
    putd(int_part);

    /* 小数部分 */
    val = val - (float)int_part;

    if (val > 0.0f) {
        putch('.');

        while (val > 0.0f) {
            val      = val * 10.0f;
            int_part = (int)val;
            putch(int_part + '0');
            val = val - (float)int_part;
        }

    } /* 值大于0.0f */

    return 0;
}

/*
 * 功能描述：此函数以十六进制格式打印缓冲区
 * 入参说明：title --- 标题
 *           buff --- 指向缓冲区的指针
 *           len --- 缓冲区长度
 *           line_len --- 行长度（每行的十六进制字节数）
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int print_buffer(const char *title, const void *buff, int len, int line_len) {
    const unsigned char *data = (const unsigned char *)buff;
    int                  size;

    puts(title);

    while (len != 0) {
        size = (len < line_len) ? len : line_len;

        while (size-- != 0) {
            putx(*data, 2);
            putch(' ');
            data++;
            len--;
        }

        puts("\r\n");
    }

    return 0;
}
