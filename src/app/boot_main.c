/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：boot_main.c
 * 文件描述：主程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军    初始版本
 *           V1.1 2024/05/30 陈军    MCU内部FLASH操作加入 gd32_fmc 文件
 *           V1.2 2025/01/07 李兆越  boot适配新升级协议
 *           V1.3 2025/01/08 李兆越  无效固件判断优化
 *           V1.4 2025/03/28 陈军    更新启动串口配置获取函数 powerup_uart_para_get
 */

#include "platform.h"
#include "upgrade.h"

#define FW_BUILT "built at " __TIME__ " " __DATE__

/* 固件头信息 */
/* 包缓存 */
static uint32_t buffer[1024];
/* APP入口函数指针 */
typedef void (*app_entry_t)(void);

/* 内部函数 */
static void    upgrade(void);
static void    start_app(void);
static void    boot_startup_info(void);
extern uint8_t put_port;
/* 外部函数 */
int powerup_uart_para_get(port_para_t *para);

/*
 * 功能描述：主函数
 * 入参说明：无
 * 返 回 值：无
 */
int main(void) {
    /* LL层初始化 */
    gd32_ll_init();
    boot_startup_info();
    upgrade();
    start_app();
}

/*
 * 功能描述：这个函数用于固件升级
 * 入参说明：无
 * 返 回 值：无
 */
static void upgrade(void) {
    uint32_t img_addr;
    uint32_t app_addr;
    uint32_t len;
    uint32_t size;

    rcu_periph_clock_enable(RCU_CRC);

    dbg_upgrade("Check image...\r\n");

    /* 加载固件头 */
    fw_info_t fw_info;
    mem_copy(&fw_info, (const void *)FW_INFO_ADDR, sizeof(fw_info));

    /* 检查固件标头的CRC32 */
    uint32_t fw_info_check = calculate_crc32((uint8_t *)&fw_info, sizeof(fw_info) - 4, ENDIAN_LITTLE);

    /* 无效固件 */
    if ((fw_info_check != fw_info.crc32)) { /* 固件信息校验失败 */
        dbg_upgrade("Image is invalid!\r\n");
        dbg_upgrade("crc err :check = %08X,fw_header.check_crc32 = %08X\r\n", fw_info_check, fw_info.crc32);
        return;
    }
    if (fw_info.img_size == 0) { /* 文件大小为0 */
        dbg_upgrade("img_size err :img_size = 0\r\n");
        return;
    }
    if (fw_info.status != UG_STATE_CHECK_PASS) {
        return; // 无效bin文件
    }
    /* 检查固包的CRC32 */
    img_addr           = FW_DATA_ADDR;
    len                = fw_info.img_size;
    uint32_t img_crc32 = calculate_crc32((uint8_t *)img_addr, len, ENDIAN_BIG);
    if (img_crc32 != fw_info.img_crc32) {
        dbg_upgrade("Image crc32 is not correct!\r\n");
        return;
    }

    /* 更新APP */
    dbg_upgrade("Update APP");
    fmc_unlock();

    fmc_sector_erase(APP_ENTRY_SECT_1);
    dbg_upgrade("-");
    fmc_sector_erase(APP_ENTRY_SECT_2);
    dbg_upgrade("-");

    img_addr = FW_DATA_ADDR;
    app_addr = APP_ENTRY_ADDR;
    len      = fw_info.img_size;
    while (len != 0) {
        size = (len < sizeof(buffer)) ? len : sizeof(buffer);
        mem_copy(buffer, (const void *)img_addr, size);
        gdf_write_data_by_word(app_addr, size, buffer);
        img_addr += size;
        app_addr += size;
        len -= size;
        dbg_upgrade(".");
    }

    fmc_lock();

    /* 检查APP */
    dbg_upgrade("\r\nCheck APP...");
    app_addr = APP_ENTRY_ADDR;
    len      = fw_info.img_size;
    crc_data_register_reset();
    uint32_t crc32 = calculate_crc32((uint8_t *)app_addr, len, ENDIAN_BIG);
    if (crc32 == fw_info.img_crc32) { /* APP正确 */
        dbg_upgrade("OK!\r\n");

        /* 清除固件标头 */
        fw_info.status = UG_STATE_FINISHED;
        gdf_write_data_by_page(FW_INFO_ADDR, sizeof(fw_info), (uint8_t *)&fw_info);
    } else { /* 不正确 */
        dbg_upgrade("ERROR!\r\n");
        gd32_delay_ms(500);
        NVIC_SystemReset();
    }
}

/*
 * 功能描述：启动应用程序
 * 入参说明：无
 * 返 回 值：无
 * 备    注：
             MSP：主堆栈指针（Main Stack Pointer, MSP）
             地址 0x08010000 至 0x08010003：存储主堆栈指针（MSP）的初始化值。
             地址 0x08010004 至 0x08010007：存储复位向量（即应用程序入口地址）。
 */
static void start_app(void) {
    app_entry_t app_entry;
    uint32_t    msp;

    dbg_info("\r\nAPP Start......");
    gd32_delay_ms(1);
    usart_disable(LIB_IO_DEV); /* 防止错误输出 */

    msp       = *(volatile uint32_t *)APP_ENTRY_ADDR;
    app_entry = (app_entry_t)(*(volatile uint32_t *)(APP_ENTRY_ADDR + 4));

    SCB->VTOR = APP_ENTRY_ADDR; /* 设置向量表偏移 */
    __set_MSP(msp);             /* 设置MSP */
    app_entry();                /* 跳转到应用程序 */
}

/*
 * 功能描述：这个函数用于在上电时获取串口参数
 * 入参说明：无
 * 返 回 值：0 --- 成功，其它 --- 失败
 * 备    注：
 */
int powerup_uart_para_get(port_para_t *para) {
    uint32_t   offset;
    sys_para_t bsp_para;

    /* 从GD-FLASH读取串口参数 */
    offset = offset_of(port, sys_para_t);
    mem_copy(&bsp_para, (const void *)USER_PARA_ADDR, sizeof(bsp_para));

    if (bsp_para.crc[PARA_PORT] != (uint16_t)~crc16_RTU((uint8_t *)&bsp_para + offset, sizeof(port_para_t))) return -1;

    para->mode     = bsp_para.port.mode;
    para->baudrate = bsp_para.port.baudrate;
    para->dbg_port = bsp_para.port.dbg_port;
    mem_copy(para->dbg_level, bsp_para.port.dbg_level, sizeof(para->dbg_level));

    return 0;
}

/*
 * 功能描述：保障 Perf_counter 正常编译通过，app则在 port.c 中定义
 * 入参说明：无
 * 返 回 值：0 --- 成功，其它 --- 失败
 * 备    注：
 */
__attribute__((used)) void SysTick_Handler(void) {
}

/*
 * 功能描述：这个函数配置和打印设备上电信息
 * 入参说明：无
 * 返 回 值：无
 */
static void boot_startup_info(void) {
    port_para_t para;

    /* 配置调试串口 */
    if (powerup_uart_para_get(&para)) {
        dbg_init(DBG_LEVEL_INFO, LIB_IO_RATE, "8N1");
    } else {
        put_port = para.dbg_port;
        dbg_init(para.dbg_level, para.baudrate, "8N1");
    }

    dbg_upgrade(BOOT_FW_DESC);
    dbg_upgrade("\r\n");
    dbg_upgrade(BOOT_FW_VER);
    dbg_upgrade(FW_BUILT);
    dbg_upgrade("\r\n");
}
