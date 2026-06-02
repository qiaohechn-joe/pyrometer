/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_exmc.c
 * 文件描述：外部存储控制器（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           v1.0 2024/07/02 张晓博 初始版本
 *           v1.1 2024/11/18 张晓博 增加psram
 */

#include "gd32_exmc.h"
#include "gd32_ll.h"

/* 定义 SDRAM 模式寄存器的内容 */
/* 突发长度 */
#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0003)

/* 突发访问的地址模式 */
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)

/* 列地址选通延迟(CAS Latency) */
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)

/* 写模式 */
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)

#define SDRAM_TIMEOUT                            ((uint32_t)0x0000FFFF)

/*
 * 功能描述：sdram外设初始化
 * 入参说明：无
 * 返 回 值：0 --- 成功；其他 --- 失败
 */
int sdram_init(void) {
    uint32_t timeout      = SDRAM_TIMEOUT;
    uint32_t mode_content = 0;

    rcu_periph_clock_enable(RCU_EXMC);

    exmc_sdram_parameter_struct         sdram_init_struct;         // 参数
    exmc_sdram_timing_parameter_struct  sdram_timing_init_struct;  // 时序
    exmc_sdram_command_parameter_struct sdram_command_init_struct; // 命令

    /*配置时序寄存器*/
    sdram_timing_init_struct.load_mode_register_delay = 2; /* 加载模式寄存器延迟为2个时钟周期 */
    sdram_timing_init_struct.exit_selfrefresh_delay   = 8; /* 退出自刷新的延迟为8个时钟周期 */
    sdram_timing_init_struct.row_address_select_delay = 5; /* 行地址选择延迟为5个时钟周期 */
    sdram_timing_init_struct.auto_refresh_delay       = 7; /* 自动刷新延迟为7个时钟周期 */
    sdram_timing_init_struct.write_recovery_delay     = 2; /* 写恢复延迟为2个时钟周期 */
    sdram_timing_init_struct.row_precharge_delay      = 3; /* 行预充电延迟为3个时钟周期 */
    sdram_timing_init_struct.row_to_column_delay      = 3; /* 行至列的延迟为3个时钟周期 */

    /* 配置控制寄存器*/
    sdram_init_struct.sdram_device         = EXMC_SDRAM_DEVICE0;           /* 选择EXMC SDRAM devicex */
    sdram_init_struct.column_address_width = EXMC_SDRAM_COW_ADDRESS_9;     /* 9位列地址位宽 */
    sdram_init_struct.row_address_width    = EXMC_SDRAM_ROW_ADDRESS_13;    /* 13位行地址位宽 */
    sdram_init_struct.data_width           = EXMC_SDRAM_DATABUS_WIDTH_16B; /* 16位SDRAM数据总线宽度 */
    sdram_init_struct.internal_bank_number = EXMC_SDRAM_4_INTER_BANK;      /* 4个内部Banks */
    sdram_init_struct.cas_latency          = EXMC_CAS_LATENCY_3_SDCLK;     /* CAS延迟为3个时钟周期 */
    sdram_init_struct.write_protection     = DISABLE;                      /* 禁用写保护，允许写访问 */
    sdram_init_struct.sdclock_config       = EXMC_SDCLK_PERIODS_3_HCLK;    /* SDRAM时钟周期为2个HCLK SDCLK=HCLK/2=240M/2=120M=8.3ns */
    sdram_init_struct.burst_read_switch    = ENABLE;                       /* 使能突发读 */
    sdram_init_struct.pipeline_read_delay  = EXMC_PIPELINE_DELAY_2_HCLK;   /* 流水线读数据延迟2个HCLK周期  */
    sdram_init_struct.timing               = &sdram_timing_init_struct;    /* 读写时序参数 */

    exmc_sdram_init(&sdram_init_struct);

    /* 时钟配置使能*/
    sdram_command_init_struct.command               = EXMC_SDRAM_CLOCK_ENABLE;
    sdram_command_init_struct.bank_select           = EXMC_SDRAM_DEVICE0_SELECT;
    sdram_command_init_struct.auto_refresh_number   = EXMC_SDRAM_AUTO_REFLESH_2_SDCLK;
    sdram_command_init_struct.mode_register_content = 0;

    /* 等待SDRAM控制器就绪 */
    while ((exmc_flag_get(EXMC_SDRAM_DEVICE0, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if (0 == timeout) {
        return -1;
    }
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* 延时10ms*/
    gd32_delay_ms(10);

    /*对所有预存储区充电*/
    sdram_command_init_struct.command               = EXMC_SDRAM_PRECHARGE_ALL;
    sdram_command_init_struct.bank_select           = EXMC_SDRAM_DEVICE0_SELECT;
    sdram_command_init_struct.auto_refresh_number   = EXMC_SDRAM_AUTO_REFLESH_2_SDCLK;
    sdram_command_init_struct.mode_register_content = 0;

    /* 等待SDRAM控制器就绪 */
    timeout = SDRAM_TIMEOUT;
    while ((exmc_flag_get(EXMC_SDRAM_DEVICE0, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if (0 == timeout) {
        return -1;
    }
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* 刷新配置*/
    sdram_command_init_struct.command               = EXMC_SDRAM_AUTO_REFRESH;
    sdram_command_init_struct.bank_select           = EXMC_SDRAM_DEVICE0_SELECT;
    sdram_command_init_struct.auto_refresh_number   = EXMC_SDRAM_AUTO_REFLESH_9_SDCLK;
    sdram_command_init_struct.mode_register_content = 0;

    /* 等待SDRAM控制器就绪 */
    timeout = SDRAM_TIMEOUT;
    while ((exmc_flag_get(EXMC_SDRAM_DEVICE0, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if (0 == timeout) {
        return -1;
    }
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* 模式配置 */
    mode_content = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1 | SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL | SDRAM_MODEREG_CAS_LATENCY_3 |
                   SDRAM_MODEREG_OPERATING_MODE_STANDARD | SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    sdram_command_init_struct.command               = EXMC_SDRAM_LOAD_MODE_REGISTER;
    sdram_command_init_struct.bank_select           = EXMC_SDRAM_DEVICE0_SELECT;
    sdram_command_init_struct.auto_refresh_number   = EXMC_SDRAM_AUTO_REFLESH_2_SDCLK;
    sdram_command_init_struct.mode_register_content = mode_content;

    /* 等待SDRAM控制器就绪 */
    timeout = SDRAM_TIMEOUT;
    while ((exmc_flag_get(EXMC_SDRAM_DEVICE0, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if (0 == timeout) {
        return -1;
    }
    exmc_sdram_command_config(&sdram_command_init_struct);

    /*
     * 配置自动刷新间隔,计算方法如下:
     * w9825g6kh-6i的刷新周期为64ms,总行数为8192(2^13)，所以每行的刷新速率为64ms/8192=7.813us
     * SDRAM的时钟频率为=系统时钟/2=240Mhz/2=120Mhz
     * 所以,count=7.813us/(1/120Mhz)-20=7.813us*120MHz-20=918
     */
    exmc_sdram_refresh_count_set(918);

    /* 等待SDRAM控制器就绪 */
    timeout = SDRAM_TIMEOUT;
    while ((exmc_flag_get(EXMC_SDRAM_DEVICE0, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if (0 == timeout) {
        return -1;
    }
    return 0;
}

/*
 * 功能描述：sqpipsram外设初始化
 * 入参说明：无
 * 返 回 值：0 --- 成功；其他 --- 失败
 */
void sqpipsram_init(void) {
    rcu_periph_clock_enable(RCU_EXMC);

    exmc_norsram_timing_parameter_struct norsram_timing; // EXMC NOR/SRAM 时序
    exmc_norsram_parameter_struct        norsram_init;   // EXMC NOR/SRAM 参数

    exmc_norsram_deinit(EXMC_BANK0_NORSRAM_REGION0);

    norsram_timing.syn_clk_division       = EXMC_SYN_CLOCK_RATIO_2_CLK;
    norsram_timing.asyn_access_mode       = EXMC_ACCESS_MODE_A;
    norsram_timing.syn_data_latency       = EXMC_DATALAT_2_CLK;
    norsram_timing.bus_latency            = 1;
    norsram_timing.asyn_data_setuptime    = 5;
    norsram_timing.asyn_address_holdtime  = 2;
    norsram_timing.asyn_address_setuptime = 2;

    /* configure EXMC bus parameters */
    norsram_init.norsram_region    = EXMC_BANK0_NORSRAM_REGION0;
    norsram_init.write_mode        = EXMC_SYN_WRITE;
    norsram_init.extended_mode     = DISABLE;
    norsram_init.asyn_wait         = DISABLE;
    norsram_init.nwait_signal      = DISABLE;
    norsram_init.memory_write      = ENABLE;
    norsram_init.nwait_config      = EXMC_NWAIT_CONFIG_BEFORE;
    norsram_init.wrap_burst_mode   = DISABLE;
    norsram_init.nwait_polarity    = EXMC_NWAIT_POLARITY_LOW;
    norsram_init.burst_mode        = ENABLE;
    norsram_init.databus_width     = EXMC_NOR_DATABUS_WIDTH_8B;
    norsram_init.memory_type       = EXMC_MEMORY_TYPE_PSRAM;
    norsram_init.address_data_mux  = DISABLE;
    norsram_init.read_write_timing = &norsram_timing;
    norsram_init.write_timing      = &norsram_timing;

    exmc_norsram_init(&norsram_init);
    exmc_norsram_page_size_config(EXMC_BANK0_NORSRAM_REGION0, EXMC_CRAM_PAGE_SIZE_1024_BYTES);
    exmc_norsram_consecutive_clock_config(EXMC_CLOCK_SYN_MODE);
    exmc_norsram_enable(EXMC_BANK0_NORSRAM_REGION0);

    exmc_sqpipsram_parameter_struct psram_init; // EXMC的SQPI通信模式结构体，仅适用BANK0的REGION0

    psram_init.sample_polarity = EXMC_SQPIPSRAM_SAMPLE_RISING_EDGE;
    psram_init.id_length       = EXMC_SQPIPSRAM_ID_LENGTH_64B;
    psram_init.address_bits    = EXMC_SQPIPSRAM_ADDR_LENGTH_24B;
    psram_init.command_bits    = EXMC_SQPIPSRAM_COMMAND_LENGTH_8B;
    exmc_sqpipsram_init(&psram_init);

    /* 等待EXMC操作空闲*/
    while (exmc_sqpipsram_send_command_state_get(EXMC_SEND_COMMAND_FLAG_SC));
    while (exmc_sqpipsram_send_command_state_get(EXMC_SEND_COMMAND_FLAG_RDID));
}

/*
 * 功能描述：sqpipsram写设置
 * 入参说明：write_cmd_mode  --- 写模式，取值如下：
 *                                  EXMC_SQPIPSRAM_WRITE_MODE_DISABLE: not SPI mode
                                    EXMC_SQPIPSRAM_WRITE_MODE_SPI: SPI mode
                                    EXMC_SQPIPSRAM_WRITE_MODE_SQPI: SQPI mode
                                    EXMC_SQPIPSRAM_WRITE_MODE_QPI: QPI mode
 *          write_cmd_code  --- 命令码
 *          write_wait_cycle--- 写等待周期（0~15）
 *          is_special_cmd  --- 是否是特殊命令，0：否，其他：是
 * 返 回 值：无
 */
void sqpipsram_write_set(uint32_t write_cmd_mode, uint32_t write_wait_cycle, uint32_t write_cmd_code, uint8_t is_special_cmd) {
    exmc_sqpipsram_write_command_set(write_cmd_mode, write_wait_cycle, write_cmd_code);

    if (is_special_cmd) {
        exmc_sqpipsram_write_cmd_send();
        while (exmc_sqpipsram_send_command_state_get(EXMC_SEND_COMMAND_FLAG_SC));
    }
}

/*
 * 功能描述：sqpipsram读设置
 * 入参说明：read_cmd_mode --- 读模式,取值如下：
 *                                  EXMC_SQPIPSRAM_READ_MODE_DISABLE: not SPI mode
                                    EXMC_SQPIPSRAM_READ_MODE_SPI: SPI mode
                                    EXMC_SQPIPSRAM_READ_MODE_SQPI: SQPI mode
                                    EXMC_SQPIPSRAM_READ_MODE_QPI: QPI mode
 *          read_cmd_code   --- 命令码
 *          read_wait_cycle --- 写等待周期（0~15）
 *          is_special_cmd  --- 是否是特殊命令，0：否，其他：是
 * 返 回 值：无
 */
void sqpipsram_read_set(uint32_t read_cmd_mode, uint32_t read_wait_cycle, uint32_t read_cmd_code, uint8_t is_special_cmd) {
    exmc_sqpipsram_read_command_set(read_cmd_mode, read_wait_cycle, read_cmd_code);

    if (is_special_cmd) {
        exmc_sqpipsram_read_id_command_send();
        while (exmc_sqpipsram_send_command_state_get(EXMC_SEND_COMMAND_FLAG_RDID));
    }
}
