/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：FreeRTOSConfig.h
 * 文件描述：FreeRTOS实时操作系统配置文件
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军    初始版本
 *           V1.1 2025/04/21 李兆越  适配AC6
 */
#ifndef _FREERTOS_CONFIG_H_
#define _FREERTOS_CONFIG_H_

#include <stdio.h>
#include "tools_cfg.h"

/********************************************************************************************************
 *   编译器和平台特定配置
 ********************************************************************************************************/
/* 编译器宏区分说明：
 * __ICCARM__   → IAR Embedded Workbench，使用 _Pragma 方式对齐
 * __CC_ARM     → Keil MDK（ARMCC，AC5 编译器），使用 __attribute__((aligned)) 和 __packed__
 * __clang__    → Keil MDK（ARMClang，AC6 编译器），使用 Clang 风格的 __attribute__
 * __GNUC__     → GNU GCC 编译器（如 ARM-GCC），同样使用 __attribute__ 风格
 */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)||defined(__clang__)
#include <stdint.h> /* 确保stdint仅由编译器使用，而不是汇编器 */
extern uint32_t SystemCoreClock;
#endif

/********************************************************************************************************
 *   FreeRTOS基础配置配置选项
 ********************************************************************************************************/
/* 任务调度配置 */
#define configUSE_PREEMPTION                     1 /* 1：使用抢占式内核，0：运行在协程模式下 */
#define configUSE_TIME_SLICING                   1 /* 1：启用时间片调度，确保同优先级任务轮流执行 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1 /* 1：启用硬件优化的任务选择方法 */
#define configUSE_TICKLESS_IDLE                  0 /* 0：不启用tickless模式 */
#define configUSE_QUEUE_SETS                     1 /* 1：启用队列集功能，允许任务等待多个队列事件 */
/* 系统时钟和任务优先级配置 */
#define configCPU_CLOCK_HZ                       SystemCoreClock       /* 设置CPU核心时钟频率 */
#define configTICK_RATE_HZ                       1000                  /* 设置系统时钟节拍频率，1000Hz意味着1ms一个tick */
#define configMAX_PRIORITIES                     32                    /* 设置系统可用的最大任务优先级数 */
#define configMINIMAL_STACK_SIZE                 ((unsigned short)200) /* 设置空闲任务堆栈大小 */
#define configMAX_TASK_NAME_LEN                  20                    /* 设置任务名的最大字符长度 */
/* 任务和中断管理配置 */
#define configUSE_16_BIT_TICKS                   0 /* 0：设置系统节拍计数器为32位，以支持长时间延时 */
#define configIDLE_SHOULD_YIELD                  1 /* 1：空闲任务在有同优先级任务时将让出CPU */
#define configUSE_TASK_NOTIFICATIONS             1 /* 1：启用任务通知功能，为任务提供一种轻量级的信号机制 */
#define configUSE_MUTEXES                        1 /* 1：启用互斥信号量功能 */
#define configQUEUE_REGISTRY_SIZE                8 /* 设置可注册的最大队列和信号量数目 */
#define configUSE_RECURSIVE_MUTEXES              1 /* 1：启用递归互斥信号量 */
#define configUSE_APPLICATION_TASK_TAG           0 /* 0：禁用任务标签功能 */
#define configUSE_COUNTING_SEMAPHORES            1 /* 1：启用计数信号量 */

/********************************************************************************************************
 *   FreeRTOS与内存申请有关配置选项
 ********************************************************************************************************/
#define configSUPPORT_STATIC_ALLOCATION          0                     /* 不启用静态内存分配支持 */
#define configSUPPORT_DYNAMIC_ALLOCATION         1                     /* 支持动态内存申请 */
#define configTOTAL_HEAP_SIZE                    ((size_t)(80 * 1024)) /* 系统所有总的堆大小30KB 50*/

/********************************************************************************************************
 *   FreeRTOS与钩子函数有关的配置选项（1为启用，0为关闭）
 ********************************************************************************************************/
#define configUSE_IDLE_HOOK                      0 /*是否启用空闲钩子？ */
#define configUSE_TICK_HOOK                      0 /*是否启用时间片钩子？ */
#define configCHECK_FOR_STACK_OVERFLOW           2 /* 是否启用堆栈溢出检测功能钩子？ */
#define configUSE_MALLOC_FAILED_HOOK             0 /* 是否启用内存分配失败钩子？ */

/********************************************************************************************************
 *   FreeRTOS性能监控与调试配置选项
 ********************************************************************************************************/
#define configGENERATE_RUN_TIME_STATS            0 /* 是否启用运行时间统计功能？ */
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()   /* 配置定时器用于运行时间统计 */
#define portGET_RUN_TIME_COUNTER_VALUE()           /* 获取运行时间计数值 */
#define configUSE_TRACE_FACILITY                 1 /* 启用可视化跟踪调试功能 */
#define configUSE_STATS_FORMATTING_FUNCTIONS     1 /* 是否启用统计格式化函数？ */

/********************************************************************************************************
 *   FreeRTOS与协程有关的配置选项
 ********************************************************************************************************/
#define configUSE_CO_ROUTINES                    0 /* 是否启用协程功能？启用时必须将croutine.c文件包含到项目中 */
#define configMAX_CO_ROUTINE_PRIORITIES          2 /* 设置协程可用的最大优先级数目 */

/********************************************************************************************************
 *   FreeRTOS与软件定时器有关的配置选项
 ********************************************************************************************************/
#define configUSE_TIMERS                         1                              /* 是否启用软件定时器功能？ */
#define configTIMER_TASK_PRIORITY                (configMAX_PRIORITIES - 1)     /* 设置软件定时器任务的优先级 */
#define configTIMER_QUEUE_LENGTH                 5                              /* 软件定时器队列的长度（同时激活的定时器数量） */
#define configTIMER_TASK_STACK_DEPTH             (configMINIMAL_STACK_SIZE * 2) /* 软件定时器任务堆栈大小 */

/********************************************************************************************************
 *   FreeRTOS可选API功能支持（1为启用相应函数，0为关闭）
 ********************************************************************************************************/
#define INCLUDE_xTaskGetSchedulerState           1 /* 是否启用查询调度器状态？ */
#define INCLUDE_vTaskPrioritySet                 1 /* 是否启用动态改变任务优先级？ */
#define INCLUDE_uxTaskPriorityGet                1 /* 是否启用获取任务当前优先级？ */
#define INCLUDE_vTaskDelete                      1 /* 是否启用删除一个任务？ */
#define INCLUDE_vTaskCleanUpResources            1 /* 是否启用清理其资源的功能？ */
#define INCLUDE_vTaskSuspend                     1 /* 是否启用挂起和恢复任务？ */
#define INCLUDE_vTaskDelayUntil                  1 /* 是否启用任务延时至指定的绝对时间？ */
#define INCLUDE_vTaskDelay                       1 /* 是否启用任务延时指定的时间周期？ */
#define INCLUDE_eTaskGetState                    1 /* 是否启用获取任务的当前状态？ */
#define INCLUDE_xTimerPendFunctionCall           1 /* 是否启用从中断服务程序中安全地调用函数？ */

/********************************************************************************************************
 *   FreeRTOS与中断有关的配置选项
 ********************************************************************************************************/
#ifdef __NVIC_PRIO_BITS
/* 当使用CMSIS时，__NVIC_PRIO_BITS会被指定 */
#define configPRIO_BITS __NVIC_PRIO_BITS
#else
#define configPRIO_BITS 4 /* 默认15个优先级级别 */
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15 /* 中断最低优先级 */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 8  /* 系统可管理的最高中断优先级 */
/* 设置内核中断优先级为系统允许的最低优先级，确保FreeRTOS内核中断运行在较低优先级，以允许更高优先级的用户
 * 定义中断得到处理。 */
#define configKERNEL_INTERRUPT_PRIORITY              (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
/* 设置系统调用的最高中断优先级，这是FreeRTOS API在ISR中安全调用的最高优先级，防止高优先级中断干扰系统调用。 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY         (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define vPortSVCHandler                              SVC_Handler    /* 指定系统调用SVC处理函数 */
#define xPortPendSVHandler                           PendSV_Handler /* 指定挂起的服务调用PendSV处理函数 */

/********************************************************************************************************
 *   FreeRTOS的断言机制
 ********************************************************************************************************/
#if USE_CM_BACKTRACE_TOOL
#include <cm_backtrace.h>
#define vAssertCalled(char, int)                                                                                                                     \
    {                                                                                                                                                \
        cm_backtrace_assert(cmb_get_sp());                                                                                                           \
        printf("Error:%s,%d\r\n", char, int);                                                                                                        \
    }
#else
#define vAssertCalled(char, int) printf("Error:%s,%d\r\n", char, int)
#endif

#define configASSERT(x)                                                                                                                              \
    if ((x) == 0) vAssertCalled(__FILE__, __LINE__)

#if USE_SYSTEMVIEW_TOOL
#include "SEGGER_SYSVIEW_FreeRTOS.h"

#define INCLUDE_xTaskGetIdleTaskHandle 1
#define INCLUDE_pxTaskGetStackStart    1

#endif

#endif /* _FREERTOS_CONFIG_H_ */
