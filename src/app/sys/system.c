/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：system.c
 * 文件描述：系统层管理业务代码（初始化，系统维护，关键业务等）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军    初始版本
 *           V1.1 2024/10/11 李兆越  加入复位监测和供电状态监测模块
 */

#include "agent.h"
#include "ostimer.h"
#include "storage.h"
#include "system.h"
#include "vti7064x.h"
#include "camera.h"
#include "record.h"
#include "lwip.h"
#include "ui.h"
#include "display.h"
#include "burst.h"
#include "measure.h"
#include "analogout.h"
#include "alarm.h"
#include "upgrade.h"
#include "mail_box.h"
#include "drv_usb_hw.h"
#include "heater.h"
#include "self_check.h"

/* 密码仓库 */
uint32_t PWD_TAB[PWD_CNT] = {80295, 80296};

/* 版本初始化 */
const sys_version_t SYS_VERSION = {
    .hw   = make_version(HW_VER_MAJOR, HW_VER_MINOR, HW_VER_SERIAL),
    .fw   = make_version(FW_VER_MAJOR, FW_VER_MINOR, FW_VER_SERIAL),
    .boot = make_version(BOOT_VER_MAJOR, BOOT_VER_MINOR, BOOT_VER_SERIAL),
};

/* 固件信息初始化（存储到bin文件指定偏移地址） */
const firmware_info_t FIRMWARE_INFO __attribute__((at(FIRMWARE_INFO_ADDR))) = {
    .ver =
        {
            /* 位置保持不动，影响upgrade */
            .boot = make_version(BOOT_VER_MAJOR, BOOT_VER_MINOR, BOOT_VER_SERIAL),
            .hw   = make_version(HW_VER_MAJOR, HW_VER_MINOR, HW_VER_SERIAL),
            .fw   = make_version(FW_VER_MAJOR, FW_VER_MINOR, FW_VER_SERIAL),
        },
    .describe = SPECIAL_NAME,
};

/* 冷/热启动变量 */
static uint32_t start_flag __attribute__((section("NO_INIT"), zero_init));
int             warm_start;

/* LTC2606和OV5640共用互斥锁 */
static SemaphoreHandle_t dac_camera_mutex;

/* 内部函数 */
static void start_task(void *arg);
static void service_task(void *arg);
static void dev_start_type(void);
static void lvd_init(void);

int64_t lCycles = 0;

/*
 * 功能描述：这个函数用于初始化系统
 * 入参说明：无
 * 返 回 值：无
 */
void hq_sys_init(void) {
    rtc_time_t time;

    /* 数据结构 */
    // mem_set(&sys_state, sizeof(sys_state), 0);
    mem_set(sys_state.dev_error, sizeof(sys_state.dev_error), '0');

    /* 系统功能模块初始化 */
    //    dac_camera_mutex = xSemaphoreCreateMutex();
    //    if (dac_camera_mutex == NULL) {
    //        dbg_info("Create dac and camera mutex failed!\n");
    //    }

#if SUPPORT_EEPROM
    eeprom_wp_init();
    eeprom_wp_lock();
#endif /* SUPPORT_EEPROM */
    para_init();
#if IS_USE_SELF_CHECK
    /*日志输出自检信息的时候开启自检功能*/
    dev_self_check();
#endif
    Extern_Triger_GPIO_config();
    /* 设备启动类型 */
    dev_start_type();
    // #if SUPPORT_EEPROM
    //     powerup_quick_save_get(); /* 获取掉电快速保存参数 */
    // #endif
    lvd_init();
    test_pin_init();

    anolog_out_init();
    temp_init();
    //    vti7064x_init();
    //    camera_init();
    ui_init();
    heater_init();

#if SUPPORT_EEPROM
    record_init();
#endif

    burst_init();
    alarm_init();

    tcpsvr_init();     /* net_step1 服务器及ip初始化 */
    lwip_stack_init(); /* net_step2 Lwip初始化 */
    tcpsvr_start();    /* net_step3 启动服务器 */

    usb_host_hard_init();

    /* 系统上电RTC时间 */
    gd32_rtc_init();
    gd32_rtc_get_time(&time);
    ENTER_CRITICAL();
    sys_state.sys_sec = gd32_rtc_time_to_sec(&time);
    EXIT_CRITICAL();
    dbg_info("\r\nSystem time: %04d-%02d-%02d %02d:%02d:%02d\r\n", time.year, time.month, time.mday, time.hour, time.minute, time.second);

    /* 开机自检 */
    //    dev_self_test();

    comm_init(); // 位置靠后，避免串口阻塞

    /* 投递起始任务 */
    agent_post(NOT_FROM_ISR, AGENT_PRIO_HI, start_task, (void *)0);

    /* 注册系统维护任务 */
    ostimer_register(OSTIMER_SYS_SERVICE, service_task, (void *)0, AGENT_PRIO_HI, configTICK_RATE_HZ);
}

/*
 * 功能描述：这个函数用于系统上电硬件自检等（代理方式调用）
 * 入参说明：arg --- 入参
 * 返 回 值：无
 * 备    注：溢出时间 = 预分频值（16）   × 计数器值（4095） / FWDGT 时钟频率（IRC32.768K） ≈ 2.0s
 *           溢出时间 = 预分频值（64）   × 计数器值（4095） / FWDGT 时钟频率（IRC32.768K） ≈ 8.0s
 *           溢出时间 = 预分频值（128）  × 计数器值（4095） / FWDGT 时钟频率（IRC32.768K） ≈ 16.0s
 *           溢出时间 = 预分频值（256）  × 计数器值（4095） / FWDGT 时钟频率（IRC32.768K） ≈ 32.0s
 */
static void start_task(void *arg) {
    ENTER_CRITICAL();
    /* 看门狗溢出时间配置 */
    fwdgt_config(0x0fff, FWDGT_PSC_DIV16);
    fwdgt_enable();

    /* 启动定时服务（按宏定义的顺序排列） */
    ostimer_open(OSTIMER_SYS_SERVICE, 1); // 0
    // ostimer_open(OSTIMER_TEMP_TASK, 1);     // 1
    //     ostimer_open(OSTIMER_CAMERA_TASK, 1);   // 2
    ostimer_open(OSTIMER_UI_TASK, 1);       // 3
    ostimer_open(OSTIMER_BURST_TASK, 1);    // 4
    ostimer_open(OSTIMER_KEY_SCAN_TASK, 1); // 5
    ostimer_open(OSTIMER_RECORD_TASK, 1);   // 6
    ostimer_open(OSTIMER_ALARM_TASK, 1);    // 7
    ostimer_open(OSTIMER_USB_UVC_TASK, 1);  // 8
                                            // ostimer_open(OSTIMER_DATA_SEND_TASK, 1);  // 9
    ostimer_open(OSTIMER_HEATER_TASK, 1);   // 9
    EXIT_CRITICAL();

#if (USE_CM_BACKTRACE_TOOL && CM_BACKTRACE_TEST)
    void fault_test_by_unalign(void);
    fault_test_by_unalign();
#endif
#if SUPPORT_EEPROM
    agent_post(FROM_ISR, AGENT_PRIO_LO, state_record_task, (void *)0);
#endif
}

/*
 * 功能描述：这个函数处理设备启动类型（冷启动和热启动）
 * 入参说明：无
 * 返 回 值：无
 */
static void dev_start_type(void) {
    ENTER_CRITICAL();

    warm_start = (start_flag == WARM_START_MAGIC);
    start_flag = WARM_START_MAGIC;
    if (warm_start) { /* 热启动 */
        sys_state.dev_state |= (1 << DEV_STA_WARM_START);
        sys_state.state_sec[DEV_STA_WARM_START] = sys_state.sys_sec;
        /*热启动参数加载*/
        hot_start_quick_save_get();
    } else { /* 冷启动 */
        sys_state.dev_state |= (1 << DEV_STA_COOL_START);
        sys_state.state_sec[DEV_STA_COOL_START] = sys_state.sys_sec;
    }
    EXIT_CRITICAL();
}

/*
 * 功能描述：此函数获取DAC-CAMERA的互斥锁
 * 入参说明：无
 * 返 回 值：0 --- 成功；其他 --- 失败
 */
int dac_camera_lock(void) {
    BaseType_t ret;

    /* 获取互斥锁 */
    ret = xSemaphoreTake(dac_camera_mutex, DAC_CAMERA_MUTEX_TIMEOUT);

    return (ret == pdPASS) ? 0 : -1;
}

/*
 * 功能描述：此函数释放DAC-CAMERA的互斥锁
 * 入参说明：无
 * 返 回 值：0 --- 成功；其他 --- 失败
 */
int dac_camera_unlock(void) {
    BaseType_t ret;

    /* 释放互斥锁 */
    ret = xSemaphoreGive(dac_camera_mutex);

    return (ret == pdPASS) ? 0 : -1;
}

/*
 * 功能描述：获取复位原因
 * 入参说明：无。
 * 返 回 值：无。
 */
void reset_reason(void) {
    dbg_info("%s", "Reset Reason:");

    if (rcu_flag_get(RCU_FLAG_BORRST)) {
        dbg_info("%s", "BOR reset\r\n");
    }
    if (rcu_flag_get(RCU_FLAG_EPRST)) {
        dbg_info("%s", "external PIN reset\r\n");
    }
    if (rcu_flag_get(RCU_FLAG_PORRST)) {
        dbg_info("%s", "Power reset\r\n");
    }
    if (rcu_flag_get(RCU_FLAG_SWRST)) {
        dbg_info("%s", "software reset\r\n");
    }
    if (rcu_flag_get(RCU_FLAG_FWDGTRST)) {
        dbg_info("%s", "free watchdog timer reset\r\n");
    }
    if (rcu_flag_get(RCU_FLAG_WWDGTRST)) {
        dbg_info("%s", "window watchdog timer reset\r\n");
    }
    if (rcu_flag_get(RCU_FLAG_LPRST)) {
        dbg_info("%s", "low-power reset\r\n");
    }
    rcu_all_reset_flag_clear();
}

/*
 * 功能描述：电源低电压中断
 * 入参说明：无
 * 返 回 值：无
 */
static void lvd_handler(void) {
    DEFINE_CPU_SR;

    ENTER_ISR();
    if (exti_interrupt_flag_get(EXTI_16) == SET) {
        exti_interrupt_flag_clear(EXTI_16);

        sys_quick_save.flag_temp_internal = 1;
        sys_quick_save.temp_internal_n    = sys_state.internal_temp[0];
        sys_quick_save.temp_internal_w    = sys_state.internal_temp[1];
        /* 快速直接写入 Flash（已提前擦除） */
        //  flash_write_fast(QUICK_SAVE_ADDR, (const uint32_t *)&sys_quick_save, sizeof(sys_quick_save_t) / 4);
        gdf_write_data_by_page_fast(QUICK_SAVE_ADDR, sizeof(sys_quick_save_t), (uint8_t *)&sys_quick_save);
    }
    EXIT_ISR();
}

/*
 * 功能描述：低电压监测初始化
 * 入参说明：无
 * 返 回 值：无
 * 备    注：需要在para_init之后
 */
static void lvd_init(void) {
    pmu_lvd_select(PMU_LVDT_7);

    exti_init(EXTI_16, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    exti_interrupt_flag_clear(EXTI_16);

    gd32_setup_isr(LVD_IRQn, lvd_handler, 0);
    gd32_setup_irq(LVD_IRQn, 1);

    gdf_erase_pages(QUICK_SAVE_ADDR, 1); /* 先擦除页节省时间 */
}

#if (USE_CM_BACKTRACE_TOOL && CM_BACKTRACE_TEST)
/*
 * 功能描述：cmbacktreac 除0测试
 * 入参说明：无。
 * 返 回 值：无。
 */
void fault_test_by_div0(void) {
    volatile int *SCB_CCR = (volatile int *)0xE000ED14; // SCB->CCR
    int           x, y, z;

    *SCB_CCR |= (1 << 4); /* bit4: DIV_0_TRP. */

    x = 10;
    y = 0;
    z = x / y;
    printf("z:%d\n", z);
}

/*
 * 功能描述：cmbacktreac 字节不对齐测试
 * 入参说明：无。
 * 返 回 值：无。
 */
void fault_test_by_unalign(void) {
    volatile int *SCB_CCR = (volatile int *)0xE000ED14; // SCB->CCR
    volatile int *p;
    volatile int  value;

    *SCB_CCR |= (1 << 3); /* bit3: UNALIGN_TRP. */

    p     = (int *)0x00;
    value = *p;
    printf("addr:0x%02X value:0x%08X\r\n", (int)p, value);

    p     = (int *)0x04;
    value = *p;
    printf("addr:0x%02X value:0x%08X\r\n", (int)p, value);

    p     = (int *)0x03;
    value = *p;
    printf("addr:0x%02X value:0x%08X\r\n", (int)p, value);
}
#endif

/*
 * 功能描述：这个函数处理tp1协议的这个命令：CMD_REBOOT
 * 入参说明：op --- 操作类型
 *           data --- 指向包有效载荷
 *           len --- 有效载荷长度
 *           pkt --- 输出包缓冲
 * 返 回 值：输出包长度
 */
int cmd_version(int op, void *data, int len, void *pkt) {
    int  ret = 0;
    char ver[10];

    if (op == PROT_OP_RD) {
        /* AAP版本 */
        ver[0] = version_major(SYS_VERSION.fw);
        ver[1] = version_minor(SYS_VERSION.fw);
        ver[2] = version_serial(SYS_VERSION.fw);
        /* 硬件版本 */
        ver[3] = version_major(SYS_VERSION.hw);
        ver[4] = version_minor(SYS_VERSION.hw);
        ver[5] = version_serial(SYS_VERSION.hw);
        /* BootLoader版本 */
        ver[6] = version_major(SYS_VERSION.boot);
        ver[7] = version_minor(SYS_VERSION.boot);
        ver[8] = version_serial(SYS_VERSION.boot);

        // ret = proto_base_build_pkt(pkt, DEV_ID, CMD_VERSION, op, ver, 9);
    }

    return ret;
}

/*
 * 功能描述：这个函数处理tp1协议的这个命令：CMD_REBOOT
 * 入参说明：op --- 操作类型
 *           data --- 指向包有效载荷
 *           len --- 有效载荷长度
 *           pkt --- 输出包缓冲
 * 返 回 值：输出包长度
 */
int sys_reboot(int op, void *data, int len, void *pkt) {
    int ret = 0;

    data = data;

    if (op == PROT_OP_RD) {
        ENTER_CRITICAL();
        sys_state.sys_op.reset = 1;
        EXIT_CRITICAL();

        // ret = proto_base_build_pkt(pkt, DEV_ID, CMD_REBOOT, op, 0, 0);
    }

    return ret;
}

void dev_info_init(sys_state_t *para) {
    mem_set(para->dev_info, sizeof(para->dev_info), 0);
    snprintf(para->dev_info, sizeof(para->dev_info), "\r\n%s\r\n%s\r\n", COMPANY_LOGO, PRODUCT_NAME);
    snprintf(para->dev_info + string_length(para->dev_info), sizeof(para->dev_info) - strlen(para->dev_info), "HW: V%d.%d.%d\r\n", HW_VER_MAJOR,
             HW_VER_MINOR, HW_VER_SERIAL);
    snprintf(para->dev_info + string_length(para->dev_info), sizeof(para->dev_info) - strlen(para->dev_info), "FW: V%d.%d.%d, %s\r\n", FW_VER_MAJOR,
             FW_VER_MINOR, FW_VER_SERIAL, FW_BUILT);
}

/*
 * 功能描述：这个函数用于处理系统命令维护
 * 入参说明：arg --- 入参
 * 返 回 值：无
 * 备    注：周期性（2秒）调用
 */
static void sys_cmd_handler(void *arg) {
    if (!sys_state.sys_op.valid) {
        // 无效命令，返回
        return;
    }

    /* 参数备份与恢复 */
    if (sys_state.sys_op.recover == 1) {
        /* 工厂参数->用户参数、备份参数 */
        pmm_restore_to_factory();
        // sys_para_restore();
    } else if (sys_state.sys_op.recover == 2) {
        /* 用户参数->备份参数、工厂参数 */
        // sys_para_backup();
        pmm_save_to_factory();
    } else if (sys_state.sys_op.recover == 3) {
        /* 默认参数->用户参数、备份参数 */
        pmm_restore_to_default();
    } else if (sys_state.sys_op.recover == 4) {
        /* 用户参数->备份参数 */
        pmm_save_to_backup();
    } else { /* 错误处理 */
    }
    sys_state.sys_op.recover = 0;

    /* 复位 */
    if (fw_info.status == UG_STATE_CHECK_PASS) {
        dbg_info("UP: %s\r\n", "FW write complete, reset!");
        gd32_delay_ms(1000);
        __set_FAULTMASK(1);
        safe_reset();
    } else if (sys_state.sys_op.reset) {
        gd32_delay_ms(15);
        __disable_irq();
        safe_reset();
    }
    taskENTER_CRITICAL();
    sys_state.sys_op.valid = 0;
    taskEXIT_CRITICAL();
}

/*
 * 功能描述：这个函数用于系统维护（系统运行指示灯，看门狗，更新系统时间）
 * 入参说明：arg --- 入参
 * 返 回 值：无
 * 备    注：周期性（1秒）调用执行
 */
static void service_task(void *arg) {

    /* 喂狗 */
    fwdgt_counter_reload();
    /* 热启动参数临时设置 */
    hot_start_quick_save_set();

#if IS_USE_SELF_CHECK
    /* 打印设备开机自检结果 */
    dbg_self_check_info();
#endif

    /* 更新系统时间和运行时间 */
    sys_state.run_sec++;
    if (sys_state.dev_error[DEV_ERR_RTC] == DEV_ST_ERR) {
        sys_state.sys_sec++;
    } else {
        sys_state.sys_sec = gd32_rtc_get_second();
    }

    /* 处理下发的系统命令 */
    sys_cmd_handler((void *)0);
    /*更新错误字*/
    error_state_upgrade();

    /*模拟中断触发*/
    //    static int triger_count = 0;
    //    if (triger_count) {
    //        exti_software_interrupt_enable(EXTI_0);
    //    }
}
