/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：tm1639.c
 * 文件描述：tm1639驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：按键使用K0、K1和KS1、KS4
 * 更新记录：
 *           V1.0 2024/01/24  乔鹤    初始版本
 *           V1.1 2024/11/15  张晓博  适配新框架
 */

#include "tm1639.h"
#include "display.h"
static void    tm1639_write_byte(uint8_t byte);
static uint8_t tm1639_read_byte(void);

const uint8_t numchar[] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9
};
const uint8_t specialchar[] = {
    0x00, // empty,0
    0x71, // F,1
    0x70, // K,2
    0x39, // C,3
    0x40, //-,4
    0x0F, // I,5
    0x79, // E,6
    0x31, // T,7
    0x77, // A,8
    0x38, // L,9
    0x76, // H,10
    0x37, // M,11
    0x3E, // U,12
    0x73, // P,13
    0x18, // l,14
    0x5C, // o,15
};

/*
 * 功能描述：tm1639初始化
 * 入参说明：无
 * 返 回 值：无
 */
void tm1639_init(void) {
    uint8_t wrbuf = 0x00;
    uint8_t i;

    tm1639_pin_init();

    tm1639_clk_out();
    tm1639_stb_out();
    tm1639_dio_out();

    tm1639_clk_high();
    tm1639_stb_high();
    tm1639_dio_high();

    /*tm1639所有段清零 */
    for (i = 0; i < 12; i++) {
        tm1639_write_buff(i, (uint8_t *)&wrbuf, 1);
    }

    /* 打开显示，显示最亮 */
    tm1639_set_command(1, 0);
}

/*
 * 功能描述：写显示数据
 * 入参说明：addr --- 写入的起始地址（最大地址为 0x0F，共 16 个字节）
 *           buff --- 数据缓冲区的指针
 *           len  --- 数据长度（字节）
 * 返 回 值：无
 */
void tm1639_write_buff(uint8_t addr, uint8_t *buff, uint8_t len) {
    uint8_t address_cmd;
    uint8_t i;

    if ((addr + len) > 0x0F) {
        return;
    }

    tm1639_stb_low();
    tm1639_write_byte(TM1639_CMD_DATA);
    tm1639_stb_high();
    tm1639_delay_us(1);

    address_cmd = TM1639_CMD_ADDR | addr;

    tm1639_stb_low();
    tm1639_write_byte(address_cmd);
    for (i = 0; i < len; i++) {
        tm1639_write_byte(*(buff++));
    }
    tm1639_stb_high();
    tm1639_delay_us(1);
}

/*
 * 功能描述：写显示控制
 * 入参说明：status--- 是否开启显示，0：关闭，1：显示
 *           luminance --- 亮度0-7
 * 返 回 值：无
 */
void tm1639_set_command(uint8_t status, uint8_t luminance) {
    uint8_t display_cmd;

    /* 限制亮度参数范围 */
    if (luminance > 7) {
        luminance = 7;
    }

    /* 构造命令：显示控制 + 亮度设置 */
    display_cmd = TM1639_CMD_DISPLAY | luminance;

    /* 根据状态添加显示开关标志 */
    if (status) {
        display_cmd |= TM1639_DISP_ON;
    }

    /* 发送命令 */
    tm1639_stb_low();
    tm1639_write_byte(display_cmd);
    tm1639_stb_high();
    tm1639_delay_us(1); /* 保持短暂延时确保信号稳定 */
}

/*
 * 功能描述：扫描并返回按键的值，从 TM1639 芯片读取按键状态。
 * 入参说明：无
 * 返 回 值：按键的 16 位值，包含所有扫描到的按键状态。
 */
uint16_t tm1639_key_scan(void) {
    uint16_t key_value;
    uint8_t *p = (uint8_t *)&key_value; /* 通过字节指针访问16位数据 */

    /* 启动通信：STB拉低 + 发送读键指令 */
    tm1639_stb_low();
    tm1639_write_byte(TM1639_CMD_DATA | TM1639_DATA_RD);

    /* 切换DIO为输入模式并读取双字节键值 */
    tm1639_dio_in();
    p[0] = tm1639_read_byte(); /* 读取低字节，BYTE1 */
    p[1] = tm1639_read_byte(); /* 读取高字节，BYTE2 */

    /* 结束通信：STB拉高 + 恢复DIO输出模式 */
    tm1639_stb_high();
    tm1639_dio_out();
    tm1639_delay_us(1); /* 保持短暂延时确保信号稳定 */

    return key_value;
}

/*
 * 功能描述：写一字节数据
 * 入参说明：byte --- 写数据
 * 返 回 值：无
 * 备    注：上升沿读取
 */
static void tm1639_write_byte(uint8_t byte) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        tm1639_clk_low();
        if (byte & 0x01) { /* 在下降沿刷新数据 */
            tm1639_dio_high();
        } else {
            tm1639_dio_low();
        }

        tm1639_delay_us(1); /* 最小保持400nS */
        tm1639_clk_high();
        byte >>= 1;
        tm1639_delay_us(1); /* 最小保持400nS */
    }
}

/*
 * 功能描述：读取一字节数据
 * 入参说明：无
 * 返 回 值：读取的字节数据
 */
static uint8_t tm1639_read_byte(void) {
    uint8_t i;
    uint8_t val = 0;

    for (i = 0; i < 8; i++) {
        tm1639_clk_low();
        val >>= 1;
        tm1639_delay_us(1); /* 最小保持400nS */
        tm1639_clk_high();
        if (tm1639_dio_get()) {
            val |= 0x80; /* 将读取的比特位放到最高位 */
        }
        tm1639_delay_us(1); /* 最小保持400nS */
    }
    return val;
}
