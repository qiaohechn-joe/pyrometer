#ifndef _SELF_CHECK_H
#define _SELF_CHECK_H

#include "platform.h"
#include <stdbool.h>
#include <stdint.h>
/*设备硬件自检错误信息*/
#define IS_USE_SELF_CHECK            0

#define EEPROM_CHECK_ADDRESS         131050
#define EEPROM_CHECK_LEN             1

#define PERIPHERAL_ERR_STATUS_OK     0
#define PERIPHERAL_ERR_STATUS_ERR    1
#define PERIPHERAL_ERR_STATUS_UNKNOW 3
#define TIME_OUT                     0xFFF0

/*设备工作状态测量错误信息*/
// 错误状态位定义（从高优先级到低优先级）
typedef enum {
    ERR_DETECTOR_FAULT = 0x0001, // EHHH
    ERR_HEATER_HIGH    = 0x0002, // ECHH
    ERR_HEATER_LOW     = 0x0004, // ECUU
    ERR_INTERNAL_HIGH  = 0x0008, // EIHH
    ERR_INTERNAL_LOW   = 0x0010, // EIUU
    ERR_ENERGY_LOW     = 0x0020, // EUUU
    ERR_ATTEN_HIGH     = 0x0040, // EAAA
    // ERR_DIRTY_WINDOW   = 0x0080, // 脏污窗口（衰减>95%）
    ERR_NORMAL = 0x0000 // 正常
} ErrorFlag_t;

// 错误状态字结构
typedef hq_packed struct {
    uint16_t error_flags; // 错误标志位
    uint8_t  error_index; // 错误标志位
    float    temperature; // 当前温度值（如果有）
    char     err_text[10];
    bool     relay_state;      // true=正常，false=异常
    float   *analog_output[2]; // 模拟输出电流值
} ErrorStatus_t;

void dev_self_check(void);
void dbg_self_check_info(void);
void Extern_Triger_GPIO_config(void);

/*测量错误字相关*/
#define ERROR_ALL_CNT 7
void error_state_set(ErrorFlag_t err_flag);
void error_state_clear(ErrorFlag_t err_flag);
int  get_highest_error_index(void);
void error_state_upgrade(void);

#endif
