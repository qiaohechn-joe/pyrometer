/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：soft_i2c.c
 * 文件描述：软件模拟I2C驱动
 * 作    者：和其光电嵌软团队
 * 备    注：从设备地址左移一位处理
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/06/05 陈军  移除引脚配置到dev_cfg文件
 */

#include "soft_i2c.h"

#define SI2C_ACK_TIMEOUT 100u /* 读取ACK超时（微秒） */

/* I2C总线 */
struct si2c_bus_t {
    rcu_periph_enum sda_pid;  /* RCU_GPIOx */
    uint32_t        sda_port; /* GPIOx */
    uint32_t        sda_pin;  /* GPIO_PIN_xx */
    rcu_periph_enum scl_pid;  /* RCU_GPIOx */
    uint32_t        scl_port; /* GPIOx */
    uint32_t        scl_pin;  /* GPIO_PIN_xx */
    uint32_t        delay;    /* 单位微秒，延时=周期/4 */
};

static const struct si2c_bus_t BUS_TABLE[] = {
    /* EEPROM : SDA=PC9  SCL=PA8  period=12us */
    {.sda_pid = RCU_GPIOC, .sda_port = GPIOC, .sda_pin = GPIO_PIN_9, .scl_pid = RCU_GPIOA, .scl_port = GPIOA, .scl_pin = GPIO_PIN_8, .delay = 12},

    /* OV5640: SCL=PB8, SDA=PB9 */
    {.sda_pid = RCU_GPIOB, .sda_port = GPIOB, .sda_pin = GPIO_PIN_9, .scl_pid = RCU_GPIOB, .scl_port = GPIOB, .scl_pin = GPIO_PIN_8, .delay = 4},

    /* OLED : SCL=PB10, SDA=PB3 */
    {.sda_pid = RCU_GPIOB, .sda_port = GPIOB, .sda_pin = GPIO_PIN_3, .scl_pid = RCU_GPIOB, .scl_port = GPIOB, .scl_pin = GPIO_PIN_10, .delay = 1},

    /* LTC2606 : SCL=PB4, SDA=PB5 */
    {.sda_pid = RCU_GPIOB, .sda_port = GPIOB, .sda_pin = GPIO_PIN_5, .scl_pid = RCU_GPIOB, .scl_port = GPIOB, .scl_pin = GPIO_PIN_4, .delay = 4},

};

#define BUS_TABLE_SIZE sizeof(BUS_TABLE) / sizeof(BUS_TABLE[0])

/* n: 总线号, 0-N */
#define pin_init(n)                                                                                                                                  \
    {                                                                                                                                                \
        rcu_periph_clock_enable(BUS_TABLE[n].sda_pid);                                                                                               \
        rcu_periph_clock_enable(BUS_TABLE[n].scl_pid);                                                                                               \
    }

/* 数据脚 */
#define set_sda_out(n)                                                                                                                               \
    {                                                                                                                                                \
        gpio_mode_set(BUS_TABLE[n].sda_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BUS_TABLE[n].sda_pin);                                                \
        gpio_output_options_set(BUS_TABLE[n].sda_port, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, BUS_TABLE[n].sda_pin);                                      \
    }
#define set_sda_in(n)    gpio_mode_set(BUS_TABLE[n].sda_port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, BUS_TABLE[n].sda_pin)
#define pull_sda_high(n) gpio_bit_set(BUS_TABLE[n].sda_port, BUS_TABLE[n].sda_pin)
#define pull_sda_low(n)  gpio_bit_reset(BUS_TABLE[n].sda_port, BUS_TABLE[n].sda_pin)
#define sda_high(n)      gpio_input_bit_get(BUS_TABLE[n].sda_port, BUS_TABLE[n].sda_pin)
#define sda_low(n)       (!sda_high())

/* 时钟脚 */
#define set_scl_out(n)                                                                                                                               \
    {                                                                                                                                                \
        gpio_mode_set(BUS_TABLE[n].scl_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BUS_TABLE[n].scl_pin);                                                \
        gpio_output_options_set(BUS_TABLE[n].scl_port, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, BUS_TABLE[n].scl_pin);                                      \
    }
#define set_scl_in(n)    gpio_mode_set(BUS_TABLE[n].scl_port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, BUS_TABLE[n].scl_pin)
#define pull_scl_high(n) gpio_bit_set(BUS_TABLE[n].scl_port, BUS_TABLE[n].scl_pin)
#define pull_scl_low(n)  gpio_bit_reset(BUS_TABLE[n].scl_port, BUS_TABLE[n].scl_pin)
#define scl_high(n)      gpio_input_bit_get(BUS_TABLE[n].scl_port, BUS_TABLE[n].scl_pin)
#define scl_low(n)       (!scl_high())

/* 延时 */
#define phase_delay(n)   gd32_delay_us(BUS_TABLE[n].delay / 4)

/* 内部函数 */
static void tx_bit(int bus, int bit);
static int  rx_bit(int bus);
static int  rx_ack(int bus, uint32_t timeout);

/*
 * 功能描述：该函数初始化软件I2C，准备控制I2C总线
 * 入参说明：bus --- 总线编号，从0开始
 * 返 回 值：无
 */
void si2c_init(int bus) {
    /* 引脚配置 */
    pin_init(bus);
    set_sda_out(bus);
    set_scl_out(bus);

    /* 设置SDA和SCL为高电平 */
    pull_sda_high(bus);
    pull_scl_high(bus);
}

/*
 * 功能描述：此函数向指定的I2C从设备发送若干字节
 * 入参说明：bus --- 总线编号，从0开始
 *           addr----从设备地址（7位）
 *           buff----指向数据缓冲区的指针
 *           len----数据长度（字节）
 * 返 回 值：成功传输的字节数
 */
int si2c_send(int bus, uint32_t addr, const void *buff, int len) {
    int ret;

    si2c_start(bus); /* 发送开始 */

    /* 发送从设备地址以进行写操作 */
    if (si2c_select(bus, addr, SI2C_OP_WRITE) != 0) {
        si2c_stop(bus);
        return 0;
    }

    ret = si2c_tx(bus, buff, len); /* 发送字节 */
    si2c_stop(bus);                /* 发送停止 */

    return ret; /* 实际发送的数据字节数 */
}

/*
 * 功能描述：此函数从指定的I2C从设备接收若干字节数据
 * 入参说明：bus --- 总线编号，从0开始
 *           addr----从设备地址
 *           buff----指向数据缓冲区的指针
 *           len----数据长度（字节）
 * 返 回 值：成功接收的字节数
 */
int si2c_receive(int bus, uint32_t addr, void *buff, int len) {
    int ret;

    si2c_start(bus); /* 接收开始 */

    /* 接收从设备地址以进行读操作 */
    if (si2c_select(bus, addr, SI2C_OP_READ) != 0) {
        si2c_stop(bus);
        return 0;
    }

    ret = si2c_rx(bus, buff, len); /* 接收字节 */
    si2c_stop(bus);                /* 接收停止 */

    return ret; /* 实际接收的数据字节数 */
}

/*
 * 功能描述：此函数在I2C总线上发起一个START条件
 *           当SCL为高电平时，SDA线上的高到低跳变定义了一个START条件
 * 入参说明：bus --- 总线编号，从0开始
 * 返 回 值：无
 */
void si2c_start(int bus) {
    set_sda_out(bus);

    pull_scl_high(bus);
    pull_sda_high(bus);

    phase_delay(bus);

    pull_sda_low(bus);
    phase_delay(bus);

    pull_scl_low(bus);
    phase_delay(bus);
}

/*
 * 功能描述：此函数在I2C总线上发起一个STOP条件
 *           当SCL为高电平时，SDA线上的低到高跳变定义了一个STOP条件
 * 入参说明：bus --- 总线编号，从0开始
 * 返 回 值：无
 */
void si2c_stop(int bus) {
    set_sda_out(bus);

    pull_scl_low(bus);
    pull_sda_low(bus);
    phase_delay(bus);

    pull_scl_high(bus);
    phase_delay(bus);

    pull_sda_high(bus);
    phase_delay(bus);
}

/*
 * 功能描述：该函数向I2C总线传输一个从设备地址
 * 入参说明：bus --- 总线编号，从0开始
 *           addr --- 从设备地址，7位
 *           op --- 操作类型，SI2C_OP_WRITE 或 SI2C_OP_READ
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
int si2c_select(int bus, uint32_t addr, int op) {
    uint32_t byte;
    int      i;

    /* 发送地址 */
    byte = ((addr & 0x7f) << 1) | (op & 0x01);
    set_sda_out(bus);
    for (i = 7; i >= 0; i--) {
        tx_bit(bus, byte >> i); /* 发送8-bit，MSB */
    }

    /* 接收ACK */
    set_sda_in(bus);
    if (rx_ack(bus, SI2C_ACK_TIMEOUT) != 0) {
        return -1;
    }

    return 0; /* 成功 */
}

/*
 * 功能描述：该函数向I2C总线传输数据字节
 * 入参说明：bus --- 总线编号，从0开始
 *           buff----数据缓冲区的指针
 *           len----数据长度（字节）
 * 返 回 值：成功传输的字节数
 */
int si2c_tx(int32_t bus, const void *buff, int len) {
    const uint8_t *data  = (const uint8_t *)buff;
    const uint8_t *start = data;
    uint8_t        byte;
    int            i;

    while (len-- > 0) {
        /* 发送字节 */
        byte = *data;
        set_sda_out(bus);
        for (i = 7; i >= 0; i--) {
            tx_bit(bus, byte >> i); /* 发送8-bit，MSB */
        }

        /* 接收ACK */
        set_sda_in(bus);
        if (rx_ack(bus, SI2C_ACK_TIMEOUT) != 0) {
            break; /* 失败放弃 */
        }

        data++;
    }

    return (data - start); /* 实际发送的字节数 */
}

/*
 * 功能描述：该函数从I2C总线接收数据字节
 * 入参说明：bus---总线编号，从0开始
 *           buff----数据缓冲区的指针
 *           len----数据长度（字节）
 * 返 回 值：成功接收的字节数
 */
int si2c_rx(int bus, void *buff, int len) {
    uint8_t *data  = (uint8_t *)buff;
    uint8_t *start = data;

    int byte;
    int i;

    while (len != 0) {
        /* 接收字节 */
        set_sda_in(bus);
        byte = 0;

        for (i = 8; i != 0; i--) {
            /* 读一个BIT */
            byte = byte << 1;
            byte |= (rx_bit(bus) & 0x01);
        }

        /* 保存字节 */
        *data++ = (uint8_t)byte;
        len--;

        /* 发送ACK/NAK */
        set_sda_out(bus);
        if (len == 0) {
            tx_bit(bus, 1); /* NAK */
        } else {
            tx_bit(bus, 0); /* ACK */
        }
    }

    return (data - start); /* 实际接收的字节 */
}

/*
 * 功能描述：此函数向I2C总线发送一个位
 *           调用此函数时，SDA必须处于输出模式
 * 入参说明：bus --- 总线编号，从0开始
 *           bit----位值，0或1
 * 返 回 值：无
 */
static void tx_bit(int bus, int bit) {
    if (bit & 0x01) {
        pull_sda_high(bus);
    } else {
        pull_sda_low(bus);
    }

    phase_delay(bus);

    pull_scl_high(bus);
    phase_delay(bus);
    phase_delay(bus);

    pull_scl_low(bus);
    phase_delay(bus);
}

/*
 * 功能描述：此函数从I2C总线接收一个位
 *           调用此函数时，SDA必须处于输入模式
 * 入参说明：bus --- 总线编号，从0开始
 * 返 回 值：接收到的数据位，0或1
 */
static int rx_bit(int bus) {
    int temp;

    phase_delay(bus);

    pull_scl_high(bus);
    phase_delay(bus);

    temp = (sda_high(bus)) ? 1 : 0;

    phase_delay(bus);

    pull_scl_low(bus);
    phase_delay(bus);

    return temp;
}

/*
 * 功能描述：此函数从I2C总线接收一个ACK位
 *           调用此函数时，SDA必须处于输入模式
 * 入参说明：bus --- 总线编号，从0开始
 *           timeout --- 等待的超时时间（单位为微秒）
 * 返 回 值：0 --- 成功  其他 --- ACK超时
 */
static int rx_ack(int bus, uint32_t timeout) {
    phase_delay(bus);

    pull_scl_high(bus);
    phase_delay(bus);

    while (timeout-- != 0) {
        if (!sda_high(bus)) {
            break;
        }
        gd32_delay_us(1);
    }

    phase_delay(bus);

    pull_scl_low(bus);
    phase_delay(bus);

    return (timeout == 0) ? -1 : 0;
}
