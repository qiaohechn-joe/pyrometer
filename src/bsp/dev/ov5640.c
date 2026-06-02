/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ov5640.c
 * 文件描述：ov5640驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/11/18  张晓博、乔鹤 初始版本
 *           V1.1 2025/03/08  陈军 整理
 */

#include "soft_i2c.h"
#include "ov5640.h"
#include "ov5640_cfg.h"
#include "platform.h"

static uint8_t sccb_write_reg(uint16_t reg, uint8_t data);
static uint8_t sccb_read_reg(uint16_t reg);

/*
 * 功能描述：此函数初始化OV5640并且检查器件是否在位
 * 入参说明：无
 * 返 回 值：0 --- 成功，其它 --- 失败
 * 备    注：配置完以后,默认输出是1600*1200尺寸的图片
 */
int ov5640_init(void) {
    uint16_t chip_id = 0;

    ov5640_ctrl_pin_init();
    si2c_init(SCCB_I2C_BUS);

    ov5640_rst_low(); /* 必须先拉低OV5640的RST脚,再上电 */
    gd32_delay_ms(20);
    ov5640_power_on(); /* POWER ON */
    gd32_delay_ms(5);
    ov5640_rst_high(); /* 结束复位 */
    gd32_delay_ms(20);

    gd32_delay_ms(5);
    chip_id = sccb_read_reg(OV5640_CHIPIDH); /* 读取ID高八位 */
    chip_id <<= 8;
    chip_id |= sccb_read_reg(OV5640_CHIPIDL); /* 读取ID低八位 */
    if (chip_id != OV5640_CHIP_ID) {
        //        dbg_info("%s", "ov5640 is not found!\r\n");
        return -1;
    }
    sccb_write_reg(0x3103, 0X11); /* system clock from pad, bit[1] */
    sccb_write_reg(0X3008, 0X82); /* 软复位 */

    gd32_delay_ms(10);

    /* 初始化OV5640,采用SXGA分辨率(1600*1200) */
    for (uint16_t i = 0; i < sizeof(OV5640_UXGA_REG_TBL) / 4; i++) {
        sccb_write_reg(OV5640_UXGA_REG_TBL[i][0], OV5640_UXGA_REG_TBL[i][1]);
        gd32_delay_us(50);
    }

    return 0;
}

/*
 * 功能描述：这个函数将 OV5640 切换为 JPEG 模式
 * 入参说明：无
 * 返 回 值：无
 */
void ov5640_jpeg_mode(void) {
    uint16_t i = 0;

    /* 设置:输出JPEG数据 */
    for (i = 0; i < (sizeof(OV5640_JPEG_REG_TBL) / 4); i++) {
        sccb_write_reg(OV5640_JPEG_REG_TBL[i][0], OV5640_JPEG_REG_TBL[i][1]);
    }
}

/*
 * 功能描述：这个函数将 OV5640 切换为 RGB565 模式
 * 入参说明：无
 * 返 回 值：无
 */
void ov5640_rgb565_mode(void) {
    uint16_t i = 0;

    /* 设置:RGB565输出 */
    for (i = 0; i < (sizeof(OV5640_RGB565_REG_TBL) / 4); i++) {
        sccb_write_reg(OV5640_RGB565_REG_TBL[i][0], OV5640_RGB565_REG_TBL[i][1]);
    }
}

/*
 * 功能描述：这个函数获取OV5640的曝光值
 * 入参说明：无
 * 返 回 值：曝光值
 */
uint16_t ov5640_get_exposure(void) {
    uint16_t reg = 0;

    reg = sccb_read_reg(0x3501); /* 读取高八位 */
    reg <<= 8;
    reg |= sccb_read_reg(0x3502); /* 读取低八位 */

    return reg;
}

/*
 * 功能描述：这个函数设置OV5640的曝光模式
 * 入参说明：mode --- 0：自动，1：手动
 * 返 回 值：无
 */
void ov5640_set_exposure(uint8_t mode) {
    if (mode == 0)
        sccb_write_reg(0x3503, 0x00);
    else
        sccb_write_reg(0x3503, 0x07);
}

/*
 * 功能描述：设置OV5640摄像头的快门时间
 * 入参说明：shutter --- 代表了控制光线进入传感器的时间长度
 * 返 回 值：无
 */
int ov5640_set_shutter(int shutter) {
    int temp;

    shutter = shutter & 0xffff;
    temp    = shutter & 0x0f;
    temp    = temp << 4;
    sccb_write_reg(0x3502, temp);
    temp = shutter & 0xfff;
    temp = temp >> 4;
    sccb_write_reg(0x3501, temp);
    temp = shutter >> 12;
    sccb_write_reg(0x3500, temp);

    return 0;
}

/*
 * 功能描述：EV曝光补偿
 * 入参说明：exposure --- 0~6，代表补偿-3~3
 * 返 回 值：无
 */
void ov5640_exposure(uint8_t exposure) {
    // sccb_write_reg(0x3212,0x03); /* start group 3 */
    sccb_write_reg(0x3a0f, OV5640_EXPOSURE_TBL[exposure][0]);
    sccb_write_reg(0x3a10, OV5640_EXPOSURE_TBL[exposure][1]);
    sccb_write_reg(0x3a1b, OV5640_EXPOSURE_TBL[exposure][2]);
    sccb_write_reg(0x3a1e, OV5640_EXPOSURE_TBL[exposure][3]);
    sccb_write_reg(0x3a11, OV5640_EXPOSURE_TBL[exposure][4]);
    sccb_write_reg(0x3a1f, OV5640_EXPOSURE_TBL[exposure][5]);
    // sccb_write_reg(0x3212,0x13); /* end group 3 */
    // sccb_write_reg(0x3212,0xa3); /* launch group 3 */
}

/*
 * 功能描述：白平衡设置
 * 入参说明：mode --- 0：自动
 *                    1：日光sunny
 *                    2：办公室office
 *                    3：阴天cloudy
 *                    4：家里home
 * 返 回 值：无
 */
void ov5640_light_mode(uint8_t mode) {
    uint8_t i;

    sccb_write_reg(0x3212, 0x03);                                                      /* start group 3 */
    for (i = 0; i < 7; i++) sccb_write_reg(0x3400 + i, OV5640_LIGHTMODE_TBL[mode][i]); /* 设置饱和度 */

    sccb_write_reg(0x3212, 0x13); /* end group 3 */
    sccb_write_reg(0x3212, 0xa3); /* launch group 3 */
}

/*
 * 功能描述：色度设置
 * 入参说明：sat --- 0~6，代表饱和度-3~3
 * 返 回 值：无
 */
void ov5640_color_saturation(uint8_t sat) {
    uint8_t i;

    sccb_write_reg(0x3212, 0x03); /* start group 3 */
    sccb_write_reg(0x5381, 0x1c);
    sccb_write_reg(0x5382, 0x5a);
    sccb_write_reg(0x5383, 0x06);
    for (i = 0; i < 6; i++) sccb_write_reg(0x5384 + i, OV5640_SATURATION_TBL[sat][i]); /* 设置饱和度 */
    sccb_write_reg(0x538b, 0x98);
    sccb_write_reg(0x538a, 0x01);
    sccb_write_reg(0x3212, 0x13); /* end group 3 */
    sccb_write_reg(0x3212, 0xa3); /* launch group 3 */
}

/*
 * 功能描述：亮度设置
 * 入参说明：bright --- 0~8，代表亮度-4~4
 * 返 回 值：无
 */
void ov5640_brightness(uint8_t bright) {
    uint8_t brtval;

    if (bright < 4)
        brtval = 4 - bright;
    else
        brtval = bright - 4;
    sccb_write_reg(0x3212, 0x03); /* start group 3 */
    sccb_write_reg(0x5587, brtval << 4);
    if (bright < 4)
        sccb_write_reg(0x5588, 0x09);
    else
        sccb_write_reg(0x5588, 0x01);
    sccb_write_reg(0x3212, 0x13); /* end group 3 */
    sccb_write_reg(0x3212, 0xa3); /* launch group 3 */
}

/*
 * 功能描述：对比度设置
 * 入参说明：contrast --- 0~6,代表亮度-3~3
 * 返 回 值：无
 */
void ov5640_contrast(uint8_t contrast) {
    uint8_t reg0val = 0X00; /* contrast=3,默认对比度 */
    uint8_t reg1val = 0X20;

    switch (contrast) {
        case 0: reg1val = reg0val = 0X14; break; /* -3 */
        case 1: reg1val = reg0val = 0X18; break; /* -2 */
        case 2: reg1val = reg0val = 0X1C; break; /* -1 */
        case 4:
            reg0val = 0X10;
            reg1val = 0X24;
            break; /* 1 */
        case 5:
            reg0val = 0X18;
            reg1val = 0X28;
            break; /* 2 */
        case 6:
            reg0val = 0X1C;
            reg1val = 0X2C;
            break; /* 3 */
    }
    sccb_write_reg(0x3212, 0x03); /* start group 3 */
    sccb_write_reg(0x5585, reg0val);
    sccb_write_reg(0x5586, reg1val);
    sccb_write_reg(0x3212, 0x13); /* end group 3 */
    sccb_write_reg(0x3212, 0xa3); /* launch group 3 */
}

/*
 * 功能描述：锐度设置
 * 入参说明：sharp --- 0：关闭 33：auto 锐度范围0~33
 * 返 回 值：无
 */
void ov5640_sharpness(uint8_t sharp) {
    if (sharp < 33) /* 设置锐度值 */
    {
        sccb_write_reg(0x5308, 0x65);
        sccb_write_reg(0x5302, sharp);
    } else /* 自动锐度 */
    {
        sccb_write_reg(0x5308, 0x25);
        sccb_write_reg(0x5300, 0x08);
        sccb_write_reg(0x5301, 0x30);
        sccb_write_reg(0x5302, 0x10);
        sccb_write_reg(0x5303, 0x00);
        sccb_write_reg(0x5309, 0x08);
        sccb_write_reg(0x530a, 0x30);
        sccb_write_reg(0x530b, 0x04);
        sccb_write_reg(0x530c, 0x06);
    }
}

/*
 * 功能描述：特效设置
 * 入参说明：eft --- 0：正常
 *                   1：冷色
 *                   2：暖色
 *                   3：黑白
 *                   4：偏黄
 *                   5：反色
 *                   6：偏绿
 * 返 回 值：无
 */
void ov5640_special_effects(uint8_t eft) {
    sccb_write_reg(0x3212, 0x03); /* start group 3 */
    sccb_write_reg(0x5580, OV5640_EFFECTS_TBL[eft][0]);
    sccb_write_reg(0x5583, OV5640_EFFECTS_TBL[eft][1]); /* sat U */
    sccb_write_reg(0x5584, OV5640_EFFECTS_TBL[eft][2]); /* sat V */
    sccb_write_reg(0x5003, 0x08);
    sccb_write_reg(0x3212, 0x13); /* end group 3 */
    sccb_write_reg(0x3212, 0xa3); /* launch group 3 */
}

/*
 * 功能描述：测试序列
 * 入参说明：mode --- 0：关闭
 *                    1：彩条
 *                    2：色块
 * 返 回 值：无
 */
void ov5640_test_pattern(uint8_t mode) {
    if (mode == 0)
        sccb_write_reg(0X503D, 0X00);
    else if (mode == 1)
        sccb_write_reg(0X503D, 0X80);
    else if (mode == 2)
        sccb_write_reg(0X503D, 0X82);
}

/*
 * 功能描述：闪光灯控制
 * 入参说明：sw --- 0：关闭 1：打开
 * 返 回 值：无
 */
void ov5640_flash_ctrl(uint8_t sw) {
    sccb_write_reg(0x3016, 0X02);
    sccb_write_reg(0x301C, 0X02);

    if (sw)
        sccb_write_reg(0X3019, 0X02);
    else
        sccb_write_reg(0X3019, 0X00);
}

/*
 * 功能描述：设置图像输出大小
 * 入参说明：offx,offy --- 为输出图像在OV5640_ImageWin_Set设定窗口(假设长宽为xsize和ysize)上的偏移。
 *                         由于开启了scale功能,用于输出的图像窗口为:xsize-2*offx,ysize-2*offy
 *           width --- 实际输出图像宽度(对应:horizontal)
 *           height --- 实际输出图像高度(对应:vertical)
 * 返 回 值：0 --- 成功，其它失败
 * 备    注：1）OV5640输出图像的大小(分辨率),完全由该函数确定。
 *           2）实际输出(width,height)，是在xsize-2*offx、ysize-2*offy的基础上进行缩放处理，
 *              一般设置offx和offy的值为16和4,更小也是可以,不过默认是16和4。
 */
void ov5640_outsize_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height) {
    //    sccb_write_reg(0X3212, 0X03); /* 开始组3 */
    /* 以下设置决定实际输出尺寸(带缩放) */
    sccb_write_reg(0x3808, width >> 8);    /* 设置实际输出宽度高字节 */
    sccb_write_reg(0x3809, width & 0xff);  /* 设置实际输出宽度低字节 */
    sccb_write_reg(0x380a, height >> 8);   /* 设置实际输出高度高字节 */
    sccb_write_reg(0x380b, height & 0xff); /* 设置实际输出高度低字节 */
                                           /* 以下设置决定输出尺寸在ISP上面的取图范围 */
                                           /* 范围:xsize-2*offx,ysize-2*offy */
    sccb_write_reg(0x3810, offx >> 8);     /* 设置X offset高字节 */
    sccb_write_reg(0x3811, offx & 0xff);   /* 设置X offset低字节 */

    sccb_write_reg(0x3812, offy >> 8);   /* 设置Y offset高字节 */
    sccb_write_reg(0x3813, offy & 0xff); /* 设置Y offset低字节 */

    //    sccb_write_reg(0X3212, 0X13); /* 结束组3 */
    //    sccb_write_reg(0X3212, 0Xa3); /* 启用组3设置 */
}

/*
 * 功能描述：设置图像开窗大小（ISP大小），非必要，一般无需调用此函数
 * 入参说明：offx,offy --- 为输出图像在OV5640_ImageWin_Set设定窗口(假设长宽为xsize和ysize)上的偏移。
 *                         由于开启了scale功能,用于输出的图像窗口为:xsize-2*offx,ysize-2*offy
 *           width --- 实际输出图像宽度(对应:horizontal)
 *           height --- 实际输出图像高度(对应:vertical)
 * 返 回 值：0 --- 成功，其它失败
 * 备    注：在整个传感器上面开窗(最大2592*1944),用于OV5640_OutSize_Set的输出
 *           本函数的宽度和高度,必须大于等于OV5640_OutSize_Set函数的宽度和高度
 *           OV5640_OutSize_Set设置的宽度和高度,根据本函数设置的宽度和高度,由DSP
 *           自动计算缩放比例,输出给外部设备
 */
int ov5640_imagewin_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height) {
    uint16_t xst, yst, xend, yend;

    xst  = offx;
    yst  = offy;
    xend = offx + width - 1;
    yend = offy + height - 1;
    // sccb_write_reg(0X3212,0X03); /* 开始组3 */
    sccb_write_reg(0X3800, xst >> 8);
    sccb_write_reg(0X3801, xst & 0XFF);
    sccb_write_reg(0X3802, yst >> 8);
    sccb_write_reg(0X3803, yst & 0XFF);
    sccb_write_reg(0X3804, xend >> 8);
    sccb_write_reg(0X3805, xend & 0XFF);
    sccb_write_reg(0X3806, yend >> 8);
    sccb_write_reg(0X3807, yend & 0XFF);
    // sccb_write_reg(0X3212,0X13); /* 结束组3 */
    // sccb_write_reg(0X3212,0Xa3); /* 启用组3设置 */

    return 0;
}

/*
 * 功能描述：初始化自动对焦
 * 入参说明：无
 * 返 回 值：0 --- 成功，其它失败
 */
int ov5640_focus_init(void) {
    uint16_t i;
    uint16_t addr  = 0x8000;
    uint8_t  state = 0x8F;

    sccb_write_reg(0x3000, 0x20);                  /* reset MCU */
    for (i = 0; i < sizeof(OV5640_AF_CONFIG); i++) /* 发送配置数组 */
    {
        sccb_write_reg(addr, OV5640_AF_CONFIG[i]);
        addr++;
    }
    sccb_write_reg(0x3022, 0x00);
    sccb_write_reg(0x3023, 0x00);
    sccb_write_reg(0x3024, 0x00);
    sccb_write_reg(0x3025, 0x00);
    sccb_write_reg(0x3026, 0x00);
    sccb_write_reg(0x3027, 0x00);
    sccb_write_reg(0x3028, 0x00);
    sccb_write_reg(0x3029, 0x7f);
    sccb_write_reg(0x3000, 0x00);
    i = 0;
    do {
        state = sccb_read_reg(0x3029);
        gd32_delay_ms(5);
        i++;
        if (i > 1000) return -1;
    } while (state != 0x70);

    return 0;
}

/*
 * 功能描述：执行一次自动对焦
 * 入参说明：无
 * 返 回 值：0 --- 成功，其它失败
 */
int ov5640_focus_single(void) {
    uint8_t  temp;
    uint16_t retry = 0;

    sccb_write_reg(0x3022, 0x03); /* 触发一次自动对焦 */
    while (1) {
        retry++;
        temp = sccb_read_reg(0x3029); /* 检查对焦完成状态 */
        if (temp == 0x10) break;      /* focus completed */
        gd32_delay_ms(5);
        if (retry > 1000) return -1;
    }

    return 0;
}

/*
 * 功能描述：持续自动对焦,当失焦后,会自动继续对焦
 * 入参说明：无
 * 返 回 值：0 --- 成功，其它失败
 */
int ov5640_focus_constant(void) {
    uint8_t  temp  = 0;
    uint16_t retry = 0;

    sccb_write_reg(0x3023, 0x01);
    sccb_write_reg(0x3022, 0x08); /* 发送IDLE指令 */
    do {
        temp = sccb_read_reg(0x3023);
        retry++;
        if (retry > 1000) return -1;
        gd32_delay_ms(5);
    } while (temp != 0x00);

    sccb_write_reg(0x3023, 0x01);
    sccb_write_reg(0x3022, 0x04); /* 发送持续对焦指令 */
    retry = 0;
    do {
        temp = sccb_read_reg(0x3023);
        retry++;
        if (retry > 1000) return -1;
        gd32_delay_ms(5);
    } while (temp != 0x00); /* 0,对焦完成;1:正在对焦 */

    return 0;
}

/*
 * 功能描述：这个函数用于SCCB总线向寄存器写入数据
 * 入参说明：reg --- 寄存器
 *           data --- 数据
 * 返 回 值：写入的个数
 */
static uint8_t sccb_write_reg(uint16_t reg, uint8_t data) {
    int     ret;
    uint8_t wrbuf[3];

    if (dac_camera_lock()) {
        return 0;
    }
    wrbuf[0] = (reg >> 8) & 0xFF;
    wrbuf[1] = reg & 0xFF;
    wrbuf[2] = data;

    /* 开始传输 */
    si2c_start(SCCB_I2C_BUS);

    /* 传输从设备地址以进行写操作 */
    if (si2c_select(SCCB_I2C_BUS, SCCB_I2C_ADDR, SI2C_OP_WRITE) != 0) {
        si2c_stop(SCCB_I2C_BUS);
        return 0;
    }

    /* 传输数据 */
    ret = si2c_tx(SCCB_I2C_BUS, wrbuf, 3);

    /* 停止传输 */
    si2c_stop(SCCB_I2C_BUS);

    dac_camera_unlock();

    return ret;
}

/*
 * 功能描述：这个函数用于SCCB总线读寄存器
 * 入参说明：reg --- 寄存器
 * 返 回 值：读取的数据
 */
static uint8_t sccb_read_reg(uint16_t reg) {
    int     ret;
    uint8_t val = 0;
    uint8_t wrbuf[2];

    if (dac_camera_lock()) {
        return 0;
    }

    wrbuf[0] = (reg >> 8) & 0xFF;
    wrbuf[1] = reg & 0xFF;

    /* 传输开始 */
    si2c_start(SCCB_I2C_BUS);

    /* 传输从设备地址以进行写操作 */
    if (si2c_select(SCCB_I2C_BUS, SCCB_I2C_ADDR, SI2C_OP_WRITE) != 0) {
        si2c_stop(SCCB_I2C_BUS);
        return 0;
    }

    /* 传输存储地址 */
    if (si2c_tx(SCCB_I2C_BUS, wrbuf, 2) != 2) {
        si2c_stop(SCCB_I2C_BUS);
        return 0;
    }

    /* 传输开始 */
    si2c_start(SCCB_I2C_BUS);

    /* 传输从设备地址以进行读操作 */
    if (si2c_select(SCCB_I2C_BUS, SCCB_I2C_ADDR, SI2C_OP_READ) != 0) {
        si2c_stop(SCCB_I2C_BUS);
        return 0;
    }

    /* 接收数据 */
    ret = si2c_rx(SCCB_I2C_BUS, &val, 1);
    (void)ret;

    /* 传输停止 */
    si2c_stop(SCCB_I2C_BUS);

    dac_camera_unlock();

    return val;
}

/*
 * 功能描述：设置OV5640图像压缩质量（JPEG模式）
 * 入参说明：quality 压缩质量，范围 1（高压缩率）~ 100（高质量）
 * 返 回 值：0成功，1失败
 */
uint8_t ov5640_set_quality(uint8_t quality) {
    uint8_t value;

    /* 限制输入范围 */
    if (quality < 1) {
        quality = 1;
    } else if (quality > 100) {
        quality = 100;
    }

    /*
     * OV5640压缩质量控制：寄存器 0x4407
     * 通常 JPEG质量越高，压缩比越低，图像越清晰但数据量越大
     * 这个寄存器越小压缩越大；典型值：0x10~0x30
     */
    if (quality <= 10) {
        value = 0x10; /* 极高压缩，帧小，适合低带宽传输 */
    } else if (quality <= 30) {
        value = 0x20;
    } else if (quality <= 60) {
        value = 0x30;
    } else if (quality <= 80) {
        value = 0x40;
    } else {
        value = 0x50; /* 最高质量（大图像） */
    }

    return sccb_write_reg(0x4407, value);
}
