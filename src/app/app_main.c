/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：app_main.c
 * 文件描述：主程序
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2025/03/12 陈军  封装第三方工具配置和上电打印信息
 */

#include "platform.h"
#include "agent.h"
#include "ostimer.h"
#include "storage.h"
#include "thread.h"
#include "system.h"
#include "mail_box.h"
#include "message.h"
#include "console.h"

#include "drv_usb_hw.h"
#include "drv_usb_core.h"
#include "usbh_usr.h"
#include "usbh_video.h"

#if USE_SYSTEMVIEW_TOOL
#include "SEGGER_SYSVIEW.h"
#endif

#if USE_CM_BACKTRACE_TOOL
#include "cm_backtrace.h"
#endif

#if USE_PERF_COUNTER_TOOL
#include "perf_counter.h"
float time_shift_ms(int64_t time);

#endif

static TaskHandle_t os_start_handle;
extern uint8_t      put_port;
/* 内部函数 */
static void os_start_task(void *arg);
static void startup_info(void);
static void tools_init(void);

/*
 * 功能描述：主函数
 * 入参说明：无
 * 返 回 值：无
 */
int main(void) {
    BaseType_t ret;

    /* 底层初始化 */
    nvic_vector_table_set(NVIC_VECTTAB_FLASH, VECTOR_TAB_OFFSET); /* 向量表偏移 */

    gd32_ll_init();

    /* 第三方工具配置 */
    tools_init();

    /* 设备上电信息打印 */
    startup_info();

    /* 创建开始任务 */
    ret = xTaskCreate((TaskFunction_t)os_start_task,     /* 任务入口函数 */
                      (const char *)"os_start_task",     /* 任务名字 */
                      (uint16_t)START_TASK_STKSIZE,      /* 任务堆栈大小 */
                      (void *)NULL,                      /* 任务参数 */
                      (UBaseType_t)START_TASK_PRIO,      /* 任务优先级 */
                      (TaskHandle_t *)&os_start_handle); /* 任务句柄 */

    if (ret != pdPASS) {
        printf("%s(): OS-Start-Task create failed!\r\n", __FUNCTION__);
        return -1;
    }

    /* 启动系统调度 */
    vTaskStartScheduler();

    /* 正常不会执行到这里 */
    return 0;
}

/*
 * 功能描述：这个函数是创建的第一个任务
 * 入参说明：arg --- 任务参数
 * 返 回 值：无
 */
static void os_start_task(void *arg) {
    ENTER_CRITICAL();

    /* 系统初始化 */
    agent_init();
    ostimer_init();
    storage_init();
    thread_init();
    msg_init();
    mbox_init();
    console_init();
    hq_sys_init();

    /* 删除起始任务 */
    vTaskDelete(os_start_handle);

    dbg_info("%s\r\n", "System initialized successfully!");

    EXIT_CRITICAL();
}

/*
 * 功能描述：这个函数配置和打印设备上电信息
 * 入参说明：无
 * 返 回 值：无
 */
static void startup_info(void) {
    port_para_t para;

    /* 配置调试串口 */
    if (powerup_uart_para_get(&para)) {
        dbg_init(DBG_LEVEL_INFO, SYS_DEF_PARA.port.baudrate, "8N1");
    } else {
        dbg_init(para.dbg_level, para.baudrate, "8N1");
        put_port = para.dbg_port;
    }
    dev_info_init(&sys_state);
    dbg_info("%s\r\n", sys_state.dev_info);
    /* 复位原因打印 */
    reset_reason();
}

/*
 * 功能描述：这个函数初始化第三方工具包
 * 入参说明：无
 * 返 回 值：无
 */
static void tools_init(void) {
    char hw_ver[10], fw_ver[10];

    /* systemview */
#if USE_SYSTEMVIEW_TOOL
    SEGGER_SYSVIEW_Conf();
#endif

    sprintf(hw_ver, "V%d.%d.%d", version_major(SYS_VERSION.hw), version_minor(SYS_VERSION.hw), version_serial(SYS_VERSION.hw));

    sprintf(fw_ver, "V%d.%d.%d\r\n", version_major(SYS_VERSION.fw), version_minor(SYS_VERSION.fw), version_serial(SYS_VERSION.fw));

    /* cm_backtrace  */
#if USE_CM_BACKTRACE_TOOL
    cm_backtrace_init("APP", hw_ver, fw_ver);
#endif

    /* Perf_counter  */
#if USE_PERF_COUNTER_TOOL
    init_cycle_counter(true); /* SysTick初始化则 True，否则 False */

#endif
}

#if USE_PERF_COUNTER_TOOL

/*
 * 功能描述：将perf_counter获取的时钟周期转换为毫秒
 * 入参说明：time —— perf_counter返回的时钟周期数（int64_t）
 * 返 回 值：转换后的毫秒（float）
 * 备    注：The start_cycle_counter has been included. Only one can exist globally.
 *       If you want to test the code segment, please remove start_cycle_counter.
 */
float time_shift_ms(int64_t time) {
    float tiem_bsp;
    tiem_bsp = (float)time / (float)SystemCoreClock * 1000.0f;

    return tiem_bsp;
}

#endif
