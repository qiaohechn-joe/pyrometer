/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ad7124.c
 * 文件描述：AD7124 (SPI 24位ADC)驱动代码
 * 作    者：和其光电嵌软团队
 * 备    注：支持AD7124-4 和 AD7124-8
 * 更新记录：V1.0 2024/02/18 陈军  初始版本
 *           V1.1 2024/07/19 陈军  增加模拟SPI支持
 */
#include "ad7124.h"

#define OP_RW              1 /* 读和写 */
#define OP_R               2 /* 只读 */
#define OP_W               3 /* 只写 */

#define BYTE1              1 /* 1字节 */
#define BYTE2              2 /* 2字节  */
#define BYTE3              3 /* 3字节 */
#define BYTE4              4 /* 4字节 */

/* scale */
// Code = 2(N - 1) × ((AIN × Gain/VREF) + 1)
#define VOL_SCALE_BIPOLAR  (float)(2500000 / (float)(1 << 23))
// Code = (2N × AIN × Gain)/VREF
#define VOL_SCALE_UNIPOLAR (float)(AD7124_VREF / (float)(1 << 24))

// const ad7124_spi_pin_cfg_t *ad7124_pin_cfg = AD7124_SPI_PINS_CFG;

/* AD7124寄存器默认配置表 */
/* clang-format off */
const ad7124_reg_t AD7124_DEF_REG_CFG0[AD7124_REG_CNT] = {
    {AD7124_REG_STAT,     0x00,     BYTE1, OP_R},  /* R[0x00] */
    {AD7124_REG_ADC_CTRL, 0x05C0,   BYTE2, OP_RW}, /* R[0x0000] */
    {AD7124_REG_DATA,     0x000000, BYTE4, OP_R},  /* R[0x000000] 附加状态寄存器共4byte*/
    {AD7124_REG_IO_CTRL1, 0x000000, BYTE3, OP_RW}, /* R[0x000000] */
    {AD7124_REG_IO_CTRL2, 0x0000,   BYTE2, OP_RW}, /* R[0x0000] */
    {AD7124_REG_ID,       0x00,     BYTE1, OP_R},  /* R[0x07(AD7124-4)/0x06(AD7124-4 B Grade)/0x12(AD7124-8)] */
    {AD7124_REG_ERR,      0x000000, BYTE3, OP_R},  /* R[0x000000] */
    {AD7124_REG_ERREN,    0x000040, BYTE3, OP_RW}, /* R[0x000040] */
    {AD7124_REG_MCLK_CNT, 0x00,     BYTE1, OP_R},  /* R[0x00] */
    {AD7124_REG_CH0,      0x8011,   BYTE2, OP_RW}, /* R[0x0001,same below] NV*/
    {AD7124_REG_CH1,      0x0001,   BYTE2, OP_RW}, /* R[0x0001,same below] */ //0x0031
    {AD7124_REG_CH2,      0x8051,   BYTE2, OP_RW}, /* R[0x8051] AIN2(+)/AVSS(-), WV 选择配置2 */
    {AD7124_REG_CH3,      0x0001,   BYTE2, OP_RW},
    {AD7124_REG_CH4,      0x9091,   BYTE2, OP_RW}, /* R[0x0001] AIN4(+)/AVSS(-), NTC */
    {AD7124_REG_CH5,      0x0001,   BYTE2, OP_RW},
    {AD7124_REG_CH6,      0x90D1,   BYTE2, OP_RW},
    {AD7124_REG_CH7,      0x0001,   BYTE2, OP_RW},
    {AD7124_REG_CH8,      0x0001,   BYTE2, OP_RW},
    {AD7124_REG_CH9,      0x0001,   BYTE2, OP_RW},
    {AD7124_REG_CH10,     0x0001,   BYTE2, OP_RW},
    {AD7124_REG_CH11,     0x0001,   BYTE2, OP_RW},
    {AD7124_REG_CH12,     0x0001,   BYTE2, OP_RW},
    {AD7124_REG_CH13,     0x0001,   BYTE2, OP_RW},
    {AD7124_REG_CH14,     0x0001,   BYTE2, OP_RW},
    {AD7124_REG_CH15,     0x0001,   BYTE2, OP_RW},
    {AD7124_REG_CFG0,     0x0060,   BYTE2, OP_RW}, /* R[0x0860,same below] */
    {AD7124_REG_CFG1,     0x0060,   BYTE2, OP_RW},
    {AD7124_REG_CFG2,     0x8060,   BYTE2, OP_RW},
    {AD7124_REG_CFG3,     0x0060,   BYTE2, OP_RW},
    {AD7124_REG_CFG4,     0x0060,   BYTE2, OP_RW},
    {AD7124_REG_CFG5,     0x0060,   BYTE2, OP_RW},
    {AD7124_REG_CFG6,     0x0060,   BYTE2, OP_RW},
    {AD7124_REG_CFG7,     0x0060,   BYTE2, OP_RW},
    {AD7124_REG_FILT0,    0x06001E, BYTE3, OP_RW}, //470140/* R[0x060180,same below] */   //FS[10:8] FS[7:0] 0x060078       3C 1E
    {AD7124_REG_FILT1,    0x060001, BYTE3, OP_RW},
    {AD7124_REG_FILT2,    0x06001E, BYTE3, OP_RW},
    {AD7124_REG_FILT3,    0x060180, BYTE3, OP_RW},
    {AD7124_REG_FILT4,    0x060180, BYTE3, OP_RW},//180,1E
    {AD7124_REG_FILT5,    0x060180, BYTE3, OP_RW},
    {AD7124_REG_FILT6,    0x060180, BYTE3, OP_RW},
    {AD7124_REG_FILT7,    0x060180, BYTE3, OP_RW},
    {AD7124_REG_OFFS0,    0x8003F8, BYTE3, OP_RW}, /* R[0x800000,same below] NV*/
    {AD7124_REG_OFFS1,    0x800000, BYTE3, OP_RW},
    {AD7124_REG_OFFS2,    0x7FFE80, BYTE3, OP_RW},
    {AD7124_REG_OFFS3,    0x800000, BYTE3, OP_RW},
    {AD7124_REG_OFFS4,    0x800000, BYTE3, OP_RW},
    {AD7124_REG_OFFS5,    0x800000, BYTE3, OP_RW},
    {AD7124_REG_OFFS6,    0x800000, BYTE3, OP_RW},
    {AD7124_REG_OFFS7,    0x800000, BYTE3, OP_RW},
    {AD7124_REG_GAIN0,    0x500000, BYTE3, OP_RW}, /* R[0x5XXXXX,same below] WV*/
    {AD7124_REG_GAIN1,    0x500000, BYTE3, OP_RW},
    {AD7124_REG_GAIN2,    0x500000, BYTE3, OP_RW},
    {AD7124_REG_GAIN3,    0x500000, BYTE3, OP_RW},
    {AD7124_REG_GAIN4,    0x500000, BYTE3, OP_RW},
    {AD7124_REG_GAIN5,    0x500000, BYTE3, OP_RW},
    {AD7124_REG_GAIN6,    0x500000, BYTE3, OP_RW},
    {AD7124_REG_GAIN7,    0x500000, BYTE3, OP_RW},
};
/* clang-format on */

/*
 * 这是驱动程序使用的“实时”AD7124寄存器映射
 * 其他“默认”配置用于在初始化时填充此配置
 */
ad7124_reg_t ad7124_reg_cfg0[AD7124_REG_CNT];

/* 内部函数 */
static int  write_reg(uint8_t addr, uint32_t val);
static int  read_reg(uint8_t addr, uint32_t *val);
static int  wait_spi_ready(void);
static void ad7124_reset(void);
static int  ad7124_wait_power_on(void);
static int  ad7124_read_id(uint32_t *id);
static int  ad7124_calibration(int ch);

/*
 * 功能描述：该函数初始化AD7124
 * 入参说明：para --- AD7124参数
 * 返 回 值：0 --- 成功  others --- 失败
 */
int ad7124_init(ad7124_para_t para) {
    uint32_t cntr;
    uint32_t id;
    int      ret;

    /* 数据结构 */
    mem_copy(ad7124_reg_cfg0, (const void *)AD7124_DEF_REG_CFG0, sizeof(ad7124_reg_cfg0));

    /* 读取AD7124增益和补偿参数 */
    //    ad7124_reg_cfg0[AD7124_REG_CFG0].value |= AD7124_CFG_PGA(para.pga);
    ad7124_reg_cfg0[AD7124_REG_OFFS0].value = para.offs0;
    ad7124_reg_cfg0[AD7124_REG_OFFS2].value = para.offs2;

    /* 硬件 */
    ad7124_init_pin();
    ad7124_high_cs();

    /* 复位 */
    ad7124_reset();

    /* 等待电源接通 */
    if (ad7124_wait_power_on()) {
        return -1;
    }

    /* 投递延时 */
    gd32_delay_ms(AD7124_POST_DELAY);

    /* 检查ID */
    if (ad7124_read_id(&id)) {
        return -2;
    }

    /* 设置为睡眠模式 */
    if (ad7124_set_mode(IDLE_MODE)) {
        return -3;
    }

    /* 写入默认寄存器配置 */
    for (cntr = AD7124_REG_STAT; cntr < AD7124_REG_OFFS7; cntr++) {
        if (ad7124_reg_cfg0[cntr].rw == OP_RW) {
            ret = write_reg(ad7124_reg_cfg0[cntr].addr, ad7124_reg_cfg0[cntr].value);
            if (ret) {
                return -4;
            }
        }
    }

    /* 设置为连续转换模式 */
    if (ad7124_set_mode(CONTINU_MODE)) {
        return -5;
    }

    /* 校准 */
    ad7124_calibration(1);

    return 0;
}

/*
 * 功能描述：该函数设置AD7124控制寄存器
 * 入参说明：op_mode --- 操作模式
 *           pwr_mode --- 电源模式
 *           ref_en --- 内部参考电压使能（1）/失能（2）
 *           clk_sel --- 选择时钟源
 * 返 回 值：0 --- 成功  others --- 失败
 */
int ad7124_set_control(uint8_t op_mode, uint8_t pwr_mode, uint8_t ref_en, uint8_t clk_sel) {
    uint32_t aux = 0;

    /* 读取 */
    if (read_reg(AD7124_REG_ADC_CTRL, &aux)) {
        return -1;
    }

    /* 写入 */
    aux = AD7124_ADC_CTRL_MODE(op_mode) | AD7124_ADC_CTRL_PWR_MODE(pwr_mode) | AD7124_ADC_CTRL_CLK_SEL(clk_sel) |
          (ref_en ? AD7124_ADC_CTRL_REF_EN : 0) | AD7124_ADC_CTRL_DOUT_RDY_DEL;

    if (write_reg(AD7124_REG_ADC_CTRL, aux)) {
        return -1;
    }

    return 0;
}

/*
 * 功能描述：该函数设置一个setup
 * 入参说明：cfg --- setup选择(0~7)
 *           ref --- 参考源选择
 *           pga --- 增益选择
 *           polar --- 增益选择, 1=双极型 0=单极性
 *           burnout --- 选择传感器熔断检测的幅度
 * 返 回 值：0 --- 成功  others --- 失败
 */
int ad7124_set_config(uint8_t cfg, uint8_t ref, uint8_t pga, uint8_t polar, uint8_t burnout) {
    uint32_t aux = 0;

    if (cfg < 8) {
        cfg += AD7124_REG_CFG0;
    }

    /* 读取 */
    if (read_reg(cfg, &aux)) {
        return -1;
    }

    /* 写入 */
    //    aux =   AD7124_CFG_PGA(pga);

    aux = AD7124_CFG_REF_SEL(ref) | AD7124_CFG_PGA(pga) | (polar ? AD7124_CFG_BIPOLAR : 0) | AD7124_CFG_BURNOUT(burnout) | AD7124_CFG_REF_BUFP |
          AD7124_CFG_REF_BUFM | AD7124_CFG_AIN_BUFP | AD7124_CFG_AINN_BUFM;

    if (write_reg(cfg, aux)) {
        return -1;
    }

    return 0;
}

/*
 * 功能描述：该函数设置PGA（可编程增益放大器）
 * 入参说明：cfg --- setup选择(0~7)
 *           pga --- 增益选择
 * 返 回 值：0 --- 成功  others --- 失败
 */
int ad7124_set_pga(uint8_t cfg, uint8_t pga) {
    uint32_t aux = 0;

    if (cfg < SETUP_CNT) {
        cfg += AD7124_REG_CFG0;
    }

    /* 读取 */
    if (read_reg(cfg, &aux)) {
        return -1;
    }

    /* 写入 */
    aux = AD7124_CFG_PGA(pga);

    if (write_reg(cfg, aux)) {
        return -1;
    }

    return 0;
}
/*
 * 功能描述：该函数设置补偿
 * 入参说明：cfg --- setup选择(0~7)
 *           ofs --- 补偿值
 * 返 回 值：0 --- 成功  others --- 失败
 */
int ad7124_set_offset(uint8_t cfg, uint32_t ofs) {
    if (cfg < SETUP_CNT) {
        cfg += AD7124_REG_OFFS0;
    }

    if (write_reg(cfg, ofs)) {
        return -1;
    }

    return 0;
}
/*
 * 功能描述：该函数使能/失能通道
 * 入参说明：ch --- 通道数（0-15）
 *           en --- 1=使能 0=失能
 * 返 回 值：0 --- 成功  others --- 失败
 */
int ad7124_enable_channel(uint8_t ch, uint8_t en) {
    uint32_t aux;

    if (ch > 15) {
        return -1;
    }

    /* 读取 */
    if (read_reg(AD7124_REG_CH0 + ch, &aux)) {
        return -1;
    }

    /* 写入 */
    if (en) {
        aux |= AD7124_CH_CH_ENABLE;
    } else {
        aux &= ~AD7124_CH_CH_ENABLE;
    }

    if (write_reg(AD7124_REG_CH0 + ch, aux)) {
        return -1;
    }

    return 0;
}
/*
 * 功能描述：该函数设置通道
 * 入参说明：ch --- 通道数（0-15）
 *           cfg --- CFG设置项选择（SETUP_0-7）。这些位标识了8个设置中的哪一个用于为该通道配置ADC
 *           ainp --- 正模拟输入AINP输入选择
 *           ainm --- 负模拟输入AINM输入选择
 *           en --- 通道使能位。设置此位可启用设备通道的转换顺序，1=使能 0=失能
 * 返 回 值：0 --- 成功  others --- 失败
 */
int ad7124_set_channel(uint8_t ch, uint8_t cfg, uint8_t ainp, uint8_t ainm, uint8_t en) {
    uint32_t aux = 0;

    if ((ch > 15) || (cfg > 7)) {
        return -1;
    }

    ch += AD7124_REG_CH0;

    /* 读取 */
    if (read_reg(ch, &aux)) {
        return -1;
    }

    /* 写入 */
    aux = AD7124_CH_SETUP(cfg) | AD7124_CH_AINP(ainp) | AD7124_CH_AINM(ainm) | (en ? AD7124_CH_CH_ENABLE : 0);

    if (write_reg(ch, aux)) {
        return -1;
    }

    return 0;
}
/*
 * 功能描述：该函数设置AD7124的操作模式
 * 入参说明：op_mode --- 操作模式
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
int ad7124_set_mode(int8_t op_mode) {
    uint32_t aux = 0;

    /* 读取 */
    if (read_reg(AD7124_REG_ADC_CTRL, &aux)) {
        return -1;
    }

    /* 写入 */
    aux &= ~AD7124_ADC_CTRL_MODE(0x0F); /* 清除 */
    aux |= AD7124_ADC_CTRL_MODE(op_mode);

    if (write_reg(AD7124_REG_ADC_CTRL, aux)) {
        return -1;
    }

    return 0;
}

/*
 * 功能描述：该函数检查ADC转换数据是否准备好
 * 入参说明：timeout --- 超时值(毫秒)
 * 返 回 值：0-15=转换数据通道数，-1=转换数据未准备好
 */
int ad7124_conv_ready(void) {
    uint32_t aux;

    if (read_reg(AD7124_REG_STAT, &aux)) {
        return -1;
    }
    if (aux & AD7124_STAT_RDY) {
        return -1; /* bit7=1: 未准备好 */
    }

    return (aux & 0x0f); /* bit[3:0] */
}

/*
 * 功能描述：该函数读取ADC转换数据
 * 入参说明：data --- 指向结果[OUT]的指针
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
int ad7124_read_data(uint32_t *data) {
    return read_reg(AD7124_REG_DATA, data);
}

/*
 * 功能描述：该函数将ADC的原始值转化为电压值
 * 入参说明：ch --- 通道数（0-15）
 *           val --- ADC原始值
 *           err --- 转换错误状态 0：成功，1：错误
 * 返 回 值：电压（微伏）
 */
float ad7124_raw_to_vol(uint8_t ch, uint32_t val, uint8_t *err) {
    uint32_t aux;
    float    vol;
    uint8_t  setup_num;
    int      polar;
    int      gain;

    *err = 0;

    if (ch > 15) {
        *err = 1;
        return 0;
    }

    /* 读取 */
    if (read_reg(AD7124_REG_CH0 + ch, &aux)) {
        *err = 1;
        return 0;
    }

    /* setup数目 : bit[14:12] */
    setup_num = get_setup_number(aux);
    if (read_reg(AD7124_REG_CFG0 + setup_num, &aux)) {
        *err = 1;
        return 0;
    }

    polar = get_channel_bipolar(aux);
    gain  = get_channel_pga(aux);

    if (polar) {
        vol = (((float)val / (1 << (24 - 1))) - 1) * (AD7124_REF_INTER / gain);
    } else {
        vol = ((float)val * AD7124_REF_EXTER1) / (1 << 24);
    }

    return vol;
}

/*
 * 功能描述：该函数写入AD7124寄存器
 * 入参说明：addr --- 寄存器地址
 *           val --- 寄存器值
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
static int write_reg(uint8_t addr, uint32_t val) {
    uint8_t buff[8];
    int     size;
    int     i;

    if (addr >= AD7124_REG_CNT) {
        return -1;
    }

    if (AD7124_DEF_REG_CFG0[addr].rw == OP_R) {
        return -1; /* 只读 */
    }

    /* 等待SPI就绪 */
    if (wait_spi_ready() != 0) {
        return -1;
    }

    /* 构造 */
    size = AD7124_DEF_REG_CFG0[addr].size;
    mem_set(buff, sizeof(buff), 0);
    buff[0] = WR_REGISTER(addr); /* 命令 */
    for (i = 0; i < size; i++) {
        buff[size - i] = val & 0x0ff;
        val            = val >> 8;
    }

    /* 发送 */
#if defined(AD7124_SOFT_SPI)
    sspi_low_nss();
    for (i = 0; i < size + 1; i++) {
        sspi_transfer(buff[i]);
    }
    sspi_high_nss();
#else
    ad7124_low_cs();
    for (i = 0; i < size + 1; i++) {
        gd32_spi_xfer(AD7124_SPI_DEV, buff[i]);
    }
    ad7124_high_cs();
#endif

    return 0;
}
/*
 * 功能描述：该函数从AD7124读取寄存器
 * 入参说明：addr --- 寄存器地址
 *           val --- 指向寄存器值[OUT]的指针
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
static int read_reg(uint8_t addr, uint32_t *val) {
    uint8_t  buff[8];
    uint32_t result;
    int      size;
    int      i;

    if (addr >= AD7124_REG_CNT) {
        return -1;
    }

    /* 等待SPI就绪 */
    if (addr != AD7124_REG_ERR) {
        if (wait_spi_ready() != 0) {
            return -1;
        }
    }

    /* 读取 */
    mem_set(buff, sizeof(buff), 0);
    buff[0] = RD_REGISTER(addr); /* 命令 */
    size    = AD7124_DEF_REG_CFG0[addr].size + 1;

#if defined(AD7124_SOFT_SPI)
    sspi_low_nss();
    for (i = 0; i < size + 1; i++) {
        buff[i] = sspi_transfer(buff[i]);
    }
    sspi_high_nss();
#else
    ad7124_low_cs();
    for (i = 0; i < size; i++) {
        buff[i] = gd32_spi_xfer(AD7124_SPI_DEV, buff[i]);
    }
    ad7124_high_cs();
#endif

    /* 构建结果 */
    result = 0;
    for (i = 1; i < size; i++) {
        result = result << 8;
        result |= buff[i];
    }

    *val = result;

    return 0;
}
/*
 * 功能描述：该函数等待SPI接口就绪
 * 入参说明：None
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
static int wait_spi_ready(void) {
    uint32_t cntr;
    uint32_t aux;
    int      ret;

    for (cntr = AD7124_SPI_RDY_TIMEOUT / 10; cntr != 0; cntr--) {
        ret = read_reg(AD7124_REG_ERR, &aux);
        if ((ret == 0) && ((aux & AD7124_ERR_SPI_IGNORE_ERR) == 0)) {
            break;
        }

        gd32_delay_us(10);
    }

    return (cntr == 0) ? -1 : 0;
}

/*
 * 功能描述：该函数复位AD7124
 * 入参说明：None
 * 返 回 值：None
 */
static void ad7124_reset(void) {
    uint32_t cntr;

#if defined(AD7124_SOFT_SPI)
    sspi_low_nss();
    for (cntr = 8; cntr != 0; cntr--) {
        sspi_transfer(0x0ff);
    }
    sspi_high_nss();
#else
    ad7124_low_cs();
    for (cntr = 8; cntr != 0; cntr--) {
        gd32_spi_xfer(AD7124_SPI_DEV, 0x0ff);
    }
    ad7124_high_cs();
#endif

    gd32_delay_ms(AD7124_POST_RESET_DELAY);
}

/*
 * 功能描述：该函数等待AD7124完成上电复位操作
 * 入参说明：None
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
static int ad7124_wait_power_on(void) {
    uint32_t aux;
    for (int cntr = AD7124_POR_TIMEOUT / 10; cntr > 0; cntr--) {
        if (read_reg(AD7124_REG_STAT, &aux) == 0 && (aux & AD7124_STAT_POR_FLG) == 0) {
            return 0;
        }
        gd32_delay_us(10);
    }
    return -1;
}

/*
 * 功能描述：该函数读取AD7124的识别号
 * 入参说明：id——指向标识号的指针
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
static int ad7124_read_id(uint32_t *id) {
    if (read_reg(AD7124_REG_ID, id)) {
        return -1;
    }
    if ((*id == AD7124_4_DEVICE_ID) || (*id == AD7124_4_DEVICE_ID_OLD)) {
        return 0;
    }
    return -1;
}

/*
 * 功能描述：该函数校准AD7124
 * 入参说明：ch --- 通道数（1-15）
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
static int ad7124_calibration(int ch) {
    return 0;
}

/*
 * 功能描述：该函数校准偏置及增益
 * 入参说明：ch --- 通道数（1-15）
 *           offset --- 指向补偿值的指针
 *           gain --- 指向增益的指针
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
static int ad7124_offset_gain_calibration(uint8_t ch, uint32_t *offset, uint32_t *gain) {
    uint32_t aux;
    uint32_t offset_value;

    /* 读取 */
    if (read_reg(AD7124_REG_ADC_CTRL, &aux)) {
        return -1;
    }

    /* 写入 */
    aux &= ~AD7124_ADC_CTRL_PWR_MODE(0x3); /* 清除 */
    aux |= AD7124_ADC_CTRL_PWR_MODE(MID_POWER_MODE);

    aux &= ~AD7124_ADC_CTRL_MODE(0x0F); /* 清除 */
    aux |= AD7124_ADC_CTRL_MODE(IDLE_MODE);
    if (write_reg(AD7124_REG_ADC_CTRL, aux)) {
        return -1;
    }

    gd32_delay_ms(1000);

    offset_value = 0x7FFE20;

    if (write_reg(AD7124_REG_OFFS0, offset_value)) {
        return -1;
    }

    gd32_delay_ms(1000);

    if (read_reg(AD7124_REG_ADC_CTRL, &aux)) {
        return -1;
    }

    /* 写入 */
    aux &= ~AD7124_ADC_CTRL_PWR_MODE(0x3); /* 清除 */
    aux |= AD7124_ADC_CTRL_PWR_MODE(FULL_POWER_MODE);

    aux &= ~AD7124_ADC_CTRL_MODE(0x0F); /* 清除 */
    aux |= AD7124_ADC_CTRL_MODE(CONTINU_MODE);
    if (write_reg(AD7124_REG_ADC_CTRL, aux)) {
        return -1;
    }

    gd32_delay_ms(1000);

    return 0;
}
