/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：comm_cfg.c
 * 文件描述：端口配置接口代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/02/13 李兆越 初始版本
 *           V1.1 2025/04/23 李兆越 加入按块发送函数
 *           V1.2 2025/05/12 李兆越 命名通用化与结构清晰化
 */

#include "platform.h"
#include "comm_cfg.h"
#include "agent.h"
#include "freertos.h"
#include "upgrade.h"

SemaphoreHandle_t comm_mgr_mutex = NULL;
comm_manager_t    comm_mgr; /* 全局发送管理器 */

/* 端口支持波特率 */
uint32_t PORT_BAUD_TAB[PORT_BAUD_CNT] = {9600, 19200, 38400, 57600, 115200};

int uart_group;
int tcp_group;

/*
 * 功能描述：初始化通信管理结构，添加各组及其成员
 * 入参说明：无
 * 返 回 值：无
 */
void comm_init(void) {
    memset(&comm_mgr, 0, sizeof(comm_mgr));

    pc_init();

    comm_mgr_mutex = xSemaphoreCreateMutex();
    if (comm_mgr_mutex == NULL) {
        return;
    }

    uart_group = comm_group_create("UART", pc_send_wrapper);
    comm_channel_add(uart_group, COM2);

    tcp_group = comm_group_create("TCP", tcp_server_send_wrapper);
}

/*
 * 功能描述：PC通信发送封装函数，根据成员编号调用pc_send函数发送数据
 * 入参说明：channel_idx --- 通信成员编号
 *          data        --- 要发送的数据指针
 *          len         --- 数据长度（字节）
 * 返 回 值：无
 */
void pc_send_wrapper(int channel_idx, const void *data, int len) {
    if (data == NULL || len <= 0) {
        return; /* 无效参数不发送 */
    }
    pc_send(channel_idx, data, len);
}

/*
 * 功能描述：TCP服务发送封装函数，根据成员编号调用tcp_server_send函数发送数据
 * 入参说明：channel_idx --- 通信成员编号
 *          data        --- 要发送的数据指针
 *          len         --- 数据长度（字节）
 * 返 回 值：无
 */
void tcp_server_send_wrapper(int channel_idx, const void *data, int len) {
    if (data == NULL || len <= 0) {
        return; /* 无效参数不发送 */
    }
    // tcp_server_send(channel_idx, data, len);
    ENTER_CRITICAL();
    tcp_server_send_low(channel_idx, data, len);
    EXIT_CRITICAL();
}

/*
 * 功能描述：根据 group 和 member 索引，按块发送数据
 * 入参说明：group_idx --- 通信组编号
 *          channel_idx --- 通信成员编号
 *          data --- 要发送的数据
 *          len --- 数据长度
 * 返 回 值：无
 */
void comm_send_chunk(int group_idx, int channel_idx, const void *data, int len) {
    if (group_idx >= comm_mgr.group_count) return;

    comm_group_t *group = &comm_mgr.groups[group_idx];
    if (group->send_func == NULL || group->channels == NULL || group->channel_count == 0) return;

    /* 查找成员是否存在该端口号 */
    bool found = false;
    for (int i = 0; i < group->channel_count; i++) {
        if (group->channels[i].port_id == channel_idx) {
            found = true;
            break;
        }
    }
    if (!found) return;

    /* 发送数据 */
    const char *buf = (const char *)data;
    while (len > 0) {
        int chunk = (len > 1056) ? 1056 : len;
        group->send_func(channel_idx, buf, chunk); /* 直接传端口号 tcp_group  */
        buf += chunk;
        len -= chunk;

        //        if (len > 0) {
        //            vTaskDelay(50); /* 经验延时值 */
        //        }
    }
}

/*
 * 功能描述：多端口协议处理函数
 * 入参说明：rx_buff --- 输入的原始数据
 *          tx_buff --- 输出有效数据的缓冲区
 *          inlen --- 输入有效数据的长度
 *          outlen --- 输出有效数据的长度
 *          group_idx --- 所属通信组编号
 *          channel_idx --- 组内成员编号
 * 返 回 值：无
 */
/* 当前处理命令的来源组：uart_group/tcp_group，用于跨模块判定来源 */
int g_cmd_src_group = -1;

void proto_process(char *rx_buff, char *tx_buff, int rx_len, int *tx_len, comm_port_t *comm_port) {
    if ((rx_buff[0] == PROT_SOP) && (rx_len >= sizeof(proto_base_head_t))) {
        base_pkt_process(rx_buff, rx_len, tx_buff, tx_len, comm_port);
    } else if (fw_info.status != UG_STATE_TRANS) {
        /* 记录当前命令来源供字符协议回调判断 */
        g_cmd_src_group = comm_port->group_idx;
        upp_pkt_process(rx_buff, rx_len, tx_buff, tx_len);
        g_cmd_src_group = -1;
    }
    /* 应答命令 */
    if (*tx_len > 0) {
        if (comm_port->group_idx == uart_group) {
            comm_send_chunk(comm_port->group_idx, comm_port->channel_idx, tx_buff, *tx_len);
        } else if (comm_port->group_idx == tcp_group) {
            // 网口发送  tcp_port.channel_idx = client->num;
            tcp_server_send_low(comm_port->channel_idx, tx_buff, *tx_len);
        }

        // dbg_info("[%s][port=%d] Send %d bytes\r\n", comm_mgr.groups[comm_port->group_idx].group_name, comm_port->channel_idx, *tx_len);

        mem_set(tx_buff, sizeof(tx_buff), 0x00);
        *tx_len = 0;
    }
}

/*
 * 功能描述：创建一个通信组
 * 入参说明：group_name --- 组名（如 "TCP"）
 *           send_func --- 该组的发送函数指针
 * 返 回 值：组在 comm_mgr.groups 中的索引，失败返回 -1
 */
int comm_group_create(const char *group_name, void (*send_func)(int, const void *, int)) {
    if (xSemaphoreTake(comm_mgr_mutex, portMAX_DELAY) == pdFALSE) return -1;

    int           new_count  = comm_mgr.group_count + 1;
    comm_group_t *new_groups = pvPortMalloc(sizeof(comm_group_t) * new_count);
    if (!new_groups) {
        xSemaphoreGive(comm_mgr_mutex);
        return -1;
    }

    /* 拷贝旧数据 */
    if (comm_mgr.groups) {
        memcpy(new_groups, comm_mgr.groups, sizeof(comm_group_t) * comm_mgr.group_count);
        vPortFree(comm_mgr.groups);
    }

    comm_mgr.groups          = new_groups;
    comm_group_t *new_group  = &comm_mgr.groups[comm_mgr.group_count];
    new_group->group_name    = group_name;
    new_group->send_func     = send_func;
    new_group->channel_count = 0;
    new_group->channels      = NULL;
    comm_mgr.group_count     = new_count;

    xSemaphoreGive(comm_mgr_mutex);
    return new_count - 1;
}

/*
 * 功能描述：向某个组中添加一个通信成员
 * 入参说明：group_idx --- 要添加成员的组索引
 *          name --- 成员名称
 * 返 回 值：成员索引，失败返回 -1
 */
int comm_channel_add(int group_idx, int port_id) {
    if (group_idx >= comm_mgr.group_count) return -1;
    if (xSemaphoreTake(comm_mgr_mutex, portMAX_DELAY) == pdFALSE) return -1;

    comm_group_t   *group        = &comm_mgr.groups[group_idx];
    int             new_count    = group->channel_count + 1;
    comm_channel_t *new_channels = pvPortMalloc(sizeof(comm_channel_t) * new_count);
    if (!new_channels) {
        xSemaphoreGive(comm_mgr_mutex);
        return -1;
    }

    /* 拷贝旧成员 */
    if (group->channels) {
        memcpy(new_channels, group->channels, sizeof(comm_channel_t) * group->channel_count);
        vPortFree(group->channels);
    }

    group->channels                               = new_channels;
    group->channels[group->channel_count].port_id = port_id;
    group->channel_count                          = new_count;

    xSemaphoreGive(comm_mgr_mutex);
    return port_id; /* 返回端口号作为成员索引 */
}

/*
 * 功能描述：从指定组中移除成员
 * 入参说明：group_idx --- 通信组编号
 *          channel_idx --- 要移除的成员索引
 * 返 回 值：成功返回0，失败返回-1
 * 注意事项：会重新调整成员数组并释放内存
 */
int comm_channel_remove(int group_idx, int port_id) {
    if (group_idx >= comm_mgr.group_count) return -1;
    if (xSemaphoreTake(comm_mgr_mutex, portMAX_DELAY) == pdFALSE) return -1;

    comm_group_t *group = &comm_mgr.groups[group_idx];

    int index = -1;
    for (int i = 0; i < group->channel_count; i++) {
        if (group->channels[i].port_id == port_id) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        xSemaphoreGive(comm_mgr_mutex);
        return -1;
    }

    for (int i = index; i < group->channel_count - 1; i++) {
        group->channels[i] = group->channels[i + 1];
    }

    group->channel_count--;
    if (group->channel_count == 0) {
        vPortFree(group->channels);
        group->channels = NULL;
    } else {
        comm_channel_t *new_channels = pvPortMalloc(sizeof(comm_channel_t) * group->channel_count);
        if (new_channels) {
            memcpy(new_channels, group->channels, sizeof(comm_channel_t) * group->channel_count);
            vPortFree(group->channels);
            group->channels = new_channels;
        }
    }

    xSemaphoreGive(comm_mgr_mutex);
    return 0;
}

/*
 * 功能描述：根据组名称查找组索引
 * 入参说明：group_name --- 要查找的组名称
 * 返 回 值：成功返回组索引，未找到返回-1
 */
int comm_group_find(const char *group_name) {
    for (int i = 0; i < comm_mgr.group_count; i++) {
        if (strcmp(comm_mgr.groups[i].group_name, group_name) == 0) return i;
    }
    return -1;
}
