/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：record_cfg.c
 * 文件描述：日志管理配置代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/12/20 陈军    初始版本
 *           V1.1 2025/03/07 李兆越  优化记录数量宏定义
 *           V1.2 2025/04/22 李兆越  加入日志异常处理
 */

#include "platform.h"
#include "record_cfg.h"
#include "storage.h"
#include "record.h"
#include "lwip.h"
#if SUPPORT_EEPROM
bbox_record_t  bbox_rec;
state_record_t state_rec;

void  bbox_record_task(void *arg);
void  state_record_task(void *arg);
char *bbox_rec_format(char *text, uint32_t num, void *buff);
char *state_rec_format(char *text, uint32_t num, void *buff);

/* 日志类型名称数组 */
const char *const REC_TYPE_NAME[] = {
    REC_TYPE_NAME_BBOX,
    REC_TYPE_NAME_STATE,
};
/* 日志标题数组 */
const char *const REC_TITLE[] = {
    REC_TITLE_BBOX,
    REC_TITLE_STATE,
};
/* 日志大小数组 */
const uint32_t REC_SIZE[] = {
    sizeof(bbox_record_t),
    sizeof(state_record_t),
};
/* 日志容量数组 */
const uint32_t REC_CAPACITY[] = {
    REC_BBOX_CNT,
    REC_STATE_CNT,
};
/* 日志起始地址数组 */
const uint32_t REC_START_ADDR[] = {
    REC_BBOX_ADDR,
    REC_STATE_ADDR,
};
/* 日志结束地址数组 */
const uint32_t REC_END_ADDR[] = {
    REC_BBOX_ADDR + REC_BBOX_CNT * sizeof(bbox_record_t),
    REC_STATE_ADDR + REC_STATE_CNT * sizeof(state_record_t),
};

/* 状态文本 */
static const char *const STATE_TEXT[] = {
    "Cold Start",
    "Warm Start",
};

#if SUPPORT_EEPROM
/*
 * 功能描述：这个函数用于保存黑盒日志
 * 入参说明：arg --- 入参
 * 返 回 值：无
 */
void bbox_record_task(void *arg) {
    unsigned int sec;

    arg = arg;

    /* 如果RTC异常，直接返回 */
    if (sys_state.dev_error[DEV_ERR_RECORD] == DEV_ST_ERR) return;

    /* 如果记录未停止，打印保存日志提示 */
    if (!record_is_stop(REC_TYPE_BBOX)) {
        dbg_record("Record: blackbox is saving... \r\n");
    } else {
        dbg_record("Record: blackbox record has stopped\r\n");
    }

    /* 这里保存日志实际内容，即填充rec */
    mem_set(&bbox_rec, sizeof(bbox_rec), 0);
    ENTER_CRITICAL();
    sec = sys_state.sys_sec;
    gd32_rtc_sec_to_time(sec, &bbox_rec.time);

    bbox_rec.dev_state  = 0x66;
    bbox_rec.net_status = 1;

    bbox_rec.xTotalHeapSize           = configTOTAL_HEAP_SIZE;
    bbox_rec.xFreeHeapSize            = xPortGetFreeHeapSize();
    bbox_rec.xMinimumEverFreeHeapSize = xPortGetMinimumEverFreeHeapSize();

    //  stats_display();  /* 打印所有 lwIP 的 mem 和 memp 使用情况 */
    EXIT_CRITICAL();

    record_save(REC_TYPE_BBOX, &bbox_rec);
}

/*
 * 功能描述：这个函数用于保存工作状态日志
 * 入参说明：arg --- 入参
 * 返 回 值：无
 */
void state_record_task(void *arg) {
    arg = arg;

    /* 如果RTC异常，直接返回 */
    if (sys_state.dev_error[DEV_ERR_RECORD] == DEV_ST_ERR) return;

    /* 这里保存日志实际内容，即填充rec */
    ENTER_CRITICAL();
    mem_set(&state_rec, sizeof(state_rec), 0);
    state_rec.time_sec    = gd32_rtc_get_second();
    state_rec.start_state = sys_state.dev_state;
    EXIT_CRITICAL();

    record_save(REC_TYPE_STATE, &state_rec);
}
#endif

/*
 * 功能描述：这个函数格式化一个黑匣子日志
 * 入参说明：text --- 指向格式化缓存区的指针
 *           num --- 日志编号
 *           buff --- 日志缓存区
 * 返 回 值：指向格式化缓存区中下一个写入位置的指针
 * 注意事项：不处理编号和时间戳；和黑盒标题项对应
 */
char *bbox_rec_format(char *text, uint32_t num, void *buff) {
    bbox_record_t *rec = (bbox_record_t *)buff;

    /* 序号 */
    text += sprintf(text, "%ld", (unsigned long)num);
    /* 当前时间 */
    text += sprintf(text, "%4d-%02d-%02d %02d:%02d:%02d,", rec->time.year, rec->time.month, rec->time.mday, rec->time.hour, rec->time.minute,
                    rec->time.second);
    /* 设备状态 */
    text += sprintf(text, "0x%02X,", rec->dev_state);
    /* 重启次数 */
    //    text += sprintf(text, "%02d,", rec->reboot_count);

    return text;
}

/*
 * 功能描述：这个函数格式化一个设备运行状态日志
 * 入参说明：text --- 指向格式化缓存区的指针
 *           num --- 日志编号
 *           buff --- 日志缓存区
 * 返 回 值：指向格式化缓存区中下一个写入位置的指针
 * 注意事项：不处理编号和时间戳；和状态标题项对应
 */
char *state_rec_format(char *text, uint32_t num, void *buff) {
    state_record_t *rec = (state_record_t *)buff;
    rtc_time_t      time;

    /* 序号 */
    text += sprintf(text, "%ld,", (unsigned long)num);
    /* 发生时间 */
    gd32_rtc_sec_to_time(rec->time_sec, &time);
    text += sprintf(text, "%4d-%02d-%02d %02d:%02d:%02d,", time.year, time.month, time.mday, time.hour, time.minute, time.second);
    /* 启动状态 */
    text += sprintf(text, "%s", STATE_TEXT[rec->start_state]);
    /* 环境温度 */
    //    text += sprintf(text, "%.2f,", rec->env_temp);

    return text;
}
#endif /* SUPPORT_EEPROM */
