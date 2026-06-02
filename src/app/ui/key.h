/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：key.h
 * 文件描述：按键驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：基于FreeRTOS；支持单击（按键松开）、长按、同时按下；使用标志位方式;按下、松开防抖
 * 更新记录：
 *           V1.0 2025/04/15 李兆越 初始版本
 *           V1.1 2025/04/17 李兆越 消息队列改为标志位方式
 *           V1.2 2025/04/22 李兆越 加入无按键操作置位逻辑
 */

#ifndef _KEY_H_
#define _KEY_H_

#include "platform.h"
#include "FreeRTOS.h"
#include "task.h"

/* 按键定义 */
#define KEY_UP                     0 /* 上键 */
#define KEY_DOWN                   1 /* 下键 */
#define KEY_SET                    2 /* 设置键 */
#define KEY_RIGHT                  3 /* 右键 */
#define KEY_CNT                    4 /* 按键总数 */

/* 按键掩码定义（TM1639按键值） */
#define BTN_UP_MASK                0x0080
#define BTN_DOWN_MASK              0x0004
#define BTN_SET_MASK               0x0040
#define BTN_RIGHT_MASK             0x0008

/* 时间常量定义 */
#define DEBOUNCE_TIME              pdMS_TO_TICKS(10)   /* 防抖时间10ms */
#define LONG_PRESS_1S_THRESHOLD    pdMS_TO_TICKS(1000) /* 长按1秒阈值 */
#define LONG_PRESS_2S_THRESHOLD    pdMS_TO_TICKS(2000) /* 长按2秒阈值 */
#define LONG_PRESS_3S_THRESHOLD    pdMS_TO_TICKS(3000) /* 长按3秒阈值 */

#define NO_KEY_PRESS_TIMEOUT       pdMS_TO_TICKS(10000) /* 10秒无按键操作超时时间(单位ms) */
#define NO_KEY_PRESS_SHORT_TIMEOUT pdMS_TO_TICKS(2000)  /* 2秒无按键操作超时时间(单位ms) */

/* 按键状态定义 */
typedef enum {
    STATE_IDLE,             /* 空闲状态 */
    STATE_DEBOUNCE_PRESS,   /* 按下防抖 */
    STATE_PRESSED,          /* 按键按下（已消抖）*/
    STATE_DEBOUNCE_RELEASE, /* 释放防抖 */
} button_state_t;

/* 按键信息结构体 */
typedef struct {
    button_state_t state;
    TickType_t     press_start_time;
    TickType_t     debounce_time;
    uint16_t       key_mask;
    uint8_t        key_id;
    bool           long_pressed; /* 是否已触发长按 */
    int            step_value;   /* 步进值 */
    uint8_t        short_flag;   /* 短按标志 */
    uint8_t        long_flag;    /* 长按标志 */
} key_info_t;

/* 外部接口声明 */
extern volatile uint8_t key_idle_flag;
extern key_info_t       KEY_TABLES[];

void    key_init(void);
void    key_task(void *arg);
uint8_t key_get_short_flag(uint8_t key_id);
uint8_t key_get_long_flag(uint8_t key_id);

#endif /* _KEY_H_ */
