/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：tools_cfg.h
 * 文件描述：第三方工具管理文件
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/12/03 张晓博  初始版本
 */

#ifndef _TOOLS_CFG_H_
#define _TOOLS_CFG_H_

/********************************************************************************************************
 *   SystemView 组件
 ********************************************************************************************************/
#define USE_SYSTEMVIEW_TOOL    1                /* 是否使用SystemView组件，1：使用，0：不使用 */
#define SYSTEMVIEW_APP_NAME    "PSS101(OR4000)" /* 应用名称，主要用于初始化SystemView */
#define SYSTEMVIEW_DEVICE_NAME "Cortex-M4"      /* 设备名称，主要用于初始化SystemView */

/********************************************************************************************************
 *   Cm_Backtrace 组件
 ********************************************************************************************************/
#define USE_CM_BACKTRACE_TOOL  1 /* 是否使用Cm_Backtrace组件，1：使用，0：不使用*/
#define CM_BACKTRACE_TEST      0 /* Cm_Backtrace测试函数，1：使用，0：不使用*/

/********************************************************************************************************
 *   Perf_counter 组件
 ********************************************************************************************************/
#define USE_PERF_COUNTER_TOOL  1 /* 是否使用 Perf_counter 组件，1：使用，0：不使用*/

/*
#if USE_PERF_COUNTER_TOOL
#include "perf_counter.h"
#endif

使用函数：
start_cycle_counter();           //!< 开始总计时


sys_state.long_cycles = time_shift_ms(stop_cycle_counter());  // 获取 start_cycle_counter 以来的时间
start_cycle_counter();           // 开始总计时


*/

#endif /* _TOOLS_CFG_H_ */
