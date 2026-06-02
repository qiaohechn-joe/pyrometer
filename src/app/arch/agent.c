/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：agent.c
 * 文件描述：代理任务管理
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/12/05 陈军  修改结构体定义格式
 */

#include "agent.h"

/* 代理信息结构体 */
typedef struct {
    const char *name;
    int         os_prio;
    uint32_t    stk_size;
} agent_info_t;

/* 代理信息初始化 */
static const agent_info_t AGENT_INFO[AGENT_PRIO_CNT] = {
    {.name = "Agent-HI", .os_prio = AGENT_HI_TASK_PRIO, .stk_size = AGENT_HI_TASK_STKSIZE}, /* 高优先级 */
    {.name = "Agent-MD", .os_prio = AGENT_MD_TASK_PRIO, .stk_size = AGENT_MD_TASK_STKSIZE}, /* 中优先级 */
    {.name = "Agent-LO", .os_prio = AGENT_LO_TASK_PRIO, .stk_size = AGENT_LO_TASK_STKSIZE}, /* 低优先级 */
};

static TaskHandle_t  agent_handle[AGENT_PRIO_CNT]; /* 代理任务句柄数组 */
static agent_queue_t agent_queue[AGENT_PRIO_CNT];  /* 代理队列数组 */

/* 内部函数 */
static void agent_task(void *arg);

/*
 * 功能描述：此函数初始化代理功能单元
 * 入参说明：无
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
int agent_init(void) {
    const agent_info_t *info;
    agent_queue_t      *queue;
    BaseType_t          ret;
    int                 i;

    info  = &AGENT_INFO[0];
    queue = &agent_queue[0];

    for (i = 0; i < AGENT_PRIO_CNT; i++, info++, queue++) {
        if (info->stk_size < 64) {
            continue;
        }

        ret = xTaskCreate((TaskFunction_t)agent_task,        /* 入口函数 */
                          info->name,                        /* 任务名称 */
                          (uint16_t)info->stk_size,          /* 堆栈大小 */
                          (void *)i,                         /* 参数 */
                          (UBaseType_t)info->os_prio,        /* 优先级 */
                          (TaskHandle_t *)&agent_handle[i]); /* 任务句柄指针 */

        if (ret != pdPASS) {
            printf("Task %s create failed!\r\n", info->name);
            return -1;
        }

        /* 清队列 */
        queue->count = 0;
        queue->head = queue->tail = queue->array;
        queue->sem                = xSemaphoreCreateCounting((UBaseType_t)AGENT_QUEUE_SIZE, (UBaseType_t)0);
    }

    return 0;
}

/*
 * 功能描述：此函数清除代理的函数队列
 * 入参说明：from_isr --- 是否从中断服务例程（ISR）中发送？ 0=否 其他=是
 *           prio --- 代理优先级，AGENT_PRIO_xx
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
void agent_flush(int from_isr, int prio) {
    DEFINE_CPU_SR;
    agent_queue_t *queue = &agent_queue[prio];

    if (from_isr) {
        ENTER_ISR();
    } else {
        ENTER_CRITICAL();
    }

    queue->count = 0;
    queue->head = queue->tail = queue->array;

    if (from_isr) {
        EXIT_ISR();
    } else {
        EXIT_CRITICAL();
    }
}

/*
 * 功能描述：此函数用于将一个函数入口（及其参数）加入到代理队列中
 * 入参说明：from_isr --- 是否从中断服务例程（ISR）中调用？ 0=否 其他=是
 *           prio --- 代理优先级，AGENT_PRIO_xx
 *           func --- 函数入口
 *           arg --- 函数参数
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int agent_post(int from_isr, int prio, agent_entry_t func, void *arg) {
    agent_queue_t *queue = &agent_queue[prio];
    BaseType_t     higher_woken;
    DEFINE_CPU_SR;
    BaseType_t ret;

    if (from_isr) {
        ENTER_ISR();
    } else {
        ENTER_CRITICAL();
    }

    /* 加入到任务队列 */
    if (queue->count == AGENT_QUEUE_SIZE) { /* 队列已满 */
        if (from_isr) {
            EXIT_ISR();
        } else {
            EXIT_CRITICAL();
        }
        return -1;
    }

    /* 向队列中添加元素 */
    queue->tail->entry = func;
    queue->tail->arg   = arg;
    queue->tail++;
    if (queue->tail == (queue->array + AGENT_QUEUE_SIZE)) {
        queue->tail = queue->array;
    }
    queue->count++;

    if (from_isr) {
        EXIT_ISR();
    } else {
        EXIT_CRITICAL();
    }

    /* 通知代理任务 */
    if (from_isr) {
        higher_woken = pdFALSE;
        ret          = xSemaphoreGiveFromISR(queue->sem, &higher_woken);
        if (ret != pdTRUE) {
            return -1;
        }
        portYIELD_FROM_ISR(higher_woken);
    } else {
        ret = xSemaphoreGive(queue->sem);
        if (ret != pdPASS) {
            return -1;
        }
    }

    return 0;
}

/*
 * 功能描述：这个函数是代理程序任务
 * 入参说明：arg --- 参数（代理优先级）
 * 返 回 值：无
 */
static void agent_task(void *arg) {
    int            prio;
    agent_queue_t *queue;
    agent_entry_t  func_entry;
    void          *func_arg;
    BaseType_t     ret;

    prio  = (int)arg;
    queue = &agent_queue[prio];

    for (;;) {
        /* 一直等待 */
        ret = xSemaphoreTake(queue->sem, portMAX_DELAY);
        if (ret == pdPASS) {
            func_entry = 0;

            ENTER_CRITICAL();

            if (queue->count > 0) { /* 队列不满 */
                /* 出队 */
                func_entry = queue->head->entry;
                func_arg   = queue->head->arg;
                queue->head++;
                if (queue->head == (queue->array + AGENT_QUEUE_SIZE)) {
                    queue->head = queue->array;
                }
                queue->count--;
            }

            EXIT_CRITICAL();

            if (func_entry) {
                func_entry(func_arg); /* 调用功能函数 */
            }
        }
    }
}
