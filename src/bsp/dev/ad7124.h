/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ad7124.h
 * 文件描述：AD7124 (SPI 24位ADC)驱动代码
 * 作    者：和其光电嵌软团队
 * 备    注：支持AD7124-4 和 AD7124-8
 * 更新记录：V1.0 2024/02/18 陈军  初始版本
 *           V1.1 2024/07/19 陈军  增加模拟SPI支持
 */

#ifndef _AD7124_H_
#define _AD7124_H_

#include "gd32_ll.h"
#include "gd32_spi.h"
#include "hq_generic.h"
#include "lib_cfg.h"

typedef enum {
    AD7124_PIN_MOSI = 0, // 主输出从输入引脚
    AD7124_PIN_MISO,     // 主输入从输出引脚
    AD7124_PIN_SCK,      // 串行时钟引脚
    AD7124_PIN_NSS,      // 片选引脚（也称为从选择引脚）
} AD7124_SPI_Pin_TypeDef;

/* 寄存器地址 */
#define AD7124_REG_COMM                    0x00
#define AD7124_REG_STAT                    0x00
#define AD7124_REG_ADC_CTRL                0x01
#define AD7124_REG_DATA                    0x02
#define AD7124_REG_IO_CTRL1                0x03
#define AD7124_REG_IO_CTRL2                0x04
#define AD7124_REG_ID                      0x05
#define AD7124_REG_ERR                     0x06
#define AD7124_REG_ERREN                   0x07
#define AD7124_REG_MCLK_CNT                0x08
#define AD7124_REG_CH0                     0x09
#define AD7124_REG_CH1                     0x0A
#define AD7124_REG_CH2                     0x0B
#define AD7124_REG_CH3                     0x0C
#define AD7124_REG_CH4                     0x0D
#define AD7124_REG_CH5                     0x0E
#define AD7124_REG_CH6                     0x0F
#define AD7124_REG_CH7                     0x10
#define AD7124_REG_CH8                     0x11
#define AD7124_REG_CH9                     0x12
#define AD7124_REG_CH10                    0x13
#define AD7124_REG_CH11                    0x14
#define AD7124_REG_CH12                    0x15
#define AD7124_REG_CH13                    0x16
#define AD7124_REG_CH14                    0x17
#define AD7124_REG_CH15                    0x18
#define AD7124_REG_CFG0                    0x19
#define AD7124_REG_CFG1                    0x1A
#define AD7124_REG_CFG2                    0x1B
#define AD7124_REG_CFG3                    0x1C
#define AD7124_REG_CFG4                    0x1D
#define AD7124_REG_CFG5                    0x1E
#define AD7124_REG_CFG6                    0x1F
#define AD7124_REG_CFG7                    0x20
#define AD7124_REG_FILT0                   0x21
#define AD7124_REG_FILT1                   0x22
#define AD7124_REG_FILT2                   0x23
#define AD7124_REG_FILT3                   0x24
#define AD7124_REG_FILT4                   0x25
#define AD7124_REG_FILT5                   0x26
#define AD7124_REG_FILT6                   0x27
#define AD7124_REG_FILT7                   0x28
#define AD7124_REG_OFFS0                   0x29
#define AD7124_REG_OFFS1                   0x2A
#define AD7124_REG_OFFS2                   0x2B
#define AD7124_REG_OFFS3                   0x2C
#define AD7124_REG_OFFS4                   0x2D
#define AD7124_REG_OFFS5                   0x2E
#define AD7124_REG_OFFS6                   0x2F
#define AD7124_REG_OFFS7                   0x30
#define AD7124_REG_GAIN0                   0x31
#define AD7124_REG_GAIN1                   0x32
#define AD7124_REG_GAIN2                   0x33
#define AD7124_REG_GAIN3                   0x34
#define AD7124_REG_GAIN4                   0x35
#define AD7124_REG_GAIN5                   0x36
#define AD7124_REG_GAIN6                   0x37
#define AD7124_REG_GAIN7                   0x38
#define AD7124_REG_CNT                     0x39

/* 寄存器位 */
/* 通信寄存器位 */
#define AD7124_COMM_WEN                    (0 << 7)
#define AD7124_COMM_WR                     (0 << 6)
#define AD7124_COMM_RD                     (1 << 6)
#define AD7124_COMM_RA(x)                  ((x) & 0x3F)

/* 状态寄存器位 */
#define AD7124_STAT_RDY                    (1 << 7)
#define AD7124_STAT_ERR_FLG                (1 << 6)
#define AD7124_STAT_POR_FLG                (1 << 4)
#define AD7124_STAT_CH_ACTIVE(x)           ((x) & 0xF)

/* ADC_CTRL寄存器位 */
#define AD7124_ADC_CTRL_DOUT_RDY_DEL       (1 << 12)
#define AD7124_ADC_CTRL_CONT_READ          (1 << 11)
#define AD7124_ADC_CTRL_DATA_STAT          (1 << 10)
#define AD7124_ADC_CTRL_CS_EN              (1 << 9)
#define AD7124_ADC_CTRL_REF_EN             (1 << 8)
#define AD7124_ADC_CTRL_PWR_MODE(x)        (((x) & 0x3) << 6)
#define AD7124_ADC_CTRL_MODE(x)            (((x) & 0xF) << 2)
#define AD7124_ADC_CTRL_CLK_SEL(x)         (((x) & 0x3) << 0)

/* IO_Control_1寄存器位 */
#define AD7124_IO_CTRL1_GPIO_DAT2          (1 << 23)
#define AD7124_IO_CTRL1_GPIO_DAT1          (1 << 22)
#define AD7124_IO_CTRL1_GPIO_CTRL2         (1 << 19)
#define AD7124_IO_CTRL1_GPIO_CTRL1         (1 << 18)
#define AD7124_IO_CTRL1_PDSW               (1 << 15)
#define AD7124_IO_CTRL1_IOUT1(x)           (((x) & 0x7) << 11)
#define AD7124_IO_CTRL1_IOUT0(x)           (((x) & 0x7) << 8)
#define AD7124_IO_CTRL1_IOUT_CH1(x)        (((x) & 0xF) << 4)
#define AD7124_IO_CTRL1_IOUT_CH0(x)        (((x) & 0xF) << 0)

/* IO_Control_1 AD7124-8特殊位 */
#define AD7124_8_IO_CTRL1_GPIO_DAT4        (1 << 23)
#define AD7124_8_IO_CTRL1_GPIO_DAT3        (1 << 22)
#define AD7124_8_IO_CTRL1_GPIO_DAT2        (1 << 21)
#define AD7124_8_IO_CTRL1_GPIO_DAT1        (1 << 20)
#define AD7124_8_IO_CTRL1_GPIO_CTRL4       (1 << 19)
#define AD7124_8_IO_CTRL1_GPIO_CTRL3       (1 << 18)
#define AD7124_8_IO_CTRL1_GPIO_CTRL2       (1 << 17)
#define AD7124_8_IO_CTRL1_GPIO_CTRL1       (1 << 16)

/* IO_Control_2寄存器位 */
#define AD7124_IO_CTRL2_GPIO_VBIAS7        (1 << 15)
#define AD7124_IO_CTRL2_GPIO_VBIAS6        (1 << 14)
#define AD7124_IO_CTRL2_GPIO_VBIAS5        (1 << 11)
#define AD7124_IO_CTRL2_GPIO_VBIAS4        (1 << 10)
#define AD7124_IO_CTRL2_GPIO_VBIAS3        (1 << 5)
#define AD7124_IO_CTRL2_GPIO_VBIAS2        (1 << 4)
#define AD7124_IO_CTRL2_GPIO_VBIAS1        (1 << 1)
#define AD7124_IO_CTRL2_GPIO_VBIAS0        (1 << 0)

/* IO_Control_2 AD7124-8特殊位 */
#define AD7124_8_IO_CTRL2_GPIO_VBIAS15     (1 << 15)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS14     (1 << 14)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS13     (1 << 13)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS12     (1 << 12)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS11     (1 << 11)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS10     (1 << 10)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS9      (1 << 9)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS8      (1 << 8)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS7      (1 << 7)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS6      (1 << 6)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS5      (1 << 5)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS4      (1 << 4)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS3      (1 << 3)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS2      (1 << 2)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS1      (1 << 1)
#define AD7124_8_IO_CTRL2_GPIO_VBIAS0      (1 << 0)

/* ID寄存器位 */
#define AD7124_ID_DEVICE_ID(x)             (((x) & 0xF) << 4)
#define AD7124_ID_SILICON_REV(x)           (((x) & 0xF) << 0)

/* 错误寄存器位 */
#define AD7124_ERR_LDO_CAP_ERR             (1 << 19)
#define AD7124_ERR_ADC_CAL_ERR             (1 << 18)
#define AD7124_ERR_ADC_CONV_ERR            (1 << 17)
#define AD7124_ERR_ADC_SAT_ERR             (1 << 16)
#define AD7124_ERR_AINP_OV_ERR             (1 << 15)
#define AD7124_ERR_AINP_UV_ERR             (1 << 14)
#define AD7124_ERR_AINM_OV_ERR             (1 << 13)
#define AD7124_ERR_AINM_UV_ERR             (1 << 12)
#define AD7124_ERR_REF_DET_ERR             (1 << 11)
#define AD7124_ERR_DLDO_PSM_ERR            (1 << 9)
#define AD7124_ERR_ALDO_PSM_ERR            (1 << 7)
#define AD7124_ERR_SPI_IGNORE_ERR          (1 << 6)
#define AD7124_ERR_SPI_SLCK_CNT_ERR        (1 << 5)
#define AD7124_ERR_SPI_READ_ERR            (1 << 4)
#define AD7124_ERR_SPI_WRITE_ERR           (1 << 3)
#define AD7124_ERR_SPI_CRC_ERR             (1 << 2)
#define AD7124_ERR_MM_CRC_ERR              (1 << 1)
#define AD7124_ERR_ROM_CRC_ERR             (1 << 0)

/* 错误使能寄存器位 */
#define AD7124_ERREN_MCLK_CNT_EN           (1 << 22)
#define AD7124_ERREN_LDO_CAP_CHK_TEST_EN   (1 << 21)
#define AD7124_ERREN_LDO_CAP_CHK(x)        (((x) & 0x3) << 19)
#define AD7124_ERREN_ADC_CAL_ERR_EN        (1 << 18)
#define AD7124_ERREN_ADC_CONV_ERR_EN       (1 << 17)
#define AD7124_ERREN_ADC_SAT_ERR_EN        (1 << 16)
#define AD7124_ERREN_AINP_OV_ERR_EN        (1 << 15)
#define AD7124_ERREN_AINP_UV_ERR_EN        (1 << 14)
#define AD7124_ERREN_AINM_OV_ERR_EN        (1 << 13)
#define AD7124_ERREN_AINM_UV_ERR_EN        (1 << 12)
#define AD7124_ERREN_REF_DET_ERR_EN        (1 << 11)
#define AD7124_ERREN_DLDO_PSM_TRIP_TEST_EN (1 << 10)
#define AD7124_ERREN_DLDO_PSM_ERR_ERR      (1 << 9)
#define AD7124_ERREN_ALDO_PSM_TRIP_TEST_EN (1 << 8)
#define AD7124_ERREN_ALDO_PSM_ERR_EN       (1 << 7)
#define AD7124_ERREN_SPI_IGNORE_ERR_EN     (1 << 6)
#define AD7124_ERREN_SPI_SCLK_CNT_ERR_EN   (1 << 5)
#define AD7124_ERREN_SPI_READ_ERR_EN       (1 << 4)
#define AD7124_ERREN_SPI_WRITE_ERR_EN      (1 << 3)
#define AD7124_ERREN_SPI_CRC_ERR_EN        (1 << 2)
#define AD7124_ERREN_MM_CRC_ERR_EN         (1 << 1)
#define AD7124_ERREN_ROM_CRC_ERR_EN        (1 << 0)

/* 0-15通道寄存器位 */
#define AD7124_CH_CH_ENABLE                (1 << 15)
#define AD7124_CH_SETUP(x)                 (((x) & 0x7) << 12)
#define AD7124_CH_AINP(x)                  (((x) & 0x1F) << 5)
#define AD7124_CH_AINM(x)                  (((x) & 0x1F) << 0)

/* 0-7配置寄存器位 */
#define AD7124_CFG_BIPOLAR                 (1 << 11)
#define AD7124_CFG_BURNOUT(x)              (((x) & 0x3) << 9)
#define AD7124_CFG_REF_BUFP                (1 << 8)
#define AD7124_CFG_REF_BUFM                (1 << 7)
#define AD7124_CFG_AIN_BUFP                (1 << 6)
#define AD7124_CFG_AINN_BUFM               (1 << 5)
#define AD7124_CFG_REF_SEL(x)              ((x) & 0x3) << 3
#define AD7124_CFG_PGA(x)                  (((x) & 0x7) << 0)

/* 0-7滤波寄存器位*/
#define AD7124_FILT_FILTER(x)              (((x) & 0x7) << 21)
#define AD7124_FILT_REJ60                  (1 << 20)
#define AD7124_FILT_POST_FILTER(x)         (((x) & 0x7) << 17)
#define AD7124_FILT_SINGLE_CYCLE           (1 << 16)
#define AD7124_FILT_FS(x)                  (((x) & 0x7FF) << 0)

/* 函数定义 */
/* AD7124识别号 */
#define AD7124_4_DEVICE_ID                 0x07
#define AD7124_4_DEVICE_ID_OLD             0x04
#define AD7124_8_DEVICE_ID                 0x17
// #define AD7124_DEVICE_ID_MASK              0xF0

/* 操作模式 */
#define CONTINU_MODE                       0 /* 连续转换模式（默认） */
#define SINGLE_MODE                        1 /* 单次转换模式 */
#define STANDBY_MODE                       2 /* 待机模式 */
#define POWER_DOWN_MODE                    3 /* 省电模式 */
#define IDLE_MODE                          4 /* 睡眠模式 */
#define INTER_OFFSET_CALIB_MODE            5 /* 内部零点（偏移）校准 */
#define INTER_GAIN_CALIB_MODE              6 /* 内部满量程(增益)校准 */
#define SYS_OFFSET_CALIB_MODE              7 /* 系统零店(偏移)校准 */
#define SYS_GAIN_CALIB_MODE                8 /* 系统满量程(增益)校准 */

/* 电源模式选择 */
#define LOW_POWER_MODE                     0 /* 低功率*/
#define MID_POWER_MODE                     1 /* 中功率*/
#define FULL_POWER_MODE                    2 /* 全功率 */

/* 时钟源选择 */
#define INTER_CLK                          0 /* 内部614.4 kHz时钟，此内部时钟不可在CLK引脚上使用 */
#define INTER_OUTPUT_CLK                   1 /* 内部614.4 kHz时钟，此时钟可在CLK引脚上使用 */
#define EXTER_CLK                          2 /* 外部614.4 kHz时钟 */
#define EXTER_DIV4_CLK                     3 /* 外部时钟，此外部时钟在AD7124内除以4 */

/* IOUT励磁电流值 */
#define CURRENT_OFF                        0 /* Off */
#define CURRENT_50UA                       1 /* 50μA */
#define CURRENT_100UA                      2 /* 100μA */
#define CURRENT_250UA                      3 /* 250μA */
#define CURRENT_500UA                      4 /* 500μA */
#define CURRENT_750UA                      5 /* 750μA */
#define CURRENT_1000UA                     6 /* 1000μA */

/* 励磁电流通道选择IOUT */
#define IOUT_CH0                           0 /* IOUT在AIN0引脚上可用 */
#define IOUT_CH1                           1 /* IOUT在AIN1引脚上可用 */
#define IOUT_CH2                           2 /* IOUT在AIN2引脚上可用 */
#define IOUT_CH3                           3 /* IOUT在AIN3引脚上可用 */
#define IOUT_CH4                           4 /* IOUT在AIN4引脚上可用 */
#define IOUT_CH5                           5 /* IOUT在AIN5引脚上可用 */
#define IOUT_CH6                           6 /* IOUT在AIN6引脚上可用 */
#define IOUT_CH7                           7 /* IOUT在AIN7引脚上可用 */

/* 模拟输入选择 */
#define AIN0_INPUT                         0  /* AIN0 */
#define AIN1_INPUT                         1  /* AIN1 */
#define AIN2_INPUT                         2  /* AIN2 */
#define AIN3_INPUT                         3  /* AIN3 */
#define AIN4_INPUT                         4  /* AIN4 */
#define AIN5_INPUT                         5  /* AIN5 */
#define AIN6_INPUT                         6  /* AIN6 */
#define AIN7_INPUT                         7  /* AIN7 */
#define TEMP_INPUT                         16 /* 温度传感器（内部） */
#define AVSS_INPUT                         17 /* AVSS */
#define REF_INPUT                          18 /* 内部参考 */
#define DGND_INPUT                         19 /* DGND */
#define AVDD6P_INPUT                       20 /* (AVDD-AVSS)/6+，与(AVDD-AVSS)/6-配合使用，监测供应AVDD-AVSS */
#define AVDD6M_INPUT                       21 /* (AVDD-AVSS)/6-，与(AVDD-AVSS)/6+配合使用，监测供应AVDD-AVSS */
#define IOVDD6P_INPUT                      22 /* (IOVDD-DGND)/6+，与(IOVDD-DGND)/6-配合使用，监测IOVDD-DGND */
#define IOVDD6M_INPUT                      23 /* (IOVDD-DGND)/6-，与(IOVDD-DGND)/6+配合使用，监测IOVDD-DGND */
#define ALDO6P_INPUT                       24 /* (ALDO-AVSS)/6+，与(ALDO-AVSS)/6-配合使用，以监视模拟LDO */
#define ALDO6M_INPUT                       25 /* (ALDO-AVSS)/6-, 与(ALDO-AVSS)/6+配合使用，以监视模拟LDO */
#define DLDO6P_INPUT                       26 /* (DLDO-DGND)/6+, 与(DLDO-DGND)/6-配合使用，监测数字LDO */
#define DLDO6M_INPUT                       27 /* (DLDO-DGND)/6-, 与(DLDO-DGND)/6+配合使用，监测数字LDO */
#define V20MVP_INPUT                       28 /* V_20MV_P, 与V_20MV_M配合使用，可向ADC施加20mV的p-p信号 */
#define V20MVM_INPUT                       29 /* V_20MV_M，与V_20MV_P配合使用，可向ADC施加20mV的p-p信号 */

/* 增益选择 */
#define PGA1                               0 /* 增益1,  VREF = 2.5V时输入范围:±2.5V */
#define PGA2                               1 /* 增益2,  VREF = 2.5V时输入范围:±1.25V */
#define PGA4                               2 /* 增益4,  VREF = 2.5V时输入范围:± 625mV */
#define PGA8                               3 /* 增益8,  VREF = 2.5V时输入范围:±312.5mV */
#define PGA16                              4 /* 增益16, VREF = 2.5V时输入范围:±156.25mV */
#define PGA32                              5 /* 增益32, VREF = 2.5V时输入范围:±78.125mV */
#define PGA64                              6 /* 增益64, VREF = 2.5V时输入范围:±39.06mV */
#define PGA128                             7 /* 增益128,VREF = 2.5V时输入范围:±19.53mV */

/* 参考源选择 */
#define REF_EXTER_1                        0 /* REFIN1(+)/REFIN1(-). */
#define REF_EXTER_2                        1 /* REFIN2(+)/REFIN2(-). */
#define REF_INTER                          2 /* 内部参考源 */
#define REF_AVDD                           3 /* AVDD */

/* 极性选择 */
#define UNIPOLAR                           0 /* 单极性操作 */
#define BIPOLAR                            1 /* 双极性操作 */

/* 熔断检测电流源选择 */
#define BURNOUT_OFF                        0 /* 熔断电流源关闭（默认） */
#define BURNOUT_500NA                      1 /* 熔断电流源打开, 0.5μA */
#define BURNOUT_2UA                        2 /* 熔断电流源打开, 2μA */
#define BURNOUT_4UA                        3 /* 熔断电流源打开, 4μA */

/* 滤波器类型选择 */
#define SINC4_FILTER                       0 /* sinc4数字滤波器（默认） */
#define SINC3_FILTER                       2 /* sinc3数字滤波器 */
#define SINC4_FAST_FILTER                  4 /* 使用sinc4滤波器的快速沉降滤波器*/
#define SINC3_FAST_FILTER                  5 /* 使用sinc3滤波器的快速沉降滤波器*/
#define POST_FILTER                        7 /* 启用后置滤波器 */

/* 后置滤波器选择 */
#define NO_POST                            0 /* 无后置滤波器(默认) */
#define DB47_POST                          2 /* rejection（带阻）at 50Hz and 60Hz ±1Hz: 47dB,输出数据速率(SPS): 27.27Hz*/
#define DB62_POST                          3 /* rejection（带阻）at 50Hz and 60Hz ±1Hz: 62dB,输出数据速率(SPS): 25Hz */
#define DB86_POST                          5 /* rejection（带阻）at 50Hz and 60Hz ±1Hz: 86dB,输出数据速率(SPS): 20Hz */
#define DB92_POST                          6 /* rejection（带阻）at 50Hz and 60Hz ±1Hz: 92dB,输出数据速率(SPS): 16.7Hz */

/* AD7124基本定义 */
#define AD7124_REF_EXTER1                  3300000ul  /* REFIN1(+) = 3.3 /REFIN1(-) = GND（微伏 */
#define AD7124_REF_INTER                   2500000ul  /* 内部参考（微伏） */
#define AD7124_PGA_GAIN(x)                 (1 << (x)) /* PGA 增益值*/
#define AD7124_ADC_N_BITS                  24
#define AD7124_CH_CNT                      8

/* 时间参数 */
#define AD7124_POR_TIMEOUT                 3000 /* us */
#define AD7124_SPI_RDY_TIMEOUT             3000 /* us */
#define AD7124_POST_DELAY                  5    /* ms */
#define AD7124_POST_RESET_DELAY            4    /* ms */

/* 8个独立的设置项 */
#define SETUP_0                            0
#define SETUP_1                            1
#define SETUP_2                            2
#define SETUP_3                            3
#define SETUP_4                            4
#define SETUP_5                            5
#define SETUP_6                            6
#define SETUP_7                            7
#define SETUP_CNT                          8

/* 通道 */
#define AD7124_CH_0                        0
#define AD7124_CH_1                        1
#define AD7124_CH_2                        2
#define AD7124_CH_3                        3
#define AD7124_CH_4                        4
#define AD7124_CH_5                        5
#define AD7124_CH_6                        6
#define AD7124_CH_7                        7
#define AD7124_CH_8                        8
#define AD7124_CH_9                        9
#define AD7124_CH_10                       10
#define AD7124_CH_11                       11
#define AD7124_CH_12                       12
#define AD7124_CH_13                       13
#define AD7124_CH_14                       14
#define AD7124_CH_15                       15

#define AD_CH_ENABLE                       1
#define AD_CH_DISABLE                      0

/* 内部参考电压使能/不使能 */
#define REF_INTER_ENABLE                   1
#define REF_INTER_DISABLE                  0

#define RD_REGISTER(x)                     ((x & 0x3F) | (1 << 6))
#define WR_REGISTER(x)                     ((x & 0x3F) & ~(1 << 6))

/* 函数: 获取ADC通道的setup设置
   返回: 通道配置中setup字段的值. */
#define get_setup_number(aux)              ((aux >> 12) & 0x07)
/* 函数: 获取ADC通道的PGA设置
   返回: 设置ADC通道时PGA字段的值 */
#define get_channel_pga(aux)               (1 << (aux & 0x07))

/* 函数:获取ADC通道的双极性设置
   返回: 在ADC通道的设置中双极型的值 */
#define get_channel_bipolar(aux)           ((aux >> 11) & 0x01)

typedef hq_packed struct {
    uint8_t  ctrl_pwr_mode;  /* ADC_CTRL 功率模式 */
    uint8_t  ctrl_work_mode; /* ADC_CTRL 工作模式 */
    uint8_t  ctrl_clk_src;   /* ADC_CTRL 时钟源选择 */
    uint8_t  pga;            /* PGA */
    uint32_t offs0;          /* OFFS0 */
    uint32_t offs2;          /* OFFS2 */
    uint8_t  rsvd[4];        /* RVSD */
} ad7124_para_t;

/* 设备寄存器信息 */
typedef struct {
    uint8_t addr;
    int32_t value;
    int32_t size;
    int32_t rw;
} ad7124_reg_t;

typedef struct {
    uint32_t        port;   // GPIO端口地址
    uint32_t        pin;    // GPIO引脚编号
    rcu_periph_enum clock;  // 时钟使能宏
    uint32_t        mode;   // GPIO模式
    uint32_t        pupd;   // 上拉/下拉设置
    uint8_t         otype;  // 输出类型
    uint32_t        ospeed; // 输出速度
} ad7124_spi_pin_cfg_t;

/*
   SPIx : SPI2  SCK=PC10  MOSI=PC12  MISO=PC11
   CS   : PE7
*/
#define AD7124_SPI_DEV     SPI2
#define AD7124_SPI_CLK_DIV 5 /* CLK_APB1=50MHz  50MHz/2^7=0.390625MHz */
#define AD7124_SPI_MODE    3

#define AD7124_PIN_PORT    GPIOC
#define AD7124_PIN_RCU     RCU_GPIOC
#define AD7124_PIN_AF      GPIO_AF_6

#define AD7124_SCK_PIN     GPIO_PIN_10
#define AD7124_MISO_PIN    GPIO_PIN_11
#define AD7124_MOSI_PIN    GPIO_PIN_12

#define AD7124_NSS_RCU     RCU_GPIOE
#define AD7124_NSS_PORT    GPIOE
#define AD7124_NSS_PIN     GPIO_PIN_7

#define ad7124_init_pin()                                                                                                                            \
    {                                                                                                                                                \
        rcu_periph_clock_enable(AD7124_PIN_RCU);                                                                                                     \
        rcu_periph_clock_enable(AD7124_NSS_RCU);                                                                                                     \
        gd32_spi_init(AD7124_SPI_DEV, SPI_MASTER, AD7124_SPI_CLK_DIV, AD7124_SPI_MODE, 8);                                                           \
        gpio_af_set(AD7124_PIN_PORT, AD7124_PIN_AF, AD7124_SCK_PIN | AD7124_MISO_PIN | AD7124_MOSI_PIN);                                             \
        gpio_mode_set(AD7124_PIN_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, AD7124_SCK_PIN | AD7124_MISO_PIN);                                              \
        gpio_mode_set(AD7124_PIN_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, AD7124_MOSI_PIN);                                                             \
        gpio_output_options_set(AD7124_PIN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AD7124_SCK_PIN | AD7124_MISO_PIN | AD7124_MOSI_PIN);              \
        gpio_mode_set(AD7124_NSS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, AD7124_NSS_PIN);                                                            \
        gpio_output_options_set(AD7124_NSS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AD7124_NSS_PIN);                                                  \
    }

/* MISO引脚拉低时表示转换完成 */
#define ad7124_covert_done() gpio_input_bit_get(AD7124_PIN_PORT, AD7124_MISO_PIN)

#define ad7124_low_cs()      gpio_bit_reset(AD7124_NSS_PORT, AD7124_NSS_PIN)
#define ad7124_high_cs()     gpio_bit_set(AD7124_NSS_PORT, AD7124_NSS_PIN)

extern const ad7124_reg_t          AD7124_DEF_REG_CFG0[AD7124_REG_CNT];
extern ad7124_reg_t                ad7124_reg_cfg0[AD7124_REG_CNT];
extern struct ad7124_dev_t         ad7124_dev0;
extern const ad7124_spi_pin_cfg_t *ad7124_pin_cfg;

int   ad7124_init(ad7124_para_t para);
void  ad7124_pin_init(void);
int   ad7124_set_control(uint8_t op_mode, uint8_t pwr_mode, uint8_t ref_en, uint8_t clk_sel);
int   ad7124_set_config(uint8_t cfg, uint8_t ref, uint8_t pga, uint8_t polar, uint8_t burnout);
int   ad7124_enable_channel(uint8_t ch, uint8_t en);
int   ad7124_set_channel(uint8_t ch, uint8_t cfg, uint8_t ainp, uint8_t ainm, uint8_t enable);
int   ad7124_set_mode(int8_t op_mode);
int   ad7124_set_pga(uint8_t cfg, uint8_t pga);
int   ad7124_set_offset(uint8_t cfg, uint32_t ofs);
int   ad7124_conv_ready(void);
int   ad7124_read_data(uint32_t *data);
float ad7124_raw_to_vol(uint8_t ch, uint32_t val, uint8_t *err);

#endif /* _AD7124_H_ */
