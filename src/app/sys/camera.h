/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：camera.c
 * 文件描述：相机应用
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/3/10 张晓博 整合自乔鹤代码
 */

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "platform.h"

#define DCI_DR_ADDRESS        (0x50050028U)
#define DCI_DMA_TRANSFER_SIZE 1460
#define DCI_DMA_BUF_SIZE      (DCI_DMA_TRANSFER_SIZE * 4)
#define PIC_BUF_SIZE          (400 * 1200)

// #define CAMERA_TASK_RATE_HZ   (configTICK_RATE_HZ / 40) /* 25ms */
#define CAMERA_TASK_RATE_HZ   (configTICK_RATE_HZ / 20) /* 50ms */

enum camera_state {
    CAMERA_INIT,
    CAMERA_CONFIG,
    CAMERA_START,
    CAMERA_PROCESS,
};

typedef struct {
    uint8_t *frame;       /* 采集数据缓存区指针 */
    uint32_t frame_len;   /* 采集数据总长度 */
    uint8_t *seg[2];      /* 采集数据分段接收缓存区指针 */
    uint32_t head;        /* 有效像素数据头位置 */
    uint32_t valid_len;   /* 有效像素数据总长度 */
    uint8_t  capture : 1; /* 采集完成标志 */
    uint32_t fault;       /* 异常 */
    uint8_t  state;       /* 状态 */
} camera_data_t;

void                 camera_init(void);
extern camera_data_t camera_data;

#endif
