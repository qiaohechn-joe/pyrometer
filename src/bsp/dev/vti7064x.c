/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：vti7064x.c
 * 文件描述：vti7064x(psram)驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：64Mbit
 * 更新记录：
 *           V1.0 2024/11/18  张晓博 初始版本
 */

#include "vti7064x.h"

#define VTI7064X_SPI_READ           0x03 // max frequency 33MHz
#define VTI7064X_SPI_FAST_READ      0x0B // max frequency 104MHz
#define VTI7064X_SPI_FAST_QUAD_READ 0xEB // max frequency 104Mhz

#define VTI7064X_SPI_WRITE          0x02
#define VTI7064X_QPI_WRITE          0x38

#define VTI7064X_QUAD_MODE_ENTER    0x35
#define VTI7064X_QUAD_MODE_EXIT     0xF5

#define VTI7064X_READ_ID            0x9F

#define VTI7064X_ENABLE_RESET       0x66
#define VTI7064X_RESET              0x99

#define VTI7064X_ID                 0x0D5D

#define VTI7064X_SPECIAL_CMD        0x01
#define VTI7064X_NORMAL_CMD         !VTI7064X_SPECIAL_CMD

#define PSRAM_TEST_ADDR             0x1000
#define PSRAM_TEST_LEN              100

static uint8_t psram_test_buff[4] __attribute__((at(VTI7064_START_ADDR)));
static uint8_t psram_write_buff[PSRAM_TEST_LEN];
static uint8_t psram_read_buff[PSRAM_TEST_LEN];

/*
 * 功能说明: vti7064复位
 * 入参说明: 无
 * 返 回 值：无
 */
static void vti7064x_reset(void) {
    sqpipsram_write_set(EXMC_SQPIPSRAM_WRITE_MODE_SPI, 0, VTI7064X_ENABLE_RESET, VTI7064X_SPECIAL_CMD);
    sqpipsram_write_set(EXMC_SQPIPSRAM_WRITE_MODE_SPI, 0, VTI7064X_RESET, VTI7064X_SPECIAL_CMD);
}

/*
 * 功能说明: vti7064读取id
 * 入参说明: 无
 * 返 回 值：64位id
 */
static uint64_t vti7064x_read_id(void) {
    uint64_t vti7064x_id = 0;

    sqpipsram_read_set(EXMC_SQPIPSRAM_READ_MODE_SPI, 1, VTI7064X_READ_ID, VTI7064X_SPECIAL_CMD);
    vti7064x_id = exmc_sqpipsram_high_id_get();
    vti7064x_id <<= 32;
    vti7064x_id |= exmc_sqpipsram_low_id_get();

    return vti7064x_id;
}

/*
 * 功能说明: 从PSRAM指定地址开始,连续读取n个字节
 * 入参说明: addr---开始读取的地址
 *          buff---数据缓冲区的指针
 *          len--- 读取的数据长度
 * 返 回 值： 无
 */
static void vti7064x_read_data(uint32_t addr, uint8_t *buff, uint32_t len) {
    if ((addr + len) > VTI7064_TOTAL_SIZE) {
        return;
    }

    for (; len != 0; len--) {
        *buff++ = *(uint8_t *)(VTI7064_START_ADDR + addr);
        addr++;
    }
}

/*
 * 功能说明: 从PSRAM指定地址开始,连续写入n个字节
 * 入参说明: addr---开始读取的地址
 *          buff---数据缓冲区的指针
 *          len--- 读取的数据长度
 * 返 回 值： 无
 */
static void vti7064x_write_data(uint32_t addr, uint8_t *buff, uint32_t len) {
    if ((addr + len) > VTI7064_TOTAL_SIZE) {
        return;
    }

    for (; len != 0; len--) {
        *(uint8_t *)(VTI7064_START_ADDR + addr) = *buff;
        addr++;
        buff++;
    }
}

/*
 *  功能说明: 扫描测试psram
 *  入参说明：无
 *  返 回 值: 0 --- 成功；其他 --- 失败
 */
int vti7064x_test(void) {
    uint32_t  i;
    uint32_t *paddr;
    uint8_t  *pbytes;
    uint32_t  err;

    /* 写psram */
    paddr = (uint32_t *)VTI7064_START_ADDR;
    for (i = 0; i < VTI7064_TOTAL_SIZE / 4; i++) {
        *paddr++ = i;
    }

    /* 读psram */
    err   = 0;
    paddr = (uint32_t *)VTI7064_START_ADDR;
    for (i = 0; i < VTI7064_TOTAL_SIZE / 4; i++) {
        if (*paddr++ != i) {
            err++;
        }
    }

    if (err > 0) {
        return (4 * err);
    }

    /* 对psram的数据求反并写入 */
    paddr = (uint32_t *)VTI7064_START_ADDR;
    for (i = 0; i < VTI7064_TOTAL_SIZE / 4; i++) {
        *paddr = ~*paddr;
        paddr++;
    }

    /* 再次比较psram的数据 */
    err   = 0;
    paddr = (uint32_t *)VTI7064_START_ADDR;
    for (i = 0; i < VTI7064_TOTAL_SIZE / 4; i++) {
        if (*paddr++ != (~i)) {
            err++;
        }
    }

    if (err > 0) {
        return (4 * err);
    }

    mem_set(psram_test_buff, sizeof(psram_test_buff), 0x5A);

    /* 比较psram的数据 */
    err    = 0;
    pbytes = (uint8_t *)VTI7064_START_ADDR;
    for (i = 0; i < sizeof(psram_test_buff); i++) {
        if (*pbytes++ != 0x5A) {
            err++;
        }
    }
    if (err > 0) {
        return err;
    }

    /*函数读写测试*/
    mem_set(psram_write_buff, PSRAM_TEST_LEN, 0);
    mem_set(psram_read_buff, PSRAM_TEST_LEN, 0);

    for (i = 0; i < PSRAM_TEST_LEN; i++) {
        psram_write_buff[i] = i;
    }

    vti7064x_write_data(PSRAM_TEST_ADDR, psram_write_buff, PSRAM_TEST_LEN);
    vti7064x_read_data(PSRAM_TEST_ADDR, psram_read_buff, PSRAM_TEST_LEN);

    err = 0;
    for (i = 0; i < PSRAM_TEST_LEN; i++) {
        if (psram_write_buff[i] != psram_read_buff[i]) {
            err++;
        }
    }
    if (err > 0) {
        return err;
    }
    return 0;
}

/*
 * 功能说明: vti7064 初始化
 * 入参说明: 无
 * 返 回 值：1：成功，其他：失败
 */
int vti7064x_init(void) {
    uint8_t ret = 0;
    vti7064x_pin_init();

    sqpipsram_init();

    vti7064x_reset(); /*复位之后在spi模式*/

    gd32_delay_ms(10);

    if (VTI7064X_ID == (vti7064x_read_id() >> 48)) {
        sqpipsram_write_set(EXMC_SQPIPSRAM_WRITE_MODE_SPI, 0, VTI7064X_SPI_WRITE, 0);   /*spi 104MHz 读模式*/
        sqpipsram_read_set(EXMC_SQPIPSRAM_READ_MODE_SPI, 9, VTI7064X_SPI_FAST_READ, 0); /*spi 写模式*/
        ret = 1;
    } else {
        sys_state.dev_error[DEV_ERR_PSRAM] = DEV_ST_ERR;
        dbg_info("%s", "PSRAM(vti7064x) is not found!\r\n");
    }

    return ret;
}
