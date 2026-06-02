/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：oled.c
 * 文件描述：oled驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/11/15  乔鹤   初始版本
 *           V1.1 2025/03/25  李兆越 适配新框架
 */

#include "platform.h"
#include "soft_i2c.h"
#include "oled.h"
#include "oledfont.h"

uint8_t        OLED_GRAM[128][4]     = {0}; /* 缓冲 OLED 显示数据 */
static uint8_t OLED_GRAM_OLD[128][4] = {0}; /* 上一次显示缓存 */

/* 公司logo的128×32像素数据（放在代码段，上电即可访问） */
const static uint8_t company_logo[32][16] = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFC, 0xFC, 0x00, 0x80, 0xC0, 0xF0, 0xF8, 0x38, 0xB0, 0xC0},
                                             {0x80, 0x00, 0xFC, 0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFC, 0xFC},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFC, 0xFC, 0x00, 0x00, 0x00},
                                             {0xC0, 0xE0, 0xF8, 0x78, 0x38, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C},
                                             {0x00, 0x00, 0x00, 0xFC, 0xFC, 0xFC, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C},
                                             {0x3C, 0x78, 0xF8, 0xE0, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xF0, 0xF8, 0x38, 0x3C, 0x3C, 0x3C, 0x3C},
                                             {0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x38, 0x00, 0x00, 0x00, 0xFC, 0xFC, 0x00, 0x00, 0x00},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0xE3, 0xF9, 0xFE, 0x3F, 0xEF, 0xE7, 0xF9, 0x3E, 0xDF, 0xEF, 0xF3},
                                             {0xFD, 0x3E, 0x9F, 0xE7, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF},
                                             {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00},
                                             {0xFF, 0xFF, 0xDD, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C},
                                             {0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0xF8},
                                             {0xF8, 0xFE, 0x9F, 0x0F, 0x00, 0x00, 0x00, 0x3E, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x1E, 0x1E, 0x1E},
                                             {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x00, 0x01, 0x01, 0x06, 0x0F, 0x07, 0x03, 0x01},
                                             {0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00},
                                             {0x01, 0x07, 0x07, 0x0F, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E},
                                             {0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                             {0x03, 0x07, 0x1F, 0x1E, 0x18, 0x00, 0x00, 0x00, 0x03, 0x07, 0x0F, 0x0E, 0x1E, 0x1E, 0x1E, 0x1E},
                                             {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x00, 0x00, 0x00},
                                             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00}};

/*
 * 功能描述：设置OLED反显模式
 * 入参说明：i --- 0: 正常显示, 1: 反色显示
 * 返 回 值：无
 */
void oled_color_turn(uint8_t i) {
    if (i == 0) {
        oled_write(0xA6, OLED_CMD); /* 正常显示 */
    } else {
        oled_write(0xA7, OLED_CMD); /* 反色显示 */
    }
}

/*
 * 功能描述：设置OLED屏幕旋转
 * 入参说明：i --- 0: 正常显示, 1: 旋转180度
 * 返 回 值：无
 */
void oled_display_turn(uint8_t i) {
    if (i == 0) {
        oled_write(0xC8, OLED_CMD); /* 正常显示 */
        oled_write(0xA1, OLED_CMD);
    } else {
        oled_write(0xC0, OLED_CMD); /* 反转显示 */
        oled_write(0xA0, OLED_CMD);
    }
}

/*
 * 功能描述：通过I2C向OLED写入一个字节
 * 入参说明：dat --- 需要写入的数据
 *          mode --- 0: 命令, 1: 数据
 * 返 回 值：无
 */
int oled_write(uint8_t buff, uint8_t mode) {
    int     ret;
    uint8_t data[2] = {mode ? 0x40 : 0x00, buff};

    ret = si2c_send(OLED_I2C_BUS, OLED_I2C_ADDR, data, sizeof(data));
    return ret;
}

/*
 * 功能描述：开启OLED显示
 * 入参说明：无
 * 返 回 值：无
 */
void oled_display_on(void) {
    oled_write(0x8D, OLED_CMD); /* 使能电荷泵 */
    oled_write(0x14, OLED_CMD); /* 开启电荷泵 */
    oled_write(0xAF, OLED_CMD); /* 点亮屏幕 */
}

/*
 * 功能描述：关闭OLED显示
 * 入参说明：无
 * 返 回 值：无
 */
void oled_display_off(void) {
    oled_write(0x8D, OLED_CMD); /* 使能电荷泵 */
    oled_write(0x10, OLED_CMD); /* 关闭电荷泵 */
    oled_write(0xAE, OLED_CMD); /* 关闭屏幕 */
}

/*
 * 功能描述：设置OLED光标位置
 * 入参说明：Y 以左上角为原点，向下方向的坐标，范围：0~7
 *           X 以左上角为原点，向右方向的坐标，范围：0~127
 * 返 回 值：无
 */
void oled_set_cursor(uint8_t Y, uint8_t X) {
    oled_write(0xB0 | Y, OLED_CMD);                 /* 设置Y位置（页地址） */
    oled_write(0x10 | ((X & 0xF0) >> 4), OLED_CMD); /* 设置X高4位 */
    oled_write(0x00 | (X & 0x0F), OLED_CMD);        /* 设置X低4位 */
}

/*
 * 功能描述：更新OLED显示内容
 * 入参说明：无
 * 返 回 值：无
 */
void oled_refresh(void) {
    uint8_t i, n;
    uint8_t data[129]; /* 1 字节控制位 + 128 字节数据 */
    for (i = 0; i < 4; i++) {

        oled_set_cursor(i, 0);

        data[0] = 0x40; /* 数据模式 */
        for (n = 0; n < 128; n++) {
            data[n + 1] = OLED_GRAM[n][i]; /* 复制显示数据 */
        }
        si2c_send(OLED_I2C_BUS, OLED_I2C_ADDR, data, sizeof(data)); /* 批量发送数据 */
    }
}

/*
 * 功能描述：按位差分方式刷新OLED，仅更新变化的字节
 * 入参说明：无
 * 返 回 值：无
 */
// void oled_refresh_diff(void) {
//     uint8_t page, col;
//     uint8_t data[2];

//    for (page = 0; page < 4; page++) {
//        for (col = 0; col < 128; col++) {
//            uint8_t new_data = OLED_GRAM[col][page];
//            uint8_t old_data = OLED_GRAM_OLD[col][page];

//            if (new_data != old_data) {
//                OLED_GRAM_OLD[col][page] = new_data; /* 更新缓存 */

//                oled_set_cursor(page, col);          /* 设置坐标 */

//                data[0] = 0x40;       /* 写数据模式 */
//                data[1] = new_data;   /* 写具体数据 */
//                si2c_send(OLED_I2C_BUS, OLED_I2C_ADDR, data, 2);
//            }
//        }
//    }
//}

/*
 * 功能描述：仅更新有变化的OLED显示内容（差异刷新）
 * 入参说明：无
 * 返 回 值：无
 */
void oled_refresh_diff(void) {
    uint8_t i, n;
    uint8_t data[129];

    for (i = 0; i < 4; i++) {
        uint8_t need_update = 0;

        /* 检查当前页是否有变化 */
        for (n = 0; n < 128; n++) {
            if (OLED_GRAM[n][i] != OLED_GRAM_OLD[n][i]) {
                need_update = 1;
                break;
            }
        }

        if (!need_update) {
            continue; /* 当前页没有变化，跳过 */
        }

        /* 有变化，更新该页 */
        oled_set_cursor(i, 0);
        data[0] = 0x40;
        for (n = 0; n < 128; n++) {
            data[n + 1]         = OLED_GRAM[n][i];
            OLED_GRAM_OLD[n][i] = OLED_GRAM[n][i]; /* 更新缓存 */
        }
        si2c_send(OLED_I2C_BUS, OLED_I2C_ADDR, data, sizeof(data));
    }
}

/*
 * 功能描述：精细差分清屏函数，仅清除非0的字节
 * 入参说明：无
 * 返 回 值：无
 */
void oled_clear(void) {
    uint8_t page, col;

    for (page = 0; page < 4; page++) {
        for (col = 0; col < 128; col++) {
            if (OLED_GRAM[col][page] != 0) {
                OLED_GRAM[col][page] = 0; /* 清除数据 */
            }
        }
    }

    oled_refresh_diff(); /* 差异刷新 */
}

/*
 * 功能描述：画点
 * 入参说明：
 *   x - 横坐标 (0~127)
 *   y - 纵坐标 (0~63)
 *   t - 1填充，0清空
 * 返回值：无
 */
void oled_draw_point(uint8_t x, uint8_t y, uint8_t t) {
    uint8_t i, m, n;
    i = y / 8;
    m = y % 8;
    n = 1 << m;

    if (t) {
        OLED_GRAM[x][i] |= n;
    } else {
        OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
        OLED_GRAM[x][i] |= n;
        OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
    }
}

/*
 * 功能描述：画线
 * 入参说明：
 *   x1, y1 - 起点坐标
 *   x2, y2 - 终点坐标
 *   mode   - 1填充，0清除
 * 返回值：无
 */
void oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode) {
    uint16_t t;
    int      xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int      incx, incy, uRow, uCol;

    delta_x = x2 - x1; /* 计算坐标增量 */
    delta_y = y2 - y1;
    uRow    = x1; /* 画线起点坐标 */
    uCol    = y1;

    /* 设置单步方向 */
    incx = (delta_x > 0) ? 1 : ((delta_x == 0) ? 0 : -1); /* 垂直线 */
    incy = (delta_y > 0) ? 1 : ((delta_y == 0) ? 0 : -1); /* 水平线 */

    /* 选取基本增量坐标轴 */
    delta_x  = abs(delta_x);
    delta_y  = abs(delta_y);
    distance = (delta_x > delta_y) ? delta_x : delta_y;

    for (t = 0; t <= distance; t++) {
        oled_draw_point(uRow, uCol, mode); /* 画点 */
        xerr += delta_x;
        yerr += delta_y;

        if (xerr > distance) {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance) {
            yerr -= distance;
            uCol += incy;
        }
    }
}

/*
 * 功能描述：绘制一个圆
 * 入参说明：
 *   x - 圆心的 X 坐标
 *   y - 圆心的 Y 坐标
 *   r - 圆的半径
 * 返 回 值：无
 */
void oled_draw_circle(uint8_t x, uint8_t y, uint8_t r) {
    int a = 0, b = r, num;
    while (2 * b * b >= r * r) {
        oled_draw_point(x + a, y - b, 1);
        oled_draw_point(x - a, y - b, 1);
        oled_draw_point(x - a, y + b, 1);
        oled_draw_point(x + a, y + b, 1);

        oled_draw_point(x + b, y + a, 1);
        oled_draw_point(x + b, y - a, 1);
        oled_draw_point(x - b, y - a, 1);
        oled_draw_point(x - b, y + a, 1);

        a++;
        num = (a * a + b * b) - r * r; /* 计算当前点离圆心的距离 */
        if (num > 0) {
            b--;
            a--;
        }
    }
}

/*
 * 功能描述：在指定位置显示一个字符（支持不同字体大小）
 * 入参说明：
 *   x     - 起始 X 坐标 (0~127)
 *   y     - 起始 Y 坐标 (0~63)
 *   chr   - 要显示的字符
 *   size1 - 字体大小 (6x8 / 6x12 / 8x16 / 12x24)
 *   mode  - 显示模式 (0-反色, 1-正常)
 * 返 回 值：无
 */
void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size1, uint8_t mode) {
    uint8_t i, m, temp, size2, chr1;
    uint8_t x0 = x, y0 = y;

    if (size1 == 8)
        size2 = 6;
    else
        size2 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2); /* 计算字符点阵占用的字节数 */

    chr1 = chr - ' '; /* 计算偏移后的值 */

    for (i = 0; i < size2; i++) {
        if (size1 == 8)
            temp = asc2_0806[chr1][i]; /* 调用 0806 字体 */
        else if (size1 == 12)
            temp = asc2_1206[chr1][i]; /* 调用 1206 字体 */
        else if (size1 == 16)
            temp = asc2_1608[chr1][i]; /* 调用 1608 字体 */
        else if (size1 == 24)
            temp = asc2_2412[chr1][i]; /* 调用 2412 字体 */
        else
            return;

        for (m = 0; m < 8; m++) {
            if (temp & 0x01)
                oled_draw_point(x, y, mode);
            else
                oled_draw_point(x, y, !mode);

            temp >>= 1;
            y++;
        }
        x++;

        if ((size1 != 8) && ((x - x0) == size1 / 2)) {
            x = x0;
            y0 += 8;
        }
        y = y0;
    }
}

/*
 * 功能描述：显示字符串
 * 入参说明：
 *   x, y  - 起点坐标
 *   *chr  - 字符串指针
 *   size1 - 字体大小
 *   mode  - 0反色，1正常显示
 * 返回值：无
 */
void oled_show_string(uint8_t x, uint8_t y, uint8_t *chr, uint8_t size1, uint8_t mode) {
    while ((*chr >= ' ') && (*chr <= ' ' + 95)) /* 判断是不是非法字符! */
    {
        oled_show_char(x, y, *chr, size1, mode);
        if (size1 == 8)
            x += 6;
        else
            x += size1 / 2;
        chr++;
    }
}

/*
 * 功能描述：显示数字
 * 入参说明：
 *   x, y  - 起点坐标
 *   num   - 显示的数字
 *   len   - 显示的位数
 *   size1 - 字体大小
 *   mode  - 0反色，1正常显示
 * 返回值：无
 */
void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1, uint8_t mode) {
    uint8_t t, temp, m = 0;
    if (size1 == 8) m = 2;
    for (t = 0; t < len; t++) {
        temp = (num / oled_pow(10, len - t - 1)) % 10;
        if (temp == 0) {
            oled_show_char(x + (size1 / 2 + m) * t, y, '0', size1, mode);
        } else {
            oled_show_char(x + (size1 / 2 + m) * t, y, temp + '0', size1, mode);
        }
    }
}

/*
 * 功能描述：在指定位置显示汉字
 * 入参说明：
 *   x, y  - 起点坐标
 *   num   - 汉字对应的序号
 *   size1 - 字体大小
 *   mode  - 0: 反色显示, 1: 正常显示
 * 返 回 值：无
 */
void oled_show_chinese(uint8_t x, uint8_t y, uint8_t num, uint8_t size1, uint8_t mode) {
    uint8_t  m, temp;
    uint8_t  x0 = x, y0 = y;
    uint16_t i, size3   = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * size1; /* 计算字符点阵所占字节数 */

    for (i = 0; i < size3; i++) {
        /* 选择合适的字体集 */
        if (size1 == 16) {
            temp = Hzk1[num][i]; /* 调用16*16字体 */
        } else if (size1 == 24) {
            temp = Hzk2[num][i]; /* 调用24*24字体 */
        } else if (size1 == 32) {
            temp = Hzk3[num][i]; /* 调用32*32字体 */
        } else if (size1 == 64) {
            temp = Hzk4[num][i]; /* 调用64*64字体 */
        } else {
            return;
        }

        for (m = 0; m < 8; m++) {
            if (temp & 0x01) {
                oled_draw_point(x, y, mode);
            } else {
                oled_draw_point(x, y, !mode);
            }
            temp >>= 1;
            y++;
        }
        x++;

        /* 控制字符换行 */
        if ((x - x0) == size1) {
            x  = x0;
            y0 = y0 + 8;
        }
        y = y0;
    }
}

/*
 * 功能描述：滚动显示汉字
 * 入参说明：
 *   num   - 显示汉字的个数
 *   space - 每遍显示的间隔
 *   mode  - 0: 反色显示, 1: 正常显示
 * 返 回 值：无
 */
void oled_scroll_display(uint8_t num, uint8_t space, uint8_t mode) {
    uint8_t i, n, t = 0, m = 0, r;

    while (1) {
        if (m == 0) {
            oled_show_chinese(128, 8, t, 16, mode); /* 写入一个汉字到OLED_GRAM[][] */
            t++;
        }
        if (t == num) {
            for (r = 0; r < 16 * space; r++) /* 显示间隔 */
            {
                for (i = 1; i < 144; i++) {
                    for (n = 0; n < 4; n++) {
                        OLED_GRAM[i - 1][n] = OLED_GRAM[i][n];
                    }
                }
                oled_refresh();
            }
            t = 0;
        }
        m++;
        if (m == 16) {
            m = 0;
        }

        /* 实现左移 */
        for (i = 1; i < 144; i++) {
            for (n = 0; n < 4; n++) {
                OLED_GRAM[i - 1][n] = OLED_GRAM[i][n];
            }
        }
        oled_refresh();
    }
}

/*
 * 功能描述：显示图片
 * 入参说明：
 *   x, y     - 起点坐标
 *   sizex    - 图片宽度
 *   sizey    - 图片高度
 *   BMP[]    - 图片数据数组
 *   mode     - 0: 反色显示, 1: 正常显示
 * 返 回 值：无
 */
void oled_show_picture(uint8_t x, uint8_t y, uint8_t sizex, uint8_t sizey, uint8_t BMP[], uint8_t mode) {
    uint16_t j = 0;
    uint8_t  i, n, temp, m;
    uint8_t  x0 = x, y0 = y;
    sizey = sizey / 8 + ((sizey % 8) ? 1 : 0); /* 计算图片所需行数 */

    for (n = 0; n < sizey; n++) {
        for (i = 0; i < sizex; i++) {
            temp = BMP[j];
            j++;
            for (m = 0; m < 8; m++) {
                if (temp & 0x01) {
                    oled_draw_point(x, y, mode);
                } else {
                    oled_draw_point(x, y, !mode);
                }
                temp >>= 1;
                y++;
            }
            x++;

            /* 控制图片换行 */
            if ((x - x0) == sizex) {
                x  = x0;
                y0 = y0 + 8;
            }
            y = y0;
        }
    }
}

/*
 * 功能描述：OLED 初始化
 * 入参说明：无
 * 返回值：无
 */
void oled_init(void) {

    // gd32_delay_ms(100); /* 延时 100ms，等待 OLED 稳定 */

    si2c_init(OLED_I2C_BUS); /* 初始化 I2C 总线 */
    // oled_clear();            /* 清屏 */

    oled_write(0xAE, OLED_CMD); /* 关闭显示 */

    oled_write(0x00, OLED_CMD); /* 设置列地址低 4 位 */
    oled_write(0x10, OLED_CMD); /* 设置列地址高 4 位 */
    oled_write(0x00, OLED_CMD); /* 设置显示起始行 */
    oled_write(0xB0, OLED_CMD); /* 设置页地址 */

    oled_write(0x81, OLED_CMD); /* 设置对比度 */
    oled_write(0xFF, OLED_CMD); /* 对比度值 128 */

    oled_write(0xA1, OLED_CMD); /* 设置段重映射（左右反转） */
    oled_write(0xA6, OLED_CMD); /* 设置显示模式（正常显示） */

    oled_write(0xA8, OLED_CMD); /* 设置多路复用比率 */
    oled_write(0x1F, OLED_CMD); /* 1/32 扫描 */

    oled_write(0xC8, OLED_CMD); /* 设置 COM 扫描方向（从上到下） */

    oled_write(0xD3, OLED_CMD); /* 设置显示偏移量 */
    oled_write(0x00, OLED_CMD);

    oled_write(0xD5, OLED_CMD); /* 设置时钟分频因子 */
    oled_write(0x80, OLED_CMD);

    oled_write(0xD9, OLED_CMD); /* 设置预充电周期 */
    oled_write(0x1F, OLED_CMD);

    oled_write(0xDA, OLED_CMD); /* 设置 COM 引脚配置 */
    oled_write(0x00, OLED_CMD);

    oled_write(0xDB, OLED_CMD); /* 设置 VCOMH 电压 */
    oled_write(0x40, OLED_CMD);

    oled_write(0x8D, OLED_CMD); /* 使能充电泵 */
    oled_write(0x14, OLED_CMD);

    // oled_clear(); /* 再次清屏 */

    oled_show_picture(0, 0, 128, 32, (uint8_t *)company_logo, 1);
    oled_refresh();
    oled_write(0xAF, OLED_CMD); /* 打开显示 */
    gd32_delay_ms(100);
}

/*
 * 功能描述：OLED 显示
 * 入参说明：无
 * 返回值：无
 */
void oled_display(void) {
    oled_show_string(8, 0, (uint8_t *)"Pyrometers", 16, 1);
    oled_show_string(8, 16, (uint8_t *)"LED102 V3.0.2", 16, 1);
    oled_refresh();
}

/*
 * 功能描述：计算 m 的 n 次方
 * 入参说明：
 *   m - 底数
 *   n - 指数
 * 返 回 值：m^n 的计算结果
 */
uint32_t oled_pow(uint8_t m, uint8_t n) {
    uint32_t result = 1;
    while (n--) {
        result *= m;
    }
    return result;
}
