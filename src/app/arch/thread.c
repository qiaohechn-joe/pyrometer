/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：thread.c
 * 文件描述：用户线程管理
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#include "thread.h"

static TaskHandle_t thread_handle[USER_THREAD_CNT];

/*
 * 功能描述：初始化线程单元
 * 入参说明：无
 * 返 回 值：0 --- 成功；其他 --- 失败
 */
int thread_init(void) {
    mem_set(thread_handle, sizeof(thread_handle), 0);

    return 0;
}

/*
 * 功能描述：创建线程
 * 入参说明：num      --- 线程编号，从1到USER_THREAD_CNT
 *           entry    --- 线程入口函数
 *           arg      --- 传递给入口函数的参数
 *           stk_size --- 栈大小
 * 返 回 值：线程编号，从1到USER_THREAD_CNT
 */
int thread_create(int num, thread_entry_t entry, void *arg, int stk_size) {
    BaseType_t ret;
    char       name[32];
    int        prio;

    /* 检查线程数量 */
    if ((num < 1) || (num > USER_THREAD_CNT)) {
        return 0;
    }

    sprintf(name, "UserThread%d", num);
    prio = USER_THREAD_START_PRIO - (num - 1);

    /* 创建任务线程 */
    ret = xTaskCreate((TaskFunction_t)entry,                    /* 入口函数 */
                      name,                                     /* 名称 */
                      (uint16_t)stk_size,                       /* 栈大小 */
                      (void *)arg,                              /* 参数 */
                      (UBaseType_t)prio,                        /* 优先级 */
                      (TaskHandle_t *)&thread_handle[num - 1]); /* 任务句柄的指针 */

    if (ret != pdPASS) {
        printf("Task %s create failed!\r\n", name);
        return 0;
    }

    return num;
}
