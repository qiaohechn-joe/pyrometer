/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：camera.c
 * 文件描述：相机应用
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/3/10 张晓博 整合自乔鹤代码
 */
#include "ov5640.h"
#include "platform.h"
#include "agent.h"
#include "ostimer.h"
#include "tcp_server.h"
#include "camera.h"

static uint8_t pic_buff[PIC_BUF_SIZE] __attribute__((section(".PSRAM")));
static uint8_t seg_buff[2][DCI_DMA_BUF_SIZE] ALIGN(4);

camera_data_t camera_data = {
    .frame     = &pic_buff[0],
    .frame_len = 0,
    .capture   = 0,
    .state     = CAMERA_INIT,
    .seg[0]    = &seg_buff[0][0],
    .seg[1]    = &seg_buff[1][0],
};

void camera_task(void *arg);

/*
 * 功能描述：dci 帧中断回调
 * 入参说明：arg --- 入参
 * 返 回 值：无
 */
static void dci_frame_callback(void *arg) {
    uint8_t  id;
    uint8_t *desaddr;
    uint8_t *srcaddr;
    uint32_t num;

    num = dma_transfer_number_get(DMA1, DMA_CH7);
    num = DCI_DMA_TRANSFER_SIZE - num;
    num *= 4;

    id = dma_using_memory_get(DMA1, DMA_CH7);

    if (id == DMA_MEMORY_0) {
        srcaddr = camera_data.seg[0];
    } else if (id == DMA_MEMORY_1) {
        srcaddr = camera_data.seg[1];
    }

    desaddr = (uint8_t *)(camera_data.frame + camera_data.frame_len);
    mem_copy(desaddr, srcaddr, num);
    camera_data.frame_len += num;

    camera_data.capture = 1;
}

/*
 * 功能描述：dci dma中断回调
 * 入参说明：arg --- 入参
 * 返 回 值：无
 */
static void dci_dma_callback(void *arg) {
    uint8_t  id;
    uint8_t *desaddr;
    uint8_t *srcaddr;

    id = dma_using_memory_get(DMA1, DMA_CH7);

    if (id == DMA_MEMORY_0) {
        srcaddr = camera_data.seg[1];
    } else if (id == DMA_MEMORY_1) {
        srcaddr = camera_data.seg[0];
    }

    desaddr = (uint8_t *)(camera_data.frame + camera_data.frame_len);
    mem_copy(desaddr, srcaddr, DCI_DMA_BUF_SIZE);
    camera_data.frame_len += DCI_DMA_BUF_SIZE;
}

/*
 * 功能描述：搜索有效图像数据
 * 入参说明：buff：采集到的数据
 *           len：总数据长度
 *           head：有效数据头
 *           valid_len：有效数据长度
 * 返 回 值：无
 */
void search_valid_data(void *buff, uint32_t len, uint32_t *head, uint32_t *valid_len) {
    uint8_t  head_flag = 0;
    uint8_t *data;

    data = (uint8_t *)buff;

    for (uint32_t index = 0; index < len; index++) {
        if ((data[index] == 0xff) && (data[index + 1] == 0xD8)) {
            head_flag = 1;
            *head     = index;
        }

        if ((data[index] == 0XFF) && (data[index + 1] == 0XD9) && head_flag) {
            *valid_len = index - *head + 2;
            break;
        }
    }
}

/*
 * 功能描述：相机初始化
 * 入参说明：无
 * 返 回 值：无
 */
void camera_init(void) {
    gd32_dci_init(dci_frame_callback);
    gd32_dci_dma_init(DCI_DR_ADDRESS, (uint32_t)camera_data.seg[0], (uint32_t)camera_data.seg[1], DCI_DMA_TRANSFER_SIZE, dci_dma_callback);

    ostimer_register(OSTIMER_CAMERA_TASK, camera_task, (void *)0, AGENT_PRIO_HI, CAMERA_TASK_RATE_HZ);

    sys_state.camera_flag_open_send = 0; /* 默认关闭 */
}

/*
 * 功能描述：相机任务
 * 入参说明：无
 * 返 回 值：无
 * 备    注：51.7ms
 */
#if 1
static void camera_task(void *arg) {

    if (sys_state.camera_flag_open_send == 1) {
        /* 构建单帧边界头和图像内容 */
        const char *frame_header = "\r\n--boundarydonotcross\r\n"
                                   "Content-Type: image/jpeg\r\n"
                                   "Content-Length: %5d\r\n\r\n";
        char        header_buf[128];
        int         header_len = snprintf(header_buf, sizeof(header_buf), frame_header, camera_data.valid_len);

        /* 每帧发送数据 */
        memcpy(&camera_data.frame[camera_data.head - header_len], header_buf, header_len);
        tcp_server_send_low(sys_state.camera_client_num, (char *)&camera_data.frame[camera_data.head - header_len],
                            camera_data.valid_len + header_len);
    }
}
#else
static void camera_task(void *arg) {
    switch (camera_data.state) {
        case CAMERA_INIT:
            if (ov5640_init()) {
                sys_state.dev_error[DEV_ERR_OV5640] = DEV_ST_ERR;
                camera_data.state                   = CAMERA_INIT;
                ostimer_set_period(OSTIMER_CAMERA_TASK, 1000);
            } else {
                camera_data.state = CAMERA_CONFIG;
            }
            break;
        case CAMERA_CONFIG:
            ostimer_set_period(OSTIMER_CAMERA_TASK, CAMERA_TASK_RATE_HZ);

            taskENTER_CRITICAL();

            ov5640_jpeg_mode();
            ov5640_outsize_set(16, 4, 320, 240);   /* 输出分辨率 320 x 240 */
            ov5640_imagewin_set(0, 0, 2592, 1944); /* 全图采样窗口 */
            ov5640_set_quality(100);               /* 图像压缩质量，范围1~100，10左右压缩比高 */
#if 0 /* test模式 */
            ov5640_set_exposure(1); /*手动曝光*/
            ov5640_test_pattern(0); /*输出0：关闭1：彩条2：色块*/
#else
            ov5640_exposure(4); /* EV曝光补偿 */
#endif
            taskEXIT_CRITICAL();

            camera_data.state = CAMERA_START;
            break;
        case CAMERA_START:
            taskENTER_CRITICAL();
            gd32_dci_dma_start();
            gd32_dci_start();
            camera_data.state = CAMERA_PROCESS;
            taskEXIT_CRITICAL();
            break;

        case CAMERA_PROCESS:
            if (camera_data.capture) {
                AAA camera_data.capture = 0;
                /* 处理图像数据 */
                search_valid_data(camera_data.frame, camera_data.frame_len, &camera_data.head, &camera_data.valid_len);
                dbg_camera("frame len:%d,jpg head :%d,jpg len:%d\r\n", camera_data.frame_len, camera_data.head, camera_data.valid_len);

                /* 保存图像数据至全局变量 */
#if 0
                dbg_dump_camera("", &camera_data.frame[camera_data.head], camera_data.valid_len, 32);
#endif
                if (sys_state.camera_flag_open_send == 1) {
                    /* 构建单帧边界头和图像内容 */
                    const char *frame_header = "\r\n--boundarydonotcross\r\n"
                                               "Content-Type: image/jpeg\r\n"
                                               "Content-Length: %5d\r\n\r\n";
                    char        header_buf[128];
                    int         header_len = snprintf(header_buf, sizeof(header_buf), frame_header, camera_data.valid_len);

                    /* 每帧发送数据 */
                    memcpy(&camera_data.frame[camera_data.head - header_len], header_buf, header_len);
                    tcp_server_send_low(sys_state.camera_client_num, (char *)&camera_data.frame[camera_data.head - header_len],
                                        camera_data.valid_len + header_len);
                }

                /* 图像缓存清零 */
                camera_data.frame_len = 0;
                camera_data.fault     = 0;
                camera_data.state     = CAMERA_START;

                BBB
            } else {
                camera_data.fault++;
                if (camera_data.fault > 10) {
                    camera_data.fault = 0;
                    camera_data.state = CAMERA_INIT;
                }
            }
            break;
        default: break;
    }
}
#endif
