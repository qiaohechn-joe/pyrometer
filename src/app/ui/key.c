/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：key.c
 * 文件描述：按键驱动程序
 * 作    者：和其光电嵌软团队
 * 备    注：基于FreeRTOS；支持单击（按键松开）、长按、同时按下；使用标志位方式;按下、松开防抖
 * 更新记录：
 *           V1.0 2025/04/15 李兆越 初始版本
 *           V1.1 2025/04/17 李兆越 消息队列改为标志位方式
 *           V1.2 2025/04/22 李兆越 加入无按键操作置位逻辑
 */

#include "key.h"
#include "tm1639.h"

/* 记录上次有操作的时间 */
static TickType_t last_key_action_time = 0;

/* 10秒无操作标志位 */
volatile uint8_t key_idle_flag = 1; /* 默认1为按键无操作 */

/* clang-format off */
key_info_t KEY_TABLES[] = {
    { STATE_IDLE, 0, 0, BTN_UP_MASK,    KEY_UP,    false, 1, 0, 0 },
    { STATE_IDLE, 0, 0, BTN_DOWN_MASK,  KEY_DOWN,  false, 1, 0, 0 },
    { STATE_IDLE, 0, 0, BTN_SET_MASK,   KEY_SET,   false, 1, 0, 0 },
    { STATE_IDLE, 0, 0, BTN_RIGHT_MASK, KEY_RIGHT, false, 1, 0, 0 }
};
/* clang-format on */

/*
 * 功能描述：按键初始化
 * 入参说明：无
 * 返 回 值：无
 */
void key_init(void) {
    /* 重置所有按键标志位 */
    for (size_t i = 0; i < sizeof(KEY_TABLES) / sizeof(KEY_TABLES[0]); i++) {
        KEY_TABLES[i].short_flag = 0;
        KEY_TABLES[i].long_flag  = 0;
    }
}

/*
 * 功能描述：按键处理任务
 * 入参说明：arg - 未使用
 * 返 回 值：无
 * 备    注：70us
 */
void key_task(void *arg) {
    int      retry = 3;
    uint16_t key_status;
    bool     any_key_action = false; /* 标记是否有按键操作 */

    /* 运行时自检 */
    do {
        key_status = tm1639_key_scan(); /* 扫描当前按键状态 */
        if (key_status != 0xFFFF) {
            break;
        }

    } while (retry--);
    if (key_status == 0xFFFF) {
        // dbg_info("%s\r\n", "TM1639 read failed!");
        ENTER_CRITICAL();
        sys_state.dev_error[DEV_ERR_TM1639] = DEV_ST_ERR;
        EXIT_CRITICAL();
        return;
    } else if (key_status & 0xCC) {
        ENTER_CRITICAL();
        sys_state.dev_error[DEV_ERR_TM1639] = DEV_ST_OK;
        EXIT_CRITICAL();
    }

    TickType_t current_time = xTaskGetTickCount(); /* 获取当前系统节拍数 */

    for (size_t i = 0; i < sizeof(KEY_TABLES) / sizeof(KEY_TABLES[0]); i++) {
        key_info_t *key        = &KEY_TABLES[i];
        bool        is_pressed = (key_status & key->key_mask) == key->key_mask; /* 判断按键是否被按下 */

        switch (key->state) {
            case STATE_IDLE: /* 空闲状态 */
                if (is_pressed) {
                    key->state         = STATE_DEBOUNCE_PRESS;
                    key->debounce_time = current_time;
                    key->long_pressed  = false; /* 重置长按标志 */
                }
                break;

            case STATE_DEBOUNCE_PRESS: /* 按下防抖 */
                if (!is_pressed) {
                    key->state = STATE_IDLE;
                } else if (current_time - key->debounce_time >= DEBOUNCE_TIME) {
                    key->state            = STATE_PRESSED; /* 防抖通过，确认按下 */
                    key->press_start_time = current_time;
                }
                break;

            case STATE_PRESSED: /* 按键按下（已消抖）*/
                if (!is_pressed) {
                    key->state         = STATE_DEBOUNCE_RELEASE;
                    key->debounce_time = current_time;
                } else {
                    TickType_t hold_time = current_time - key->press_start_time;

                    if (hold_time >= LONG_PRESS_3S_THRESHOLD) {
                        key->step_value   = 100;
                        key->long_flag    = 1;
                        key->long_pressed = true;
                        any_key_action    = true;
                    } else if (hold_time >= LONG_PRESS_2S_THRESHOLD) {
                        key->step_value   = 10;
                        key->long_flag    = 1;
                        key->long_pressed = true;
                        any_key_action    = true;
                    } else if (hold_time >= LONG_PRESS_1S_THRESHOLD) {
                        key->step_value   = 1;
                        key->long_flag    = 1;
                        key->long_pressed = true;
                        any_key_action    = true;
                    }
                }
                break;

            case STATE_DEBOUNCE_RELEASE: /* 释放防抖 */
                if (is_pressed) {
                    key->state = STATE_PRESSED;
                } else if (current_time - key->debounce_time >= DEBOUNCE_TIME) {
                    if (!key->long_pressed) {
                        key->short_flag = 1; /* 仅当未触发长按时，才触发短按 */
                        key->step_value = 1;
                        any_key_action  = true;
                    }
                    key->state = STATE_IDLE;
                }
                break;
        }
    }
    /* 判断是否有按键操作，有则更新操作时间并清除idle标志 */
    if (any_key_action) {
        last_key_action_time = current_time;
        key_idle_flag        = 0;
    } else if ((current_time - last_key_action_time) >= NO_KEY_PRESS_TIMEOUT) {
        key_idle_flag = 1;
    } else if ((current_time - last_key_action_time) >= NO_KEY_PRESS_SHORT_TIMEOUT) {
        key_idle_flag = 2;
    }
}
/*
 * 功能描述：短按标志位获取函数
 * 入参说明：key_id - 按键ID
 * 返 回 值：无
 */
uint8_t key_get_short_flag(uint8_t key_id) {
    if (key_id >= KEY_CNT) return 0;
    uint8_t flag                  = KEY_TABLES[key_id].short_flag;
    KEY_TABLES[key_id].short_flag = 0;
    return flag;
}

/*
 * 功能描述：长按标志位获取函数
 * 入参说明：key_id - 按键ID
 * 返 回 值：无
 */
uint8_t key_get_long_flag(uint8_t key_id) {
    if (key_id >= KEY_CNT) return 0;
    uint8_t flags                = KEY_TABLES[key_id].long_flag;
    KEY_TABLES[key_id].long_flag = 0;
    return flags;
}
