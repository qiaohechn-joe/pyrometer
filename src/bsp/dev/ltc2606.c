/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ltc2606.c
 * 文件描述：LTC2606（16Bits I2C接口DAC）驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军 初始版本
 */

#include "hq_generic.h"
#include "soft_i2c.h"
#include "ltc2606.h"

/* 内部函数 */
static int ltc2606_write(int bus, uint32_t dev_addr, void *buff, int len);

/*
 * 功能描述：此函数初始化LTC2606。
 * 入参说明：无。
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
int ltc2606_init(int bus) {
    si2c_init(bus);

    return 0;
}

/*
 * 功能描述：这个函数设置LTC2606输出电流值
 * 入参说明：volt --- 输出电压
 * 返 回 值：成功写入电流
 */
int ltc2606_write_volt(float volt) {
    int      ret;
    uint16_t dac_code;
    uint8_t  buff[3];
    uint8_t  retry = 3;

    if (volt > 5000000.0f) {
        return 0;
    }

    /* 尝试3次 */
    while (retry--) {
        dac_code = volt_to_dac_code(volt);
        buff[0]  = LTC2606_WRITE_UPDATE_COMMAND;
        buff[1]  = byte_1(dac_code);
        buff[2]  = byte_0(dac_code);

        ret = ltc2606_write(LTC2606_I2C_BUS, LTC2606_I2C_ADDR, buff, sizeof(buff));
        if (ret) {
            return ret;
        }
    }

    if (retry == 0) {
        return 0;
    }

    return ret;
}

/*
 * 功能描述：此函数向LTC2606设备写入多个字节
 * 入参说明：bus --- 总线编号，从0开始
 *           dev_addr --- LTC2606设备的I2C从设备地址（7位）
 *           buff---存放数据的缓冲区
 *           len----数据长度（字节）
 * 返 回 值：成功写入的字节数
 */
static int ltc2606_write(int bus, uint32_t dev_addr, void *buff, int len) {
    int ret;

    /* 开始传输 */
    si2c_start(bus);

    /* 传输从机地址用于写 */
    if (si2c_select(bus, dev_addr, SI2C_OP_WRITE) != 0) {
        si2c_stop(bus);
        return 0;
    }

    /* 传输数据 */
    ret = si2c_tx(bus, buff, len);

    /* 停止传输 */
    si2c_stop(bus);

    return ret;
}
