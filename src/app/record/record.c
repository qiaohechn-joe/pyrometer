/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：record.c
 * 文件描述：日志管理代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/12/10 陈军    初始版本
 *           V1.1 2025/02/13 李兆越  停止状态设置优化；复位停止信息规范处理；存储日志范围限制；
 *           V1.2 2025/04/18 李兆越  黑盒日志采用软定时统一管理
 */

#include "storage.h"
#include "record.h"
#include "system.h"
#include "para.h"
#include "ostimer.h"

#if SUPPORT_EEPROM
/* 日志索引数组 */
record_index_t rec_index[REC_TYPE_CNT];

/* 停止信息 */
record_stop_t rec_stop[REC_TYPE_CNT];
/* 日志请求缓冲区：保存REC_REQ_MAX_COUNT条日志 */
static uint8_t rec_buffer[REC_REQ_MAX_COUNT * sizeof(bbox_record_t) + 32];

/* 日志保存地址数组 */
static uint32_t rec_addr[REC_TYPE_CNT];

static int load_index(record_index_t *idx);
static int save_index(record_index_t *idx);

#endif

#if REC_FS_OK
static int prepare_dir(void);
static int create_rec_file(const char *name);
/* 文件指针 */
static FIL *file;
#endif /* REC_FS_OK */

#if SUPPORT_EEPROM
/*
 * 功能描述：这个函数初始化日志单元
 * 入参说明：无
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int record_init(void) {
    record_index_t *idx;
    int             i;
    int             ret = 0;

    if (sys_para.record.interval < REC_INTVAL_MIN) {
        dbg_error("Record interval too small: %d\n", sys_para.record.interval);
        return -1;
    }

    /* 注册黑盒日志记录任务 */
    ostimer_register(OSTIMER_RECORD_TASK, bbox_record_task, (void *)0, AGENT_PRIO_LO, sys_para.record.interval);

    /* 复位清除停止信息结构体 */
    for (int i = 0; i < REC_TYPE_CNT; i++) {
        rec_stop[i].start    = 0; /* 初始时间戳为0 */
        rec_stop[i].duration = 0; /* 默认不停止（0xFFFFFFFF为 ~0） */

        if (i == REC_TYPE_BBOX) {
            rec_stop[i].type = REC_TYPE_BBOX; /* 黑盒日志类型 */
        } else if (i == REC_TYPE_STATE) {
            rec_stop[i].type = REC_TYPE_STATE; /* 工作状态日志类型 */
        } else {
            rec_stop[i].type = 0xFF; /* 如果有其它类型，设置为无效类型 */
        }
    }

    for (idx = &rec_index[0], i = 0; i < REC_TYPE_CNT; i++, idx++) {
        if (load_index(idx) != 0) { /* 失败 */
            mem_set(idx, sizeof(*idx), 0x00);
            dbg_record("Record: index of record_%d load failed!\r\n", i);
            ret                                 = -1;
            sys_state.dev_error[DEV_ERR_RECORD] = DEV_ST_ERR;
        }

        rec_addr[i] = REC_START_ADDR[i] + (idx->tail_num % REC_CAPACITY[i]) * REC_SIZE[i]; /* 地址 */
    }

    return ret;
}

/*
 * 功能描述：这个函数清除所有日志
 * 入参说明：无
 * 返 回 值：无
 */
void record_clear(void) {
    record_index_t *idx;
    int             i;

    for (idx = &rec_index[0], i = 0; i < REC_TYPE_CNT; i++, idx++) {
        mem_set(idx, sizeof(*idx), 0x00);
        save_index(idx);
        rec_addr[i] = REC_START_ADDR[i];
        dbg_record("Record: index of record_%d clear!\r\n", i);
    }
}

/*
 * 功能描述：这个函数获取某种类型日志的存储信息
 * 入参说明：type --- 日志类型，REC_TYPE_xxx
 *           info --- 指向log_info结构体缓存区的指针
 * 返 回 值：无
 */
void record_get_info(int type, record_info_t *info) {
    record_index_t *idx;

    info->size     = REC_SIZE[type];
    info->capacity = REC_CAPACITY[type];
    info->name     = REC_TYPE_NAME[type];
    info->title    = REC_TITLE[type];

    idx = &rec_index[type];
    ENTER_CRITICAL();
    info->first_num = idx->head_num;                                  /* 第一个日志编号 */
    info->last_num  = (idx->tail_num == 0) ? 0 : (idx->tail_num - 1); /* 最后一个日志编号 */
    info->count     = idx->count;
    EXIT_CRITICAL();
}

/*
 * 功能描述：这个函数获取某种类型日志的停止状态
 * 入参说明：type --- 日志类型，REC_TYPE_xxx
 * 返 回 值：1 --- 已停止 其他 --- 未停止
 */
int record_is_stop(int type) {
    record_stop_t info;
    uint32_t      now_sec;
    int           res = 0; /* 默认不停止 */

    info    = rec_stop[type];
    now_sec = sys_state.sys_sec;

    if (info.duration == ~0) { /* 永久停止 */
        res = 1;
    } else if (info.duration > 0) {
        if (now_sec < (info.start + info.duration)) {
            res = 1;
        }
    }

    return res;
}

/*
 * 功能描述：这个函数读取一条日志（不包括编号）
 * 入参说明：type --- 日志类型，REC_TYPE_xxx
 *           num --- 日志编号
 *           buff --- 指向缓存区的指针
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int record_read(int type, unsigned long num, void *buff) {
    uint32_t addr;
    int      ret;

    addr = REC_START_ADDR[type] + (num % REC_CAPACITY[type]) * REC_SIZE[type];
    ret  = eeprom_read_lock(addr, buff, REC_SIZE[type]);

    return (ret == REC_SIZE[type]) ? 0 : -1;
}

/*
 * 功能描述：这个函数从存储介质加载多条日志到缓存区（尽量填满缓存区）
 * 入参说明：type --- 日志类型，REC_TYPE_xxx
 *           start --- 开始编号
 *           count --- 要加载的日志数量
 *           buff --- 指向缓存区的指针
 *           size --- 缓存区大小（字节数）
 * 返 回 值：成功加载的日志数量
 * 备    注：注意存储介质类型
 */
int record_load(int type, uint32_t start, int count, void *buff, int size) {
    uint8_t *data;
    uint32_t num;
    uint32_t addr;
    int      ret;
    ENTER_CRITICAL();
    if (count > rec_index[type].count) {
        count = rec_index[type].count;
    }
    EXIT_CRITICAL();
    if (count == 0) {
        return 0; /* 无日志信息可读 */
    }
    data = (uint8_t *)buff;
    num  = start;
    addr = REC_START_ADDR[type] + (num % REC_CAPACITY[type]) * REC_SIZE[type]; /* 起始地址 */
    while (size >= REC_SIZE[type]) {                                           /* 空间足够 */
        ret = eeprom_read_lock(addr, data, REC_SIZE[type]);                    /* 读取一条日志 */
        if (ret != REC_SIZE[type]) {
            break;
        }
        addr += REC_SIZE[type];
        if (addr == REC_END_ADDR[type]) {
            addr = REC_START_ADDR[type];
        }
        data += REC_SIZE[type];
        size -= REC_SIZE[type];
        num++;
        if ((num - start) == count) {
            break;
        }
    }
    return (num - start);
}

/*
 * 功能描述：这个函数保存一条日志
 * 入参说明：type --- 日志类型，REC_TYPE_xxx
 *           buff --- 日志数据缓存区
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int record_save(int type, void *data) {
    record_index_t *idx = &rec_index[type];
    uint32_t        addr;
    int             len; /* 成功写入eeprom的字节数 */

    /* 保存日志 */
    ENTER_CRITICAL();
    addr = rec_addr[type];
    EXIT_CRITICAL();
    len = eeprom_write_lock(addr, data, REC_SIZE[type]);
    dbg_record("Record: %s saved ok, %d B\r\n", REC_TYPE_NAME[type], len);
    if (len != REC_SIZE[type]) {
        return -1;
    }

    /* 修改索引 */
    ENTER_CRITICAL();
    idx->tail_num++;
    if (idx->tail_num >= REC_CAPACITY[type]) {
        idx->head_num++; /* 覆盖旧日志 */
    }
    if (idx->count < REC_CAPACITY[type]) {
        idx->count++;
    }
    rec_addr[type] += REC_SIZE[type];
    if (rec_addr[type] == REC_END_ADDR[type]) {
        rec_addr[type] = REC_START_ADDR[type];
    }
    EXIT_CRITICAL();

    if (save_index(idx)) {
        return -1; /* 保存索引 */
    }

    return 0;
}

/*
 * 功能描述：这个函数格式化一条日志（包括编号）
 * 入参说明：text --- 指向格式化文本缓存区的指针
 *           type --- 日志类型，REC_TYPE_xxx
 *           num --- 日志编号
 *           buff --- 指向日志缓存区的指针
 * 返 回 值：无
 */
void record_format(char *text, int type, uint32_t num, void *buff) {
    switch (type) {
        case REC_TYPE_BBOX: text = bbox_rec_format(text, num, buff); break;
        case REC_TYPE_STATE: text = state_rec_format(text, num, buff); break;
        default: break;
    }

    /* 终结符 */
    string_copy(text, "\r\n");
}

/*
 * 功能描述：这个函数从存储介质加载日志索引
 * 入参说明：idx --- 指向rec_index结构体的指针
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
static int load_index(record_index_t *idx) {
    int      ret;
    uint32_t offset;
    uint16_t crc_calc;

    offset = (uint8_t *)idx - (uint8_t *)(&rec_index[0]);

    ret = eeprom_read_lock(REC_INDEX_ADDR + offset, idx, sizeof(*idx));
    if (ret == sizeof(*idx)) {
        crc_calc = crc16_RTU(idx, sizeof(*idx) - 2);
        if (crc_calc == idx->crc) {
            return 0;
        }
    }

    return -1;
}

/*
 * 功能描述：这个函数将日志索引保存到存储介质
 * 入参说明：idx --- 指向rec_index结构体的指针
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
static int save_index(record_index_t *idx) {
    uint32_t offset;

    ENTER_CRITICAL();
    idx->crc = crc16_RTU(idx, sizeof(*idx) - 2);
    EXIT_CRITICAL();

    offset = (uint8_t *)idx - (uint8_t *)(&rec_index[0]);
    eeprom_write_lock(REC_INDEX_ADDR + offset, idx, sizeof(*idx));

    return 0;
}

/*
 * 功能描述：这个函数处理 CMD_REC_GET 命令
 * 入参说明：cmd --- 命令码
 *           op --- 操作类型，PROT_OP_xxx
 *           data --- 指向数据包负载的指针
 *           len --- 负载长度
 *           pkt --- 输出数据包缓存区
 * 返 回 值：输出数据包的长度
 */
int cmd_get_record(int cmd, int op, void *data, int len, void *pkt) {
    record_info_t info;
    record_num_t  num;
    record_req_t  req;
    record_req_t *reply;
    uint8_t      *ptr;
    uint32_t      count;
    int           ret = 0;
    uint8_t      *pos = (uint8_t *)data;

    if ((op == PROT_OP_RD) && (len == 1)) { /* 获取日志编号 */
        if (*pos == REC_TYPE_BBOX) {
            record_get_info(REC_TYPE_BBOX, &info);
            num.type  = REC_TYPE_BBOX;
            num.first = info.first_num;
            num.last  = info.last_num;
        } else if (*pos == REC_TYPE_STATE) {
            record_get_info(REC_TYPE_STATE, &info);
            num.type  = REC_TYPE_STATE;
            num.first = info.first_num;
            num.last  = info.last_num;
        }

        // ret = proto_base_build_pkt(pkt, DEV_ID, cmd, op, &num, sizeof(num));
    } else if ((op == PROT_OP_RD2) && (len == sizeof(req))) { /* 获取日志内容 */
        mem_copy(&req, data, sizeof(req));
        record_get_info(req.type, &info);

        if ((req.count > 0) && (req.count <= REC_REQ_MAX_COUNT) && (req.first > 0)) {
            //            req.first--;
            ptr = rec_buffer + sizeof(req);                     /* 跳过first_num和count */
            if (req.count > info.count) req.count = info.count; /* 取(info.count,req.count)的最小值 */
            for (count = 0; count < req.count; count++)         /* 读取日志 */
            {
                if (record_read(req.type, req.first + count, ptr)) break;
                ptr += info.size;
            }

            /* 填充first_num和count */
            reply        = (record_req_t *)rec_buffer;
            reply->type  = req.type;
            reply->first = req.first;
            reply->count = count;
            // ret          = proto_base_build_pkt(pkt, DEV_ID, cmd, op, rec_buffer, sizeof(req) + info.size * count);
        }
    }

    return ret;
}

/*
 * 功能描述：这个函数处理 CMD_REC_STOP 命令
 * 入参说明：cmd --- 命令码
 *           op --- 操作类型，PROT_OP_xxx
 *           data --- 指向数据包负载的指针
 *           len --- 负载长度
 *           pkt --- 输出数据包缓存区
 * 返 回 值：输出数据包的长度
 */
int cmd_stop_record(int cmd, int op, void *data, int len, void *pkt) {
    record_stop_t info_bsp = *(record_stop_t *)data;
    record_stop_t info;
    int           ret = 0;

    if ((op == PROT_OP_RD) && (len == 1)) { /* 获取停止时间 */
        ENTER_CRITICAL();
        info = rec_stop[info_bsp.type];
        EXIT_CRITICAL();
        // ret = proto_base_build_pkt(pkt, DEV_ID, cmd, op, &info, sizeof(info));
    } else if ((op == PROT_OP_WR) && (len == sizeof(record_stop_t))) { /* 设置停止时间 */

        ENTER_CRITICAL();
        rec_stop[info_bsp.type] = info_bsp; /* 开始时间戳以上位机为准 */
        EXIT_CRITICAL();
        // ret = proto_base_build_pkt(pkt, DEV_ID, cmd, op, (void *)&info_bsp.type, sizeof(info_bsp.type));
    }

    return ret;
}

/*
 * 功能描述：这个函数处理 CMD_REC_TIME 命令
 * 入参说明：cmd --- 命令码
 *           op --- 操作类型，PROT_OP_xxx
 *           data --- 指向数据包负载的指针
 *           len --- 负载长度
 *           pkt --- 输出数据包缓存区
 * 返 回 值：输出数据包的长度
 */
int cmd_time_record(int cmd, int op, void *data, int len, void *pkt) {
    rtc_time_t time_bsp = *(rtc_time_t *)data;
    rtc_time_t time;
    int        ret = 0;
    char       err = 1;

    if ((op == PROT_OP_RD) && (len == 0)) { /* 获取时间戳 */
        gd32_rtc_get_time(&time);
        // ret = proto_base_build_pkt(pkt, DEV_ID, cmd, op, &time, sizeof(rtc_time_t));
    } else if ((op == PROT_OP_WR) && (len == sizeof(rtc_time_t))) { /* 设置时间戳 */
        if (gd32_rtc_set_time(&time_bsp)) {
            err = 0;
        }
        // ret = proto_base_build_pkt(pkt, DEV_ID, cmd, op, (void *)&err, sizeof(err));
        record_clear();
    }

    return ret;
}
#endif

#if REC_FS_OK
/*
 * 功能描述：这个函数将日志通过USB导出，比如导出到U盘
 * 入参说明：无
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int record_export_usb(void) {
    static char   buff[256]; /* 大小 > 最大日志大小 */
    FRESULT       res;
    int           error;
    int           type;
    int           total;
    int           count;
    int           num;
    int           success;
    uint32_t      len;
    record_info_t rec_info;
    const char   *title;

    /* 总日志数 */
    for (total = 0, type = 0; type < REC_TYPE_CNT; type++) {
        record_get_info(type, &rec_info);
        total += rec_info.count;
    }

    if (total == 0) {
        dbg_record("REC : No records found!\r\n");
        return -1;
    }

    /* 准备目录 */
    if (prepare_dir()) {
        dbg_record("REC : Create directory failed!\r\n");
        return -1;
    }

    /* 开始导出 */
    file    = 0;
    success = 0;
    error   = 0;
    for (type = 0; type < REC_TYPE_CNT; type++) /* 每个日志类型 */
    {
        record_get_info(type, &rec_info);
        count = rec_info.count;
        num   = rec_info.first_num;
        dbg_record("REC : export type %d, count=%d\r\n", type, count);

        /* 创建文件 */
        if (create_rec_file(REC_TYPE_NAME[type])) {
            dbg_record("REC : Can\'t create file \"%s\"!\r\n", REC_TYPE_NAME[type]);
            error = 1;
            goto over;
        }

        /* 写标题 */
        title = rec_info.title;
        vTaskSuspendAll();
        len = f_puts(title, file);
        xTaskResumeAll();
        if (len != string_length(title)) {
            dbg_record("REC : write title failed!\r\n");
            error = 1;
            goto over;
        }

        while (count != 0) /* 每个日志 */
        {
            /* 读一条日志 */
            record_read(type, num, buff);

            /* 格式化日志并写入文件 */
            record_format((char *)udisk_buffer, type, num + 1, buff);

            vTaskSuspendAll();
            res = f_lseek(file, file->fsize); /* 移到文件结尾 */
            if (res != FR_OK) {
                dbg_record("%s() : write record failed!\r\n", __FUNCTION__);
                error = 1;
                xTaskResumeAll();
                goto over;
            }

            f_write(file, udisk_buffer, string_length((const char *)udisk_buffer), &len);
            xTaskResumeAll();
            if (len != string_length((const char *)udisk_buffer)) {
                dbg_record("%s() : write record failed!\r\n", __FUNCTION__);
                error = 1;
                goto over;
            }

            num++; /* 下一条日志 */
            count--;
            success++;
            // dbg_record("num=%d,count=%d,success=%d\r\n",num,count,success);
        }
    }

over:

    /* close file,cleanup udisk */
    file = udisk_close_file(file);
    if (file != 0) error = 1;

    if (error) {
        dbg_printf("REC : U-disk operation failed!\r\n");
        return -1;
    } else {
        dbg_printf("REC : Export over, %d records completed.\r\n", success);
        return 0;
    }
}

/*
 * 功能描述：这个函数创建并切换到日志目录
 * 入参说明：无
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
static int prepare_dir(void) {
    FRESULT res;
    char    buff[32];

    /* 创建并切换到第一层目录 */
    vTaskSuspendAll();
    res = f_mkdir(UDISK_WORK_DIR);
    xTaskResumeAll();
    if ((res != FR_OK) && (res != FR_EXIST)) goto error;
    vTaskSuspendAll();
    res = f_chdir(UDISK_WORK_DIR);
    xTaskResumeAll();
    if (res != FR_OK) goto error;

    /* 创建并切换到第二层目录 */
    ENTER_CRITICAL();
    sprintf(buff, "%06d", para_blk.sys_id);
    EXIT_CRITICAL();

    vTaskSuspendAll();
    res = f_mkdir(buff);
    xTaskResumeAll();
    if ((res != FR_OK) && (res != FR_EXIST)) goto error;
    vTaskSuspendAll();
    res = f_chdir(buff);
    xTaskResumeAll();
    if (res != FR_OK) goto error;

    return 0;

error:

    return -1;
}

/*
 * 功能描述：这个函数在U盘上创建日志.CSV文件
 * 入参说明：name --- 日志类型名称
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
static int create_rec_file(const char *name) {
    char buff[32];

    /* 关闭旧文件 */
    file = udisk_close_file(file);
    if (file != 0) goto error;

    /* 创建新文件 */
    sprintf(buff, "%s.csv", name);
    file = udisk_open_file(buff, FA_WRITE | FA_CREATE_ALWAYS);
    if (file == 0) goto error;

    return 0;

error:

    return -1;
}
#endif /* REC_FS_OK */
