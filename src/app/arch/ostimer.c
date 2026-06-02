/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ostimer.c
 * 文件描述：操作系统定时器管理
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/12/04 陈军  统一结构体定义方式
 *           V1.2 2024/12/24 李兆越  加入设置定时器优先级
 */

#include "agent.h"
#include "ostimer.h"

static TaskHandle_t        ostimer_handle;              /* 任务句柄 */
static ostimer_list_item_t ostimer_list[OSTIMER_COUNT]; /* 定时器列表 */

/* 内部函数 */
static void ostimer_task(void *arg);

/*
 * 功能描述：此函数初始化操作系统定时器单元
 * 入参说明：无
 * 返 回 值：0 --- 成功；其他 --- 失败
 */
int ostimer_init(void) {
    BaseType_t ret;

    /* 创建任务定时器 */
    ret = xTaskCreate((TaskFunction_t)ostimer_task,     /* 入口函数 */
                      "OSTimerTask",                    /* 任务名称 */
                      (uint16_t)OSTIMER_TASK_STKSIZE,   /* 栈大小 */
                      (void *)0,                        /* 参数 */
                      (UBaseType_t)OSTIMER_TASK_PRIO,   /* 优先级 */
                      (TaskHandle_t *)&ostimer_handle); /* 任务句柄的指针 */

    if (ret != pdPASS) {
        printf("Task OSTimer create failed!\r\n");
        return -1;
    }

    return 0;
}

/*
 * 功能描述：注册一个函数到定时器列表中
 * 入参说明：id --- 定时器ID，TIMER_xxx
 *           func --- 函数入口
 *           arg --- 函数参数
 *           prio --- 代理优先级，AGENT_PRIO_HI/AGENT_PRIO_MD/AGENT_PRIO_LO
 *           period --- 调度周期
 * 返 回 值：无
 * 备    注：必须在关键段（CRITICAL-SECTION）中保护
 */
void ostimer_register(int id, ostimer_func_t func, void *arg, int prio, int period) {
    ostimer_list_item_t *item = &ostimer_list[id];

    item->func   = func;
    item->arg    = arg;
    item->prio   = prio;
    item->start  = 0;
    item->period = period;
    item->enable = 0;
}

/*
 * 功能描述：打开定时器列表中的一个函数
 * 入参说明：id  --- 定时器ID，STIMER_xxx
 *           now --- 是否立即执行？
 * 返 回 值：无
 * 备    注：必须在关键段（CRITICAL-SECTION）中保护
 */
void ostimer_open(int id, int now) {
    ostimer_list_item_t *item = &ostimer_list[id];

    if ((item->func != 0) && (item->period > 0)) {
        item->start  = xTaskGetTickCount();
        item->enable = 1;
        if (now) {
            agent_post(NOT_FROM_ISR, item->prio, item->func, item->arg);
        }
    }
}

/*
 * 功能描述：关闭定时器列表中的一个函数
 * 入参说明：id  --- 定时器ID，TIMER_xxx
 * 返 回 值：无
 * 备    注：必须在关键段（CRITICAL-SECTION）中保护
 */
void ostimer_close(int id) {
    ostimer_list_item_t *item = &ostimer_list[id];

    item->enable = 0;
}

/*
 * 功能描述：设置定时器周期
 * 入参说明：id     --- 定时器ID，TIMER_xxx
 *           period --- 定时器周期，以OS_TICKS为单位
 * 返 回 值：无
 * 备    注：必须在关键段（CRITICAL-SECTION）中保护
 */
void ostimer_set_period(int id, uint32_t period) {
    ostimer_list_item_t *item = &ostimer_list[id];

    if (item->enable) {
        item->period = period;
    }
}

/*
 * 功能描述：重新启动定时器
 * 入参说明：id  --- 定时器ID，STIMER_xxx
 * 返 回 值：无
 * 备    注：必须在关键段（CRITICAL-SECTION）中保护
 */
void ostimer_restart(int id) {
    ostimer_list_item_t *item = &ostimer_list[id];

    if (item->enable) {
        item->start = xTaskGetTickCount();
    }
}

/*
 * 功能描述：设置定时器优先级
 * 入参说明：id  --- 定时器ID，STIMER_xxAGENT_PRIO_CNTx
 * 返 回 值：无
 * 备    注：必须在关键段（CRITICAL-SECTION）中保护
 */
void ostimer_set_prio(int id, int prio) {
    ostimer_list_item_t *item = &ostimer_list[id];

    if ((item->prio >= 0) && (item->prio < AGENT_PRIO_CNT)) {
        item->prio = prio;
    }
}

/*
 * 功能描述：这个函数是定时器任务
 * 入参说明：arg  --- 参数
 * 返 回 值：无
 */
static void ostimer_task(void *arg) {
    int                  i;
    ostimer_list_item_t *item;
    uint32_t             tick;

    (void)arg;

    while (1) {
        ENTER_CRITICAL();
        tick = xTaskGetTickCount();
        EXIT_CRITICAL();

        for (item = &ostimer_list[0], i = 0; i < OSTIMER_COUNT; i++, item++) { /* 扫描所有定时项目 */
            if (item->enable) {
                if ((tick - item->start) >= item->period) {                      /* 周期到达 */
                    agent_post(NOT_FROM_ISR, item->prio, item->func, item->arg); /* 将函数挂起到代理 */
                    item->start = tick;                                          /* 更新起始时刻 */
                }
            }
        }

        vTaskDelay(configTICK_RATE_HZ / OSTIMER_EXECUT_FREQ_MS);
    }
}
