/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ds18b20.c
 * 文件描述：DS18B20温度传感器驱动
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#include "hq_generic.h"
#include "hq_io.h"
#include "ds18b20.h"

static int     init_seq(void);        /* 初始化序列 */
static void    tx_bit(int data);      /* 发送位 */
static int     rx_bit(void);          /* 接收位 */
static void    tx_byte(uint8_t data); /* 发送字节 */
static uint8_t rx_byte(void);         /* 接收字节 */
static uint8_t rx_bit2(void);         /* 接收2位 */

/* 总线表 */
static uint32_t WIRE_PORTS[] = {
    DS18B20_WIRE1_PORT,
};
static uint32_t WIRE_PIN[] = {
    DS18B20_WIRE1_PIN,
};

/* 用于温度计算的查找表 */
static const int LOOKUP_TABLE[] = {
    0,    /*  0/16 = 0       */
    625,  /*  1/16 = 0.0625  */
    1250, /*  2/16 = 0.125   */
    1875, /*  3/16 = 0.1875  */
    2500, /*  4/16 = 0.25    */
    3125, /*  5/16 = 0.3125  */
    3750, /*  6/16 = 0.375   */
    4375, /*  7/16 = 0.4375  */
    5000, /*  8/16 = 0.5     */
    5625, /*  9/16 = 0.5625  */
    6250, /*  10/16 = 0.625  */
    6875, /*  11/16 = 0.6875 */
    7500, /*  12/16 = 0.75   */
    8125, /*  13/16 = 0.8125 */
    8750, /*  14/16 = 0.875  */
    9375, /*  15/16 = 0.9375 */
};

/* 传感器总线数量 */
static int wire_num;

#define set_wire_in() gpio_mode_set(WIRE_PORTS[wire_num], GPIO_MODE_INPUT, GPIO_PUPD_NONE, WIRE_PIN[wire_num])
#define set_wire_out()                                                                                                                               \
    {                                                                                                                                                \
        gpio_mode_set(WIRE_PORTS[wire_num], GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, WIRE_PIN[wire_num]);                                                   \
        gpio_output_options_set(WIRE_PORTS[wire_num], GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, WIRE_PIN[wire_num]);                                         \
    }
#define pull_wire_high() gpio_bit_set(WIRE_PORTS[wire_num], WIRE_PIN[wire_num])
#define pull_wire_low()  gpio_bit_reset(WIRE_PORTS[wire_num], WIRE_PIN[wire_num])
#define wire_high()      gpio_input_bit_get(WIRE_PORTS[wire_num], WIRE_PIN[wire_num])
#define wire_low()       (!wire_high())

/*
 * 功能描述：该函数初始化DS18B20
 * 入参说明：无
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
int ds18b20_init(void) {
    wire_num = 0;

    ds18b20_pin_init();
    ds18b20_reset();
    return 0;
}

/*
 * 功能描述：该函数重置DS18B20
 * 入参说明：无
 * 返 回 值：无
 */
void ds18b20_reset(void) {
    set_wire_out();

    pull_wire_low();
    wire_delay_us(480);
    pull_wire_high();

    wire_delay_us(15);
}

/*
 * 功能描述：该函数选择一个传感器线路
 * 入参说明：num --- 线路编号，从0到DS18B20_WIRE_CNT - 1
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
void ds18b20_select(int num) {
    wire_num = num;
}

/*
 * 功能描述：该函数搜索当前线路上的所有传感器。
 * 入参说明：roms --- 指向传感器ROM代码数组的指针。
 * 返 回 值：搜索到的传感器数量。
 * 备注说明：使用的搜索算法是二叉树搜索算法。
 */
int ds18b20_search(uint8_t (*roms)[8]) {
    uint8_t stack[DS18B20_CNT_PER_WIRE];
    uint8_t code[DS18B20_ROM_SIZE * 8];
    uint8_t count;
    uint8_t val;
    uint8_t pos;
    uint8_t resp;
    uint8_t conflict;
    uint8_t m;
    uint8_t n;

    count = 0;
    pos   = 0;
    mem_set(stack, sizeof(stack), 0);
    do {
        if (init_seq() != 0) {
            break;
        }

        ds18b20_reset();
        wire_delay_us(720);

        tx_byte(DS18B20_CMD_SEARCH_ROM);
        // stm32_delay_us(400);

        for (m = 0; m < 8; m++) {
            val = 0;
            for (n = 0; n < 8; n++) {
                resp = rx_bit2(); /* 读取两位数据 */
                // stm32_delay_us(10);
                resp = resp & 0x03;
                val >>= 1;

                if (resp == 0x01) { /* 如果响应为b'01，读取的数据为0，写入0，具有此位为0的设备将响应 */
                    tx_bit(0);
                    code[m * 8 + n] = 0;
                } else if (resp == 0x02) { /* 如果响应为b'10，读取的数据为1，写入1，具有此位为1的设备将响应 */
                    val |= 0x80;
                    tx_bit(1);
                    code[m * 8 + n] = 1;
                } else if (resp == 0x00) { /* 如果响应为b'00，表示存在冲突位 */
                    /* 如果冲突位大于栈顶，写入0；如果小于，写入之前的数据；如果相等，写入1 */
                    conflict = m * 8 + n + 1;
                    if (conflict > stack[pos]) {
                        tx_bit(0);
                        code[m * 8 + n] = 0;
                        stack[++pos]    = conflict;
                        /* ？？？？冲突return */
                        return count; /* ？？？？冲突return */
                    } else if (conflict < stack[pos]) {
                        // val |= code[m * 8 + n] << 7;
                        val = val | ((code[(m * 8 + n)] & 0x01) << 7);
                        tx_bit(code[m * 8 + n]);
                    } else if (conflict == stack[pos]) {
                        val |= 0x80;
                        tx_bit(1);
                        code[m * 8 + n] = 1;
                        pos--;
                    }
                } else { /* 如果响应为b'11，表示无响应 */
                    return count;
                }
            }
            roms[count][m] = val;
        }
        count++;

        if (count == DS18B20_CNT_PER_WIRE) {
            break;
        }
    } while (stack[pos] != 0);

    return count;
}

/*
 * 功能描述：该函数设置DS18B20的报警阈值和分辨率
 * 入参说明：rom --- 指向传感器ROM代码的指针，0表示SKIP_ROM
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
int ds18b20_config(uint8_t *rom) {
    int     th;
    int     tl;
    int     res;
    uint8_t reg_th;
    uint8_t reg_tl;
    uint8_t reg_cfg;
    int     neg;
    int     i;

    th  = DS18B20_ALM_HI;
    tl  = DS18B20_ALM_LO;
    res = DS18B20_RES_BIT;

    /* 处理TH */
    neg = 0;
    if (th < 0) {
        th  = -th;
        neg = 1;
    }

    reg_th = th & 0x7f; /* 位[6:0] */
    if (neg) {
        reg_th |= (1 << 7);
    }

    /* 处理TL */
    neg = 0;
    if (tl < 0) {
        tl  = -tl;
        neg = 1;
    }

    reg_tl = tl & 0x7f; /* 位[6:0] */
    if (neg) {
        reg_tl |= (1 << 7);
    }

    /* 处理RES */
    switch (res) {
        case 9: reg_cfg = DS18B20_RES_BIT9 | 0x1f; break;
        case 10: reg_cfg = DS18B20_RES_BIT10 | 0x1f; break;
        case 11: reg_cfg = DS18B20_RES_BIT11 | 0x1f; break;
        case 12: reg_cfg = DS18B20_RES_BIT12 | 0x1f; break;
        default: reg_cfg = DS18B20_RES_BIT12 | 0x1f; break;
    }

    /* 初始化序列 */
    if (init_seq() != 0) {
        return -1;
    }

    /* 地址 */
    if (rom == 0) { /* 所有传感器 */
        tx_byte(DS18B20_CMD_SKIP_ROM);
    } else { /* 一个传感器 */
        tx_byte(DS18B20_CMD_MATCH_ROM);
        for (i = DS18B20_ROM_SIZE; i != 0; i--) {
            tx_byte(*rom++);
        }
    }

    /* 写入RAM */
    tx_byte(DS18B20_CMD_WRITE_RAM);
    tx_byte(reg_th);  /* 写入TH */
    tx_byte(reg_tl);  /* 写入TL */
    tx_byte(reg_cfg); /* 写入CFG */

    /* 复制到EEPROM */
    if (init_seq() != 0) {
        return -1;
    }

    /* 地址 */
    if (rom == 0) { /* 所有传感器 */
        tx_byte(DS18B20_CMD_SKIP_ROM);
    } else { /* 一个传感器 */
        tx_byte(DS18B20_CMD_MATCH_ROM);
        for (i = DS18B20_ROM_SIZE; i != 0; i--) {
            tx_byte(*rom++);
        }
    }

    tx_byte(DS18B20_CMD_COPY_RAM);

    /* 为时长15ms启用强上拉（适用于寄生电源） */
    set_wire_out();   /* 输出模式 */
    pull_wire_high(); /* 拉高 */
    wire_delay_us(15);
    set_wire_in(); /* 释放总线 */

    return 0;
}

/*
 * 功能描述：该函数启动一根线路上所有传感器的转换过程
 * 入参说明：rom --- 指向传感器ROM代码的指针，0表示SKIP_ROM
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
int ds18b20_start(uint8_t *rom) {
    int i;

    if (init_seq() != 0) {
        return -1; /* 初始化序列 */
    }

    if (rom == 0) {
        tx_byte(DS18B20_CMD_SKIP_ROM);
    } else {
        tx_byte(DS18B20_CMD_MATCH_ROM);
        for (i = DS18B20_ROM_SIZE; i != 0; i--) {
            tx_byte(*rom++);
        }
    }

    tx_byte(DS18B20_CMD_CONVERT); /* 命令 */

    return 0;
}

/*
 * 功能描述：该函数从DS18B20读取结果
 * 入参说明：rom --- 指向传感器ROM代码的指针，0表示SKIP_ROM
 *           val --- 指向值的指针（温度 * 10^DS18B20_RESULT_DP）
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
int ds18b20_read(uint8_t *rom, int *val) {
    uint8_t data[2];
    int     result;
    int     neg;
    int     i;

    if (init_seq() != 0) {
        return -1; /* 初始化序列 */
    }

    /* 地址 */
    if (rom == 0) { /* 所有传感器 */
        tx_byte(DS18B20_CMD_SKIP_ROM);
    } else { /* 一个传感器 */
        tx_byte(DS18B20_CMD_MATCH_ROM);
        for (i = DS18B20_ROM_SIZE; i != 0; i--) {
            tx_byte(*rom++);
        }
    }

    /* 读RAM（仅2字节） */
    tx_byte(DS18B20_CMD_READ_RAM);
    data[0] = rx_byte();
    data[1] = rx_byte();

    neg = 0;
    if (data[1] > 7) { /* 负数 */
        data[0] = ~data[0] + 1;
        data[1] = (data[0] == 0) ? (~data[1] + 1) : (~data[1]);
        neg     = 1;
    }

#if DS18B20_RESULT_DP == 0 /* 整数 */

    result = (((data[1] & 0x07) << 4) | (data[0] >> 4));

#elif DS18B20_RESULT_DP == 1 /* 小数点后1位数字 */

    result = (((data[1] & 0x07) << 4) | (data[0] >> 4)) * 10 + (LOOKUP_TABLE[data[0] & 0x0f] + 500) / 1000;

#elif DS18B20_RESULT_DP == 2 /* 小数点后2位数字 */

    result = (((data[1] & 0x07) << 4) | (data[0] >> 4)) * 100 + (LOOKUP_TABLE[data[0] & 0x0f] + 50) / 100;

#elif DS18B20_RESULT_DP == 3 /* 小数点后3位数字 */

    result = (((data[1] & 0x07) << 4) | (data[0] >> 4)) * 1000 + (LOOKUP_TABLE[data[0] & 0x0f] + 5) / 10;

#elif DS18B20_RESULT_DP == 4 /* 小数点后4位数字 */

    result = (((data[1] & 0x07) << 4) | (data[0] >> 4)) * 10000 + LOOKUP_TABLE[data[0] & 0x0f];

#endif /* ds18b20_RESULT_DP */

    /* 处理符号 */
    *val = (neg) ? (-result) : (result);

    return 0;
}

/*
 * 功能描述：该函数在1-wire总线上生成一个初始化序列
 * 入参说明：无
 * 返 回 值：0 = 成功（有设备响应）  其他 = 失败（无设备响应）
 */
static int init_seq(void) {
    int resp;

    set_wire_out(); /* 输出模式 */

    pull_wire_low();                  /* 复位时序 */
    wire_delay_us(DS18B20_RST_WIDTH); /* >480us */

    set_wire_in();     /* 释放总线 */
    wire_delay_us(30); /* 15-60us  DS18B20_RST_WAIT */

    /* 采集响应 */
    resp = wire_low();
    wire_delay_us(200); /* 60-240us DS18B20_RST_WIDTH - DS18B20_RST_WAIT */

    /* 如果没有设备响应则返回-1 */
    return (resp) ? 0 : -1;
}

/*
 * 功能描述：该函数向1-wire总线传输一个位
 * 入参说明：data --- data[0]是要传输的位
 * 返 回 值：无
 */
static void tx_bit(int data) {
    set_wire_out(); /* 输出模式 */

    pull_wire_low(); /* 时隙开始 */
    wire_delay_us(DS18B20_SLOT_START);

    if (data & 0x01) {
        pull_wire_high();
    }
    wire_delay_us(60); /* >60us即可 DS18B20_SLOT_WIDTH - DS18B20_SLOT_START */

    set_wire_in(); /* 释放总线 */
}

/*
 * 功能描述：该函数从单总线接收一个位
 * 入参说明：无
 * 返 回 值：接收到的数据（data[0]是接收到的位）
 */
static int rx_bit(void) {
    int res;

    set_wire_out(); /* 输出模式 */

    pull_wire_low(); /* 时隙开始 */
    wire_delay_us(DS18B20_SLOT_START);

    set_wire_in(); /* 释放总线 */

    /* 采样 */
    wire_delay_us(DS18B20_READ_WAIT);
    res = (wire_high()) ? 1 : 0;

    wire_delay_us(60); /* >60us即可 DS18B20_SLOT_WIDTH - DS18B20_SLOT_START - DS18B20_READ_WAIT */

    return res;
}

/*
 * 功能描述：该函数向1-wire总线传输一个字节
 * 入参说明：data --- 要传输的数据字节
 * 返 回 值：无
 */
static void tx_byte(uint8_t data) {
    int i;

    for (i = 0; i < 8; i++) { /* 发送8-bit，LSB */
        tx_bit(data >> i);
        wire_delay_us(2); /* 写恢复时间(>1us即可  DS18B20_RECOVERY) */
    }
}

/*
 * 功能描述：该函数从1-wire总线接收一个字节
 * 入参说明：无
 * 返 回 值：接收到的数据字节
 */
static uint8_t rx_byte(void) {
    int byte = 0;
    int i;

    for (i = 8; i > 0; i--) { /* 接收8-bit，LSB */
        byte = byte >> 1;
        if (rx_bit()) {
            byte |= (1 << 7);
        }
        wire_delay_us(2); /* 读恢复时间(>1us即可  DS18B20_RECOVERY) */
    }

    return byte;
}

/*
 * 功能描述：该函数从总线接收2位
 * 入参说明：无
 * 返 回 值：接收到的数据字节
 * 备注说明：由ds18b20_search()调用
 */
static uint8_t rx_bit2(void) {
    int byte;

    byte = 0;
    if (rx_bit()) {
        byte |= 0x01;
    }

    wire_delay_us(DS18B20_RECOVERY);

    byte = byte << 1;
    if (rx_bit()) {
        byte |= 0x01;
    }

    return byte;
}
