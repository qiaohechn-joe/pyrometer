/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：burst.c
 * 文件描述：突发串驱动代码
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：V1.0 2024/01/26 陈军  初始版本
 */

#include "burst.h"
#include "lwip.h"

__align(4) static char burst_tx_buffer[BURST_BUF_LEN]; /* 突发串数据包缓存 */
static char             separator[2];
static volatile uint8_t burst_tcp_enabled;
static int              burst_build(const char *cmd_list, char *outbuf);

void burst_set_tcp_enable(bool en) {
    burst_tcp_enabled = en ? 1 : 0;
}
void burst_clear_tcp_enable(void) {
    burst_tcp_enabled = 0;
}

/*
 * 功能描述：定时器4中断函数
 * 入参说明：无
 * 返 回 值：无
 * 备    注：100us
 */
void timer3_handler(void) {
    UBaseType_t isr_status = taskENTER_CRITICAL_FROM_ISR();
    if (timer_flag_get(TIMER3, TIMER_FLAG_UP) == SET) {
        timer_flag_clear(TIMER3, TIMER_FLAG_UP); /* ack irq */
        agent_post(FROM_ISR, AGENT_PRIO_MD, burst_send_task, (void *)0);
    }
    taskEXIT_CRITICAL_FROM_ISR(isr_status);
}

/*
 * 功能描述：这个函数用于突发轮询初始化
 * 入参说明：无
 * 返 回 值：无
 */
void burst_init(void) {
    /* 定时器3（用于AD电压采集） 95us  */
    int timer_clk = gd32_timer_clk_freq(TIMER3);
    gd32_timer_init(TIMER3, timer_clk / 4000, 4 * sys_para.burst.speed); // 主频200MHz
    gd32_setup_isr(TIMER3_IRQn, timer3_handler, 0);
    gd32_setup_irq(TIMER3_IRQn, 15);

    timer_enable(TIMER3);
    timer_event_software_generate(TIMER3, TIMER_EVENT_SRC_UPG);
    timer_flag_clear(TIMER3, TIMER_FLAG_UP);
    timer_interrupt_enable(TIMER3, TIMER_INT_UP);

    // 初始化分隔符
    if (sys_para.burst.fmt[0] == 'S') {
        separator[0] = ' ';
    } else if (sys_para.burst.fmt[0] == 'N') {
        separator[0] = '\0';
    } else {
        separator[0] = sys_para.burst.fmt[0];
    }

    if (sys_para.burst.fmt[1] == 'S') {
        separator[1] = ' ';
    } else if (sys_para.burst.fmt[1] == 'N') {
        separator[1] = '\0';
    } else {
        separator[1] = sys_para.burst.fmt[1];
    }

    burst_clear_tcp_enable();
}

/*
 * 功能描述：检查命令列表是否合法
 * 入参说明：命令列表
 * 返 回 值：ture：格式合法， false：格式错误
 */
bool valid_cmd_list(const char *str) {
    if (!str || *str == '\0') return false;

    int  cmd_len     = 0;     // 当前命令长度
    bool has_command = false; // 是否至少有一个合法命令

    while (1) {
        char c = *str++;

        if (c == '\r' || c == '\0') {                       // 解析结束
            if (cmd_len == 0 && !has_command) return false; // 没有任何命令
            return cmd_len != 0;                            // 最后一个命令不能以逗号结尾
        }

        if (c == ',') {
            if (cmd_len == 0) return false; // 不允许连续逗号或开头就是逗号
            cmd_len     = 0;                // 新命令开始
            has_command = true;
            continue;
        }

        if (!isupper((unsigned char)c)) return false; // 必须是大写字母

        cmd_len++;
        if (cmd_len > 4) return false; // 命令长度不能超过4
    }
}

/*
 * 功能描述：该函数用于突发串发送。（周期性调用）
 * 入参说明：arg --- 入参。
 * 返 回 值：0 --- 成功  其他 --- 失败
 */
// todo 待优化采用dma发送方式，突发串执行耗时约4.5ms
void burst_send_task(void *arg) {
    int len;
    if (sys_para.burst.mode == BURST_MOD_B) {
        len = burst_build(sys_para.burst.cmd_list, burst_tx_buffer);
        if (len > 0) {
            pc_send(PC_COM, burst_tx_buffer, len); // todo port 改用变量
            /* 网络状态在线并且 TCP 使能时再尝试 TCP 发送，避免无谓占用 */
            if (burst_tcp_enabled && netif_is_up(netif_default)) {
                (void)tcp_server_send_low(WORK_CLIENT_BASE, burst_tx_buffer, len);
            }
        }
        mem_set(burst_tx_buffer, sizeof(burst_tx_buffer), 0);
    }
}

/*
 * 功能描述：该函数用于突发串创建
 * 入参说明：cfg --- 配置字符串
 *           outbuf --- 突发串输出缓冲区
 *           outlen --- 输出缓冲区的长度
 * 返 回 值：突发串总长度
 */
static int burst_build(const char *cmd_list, char *outbuf) {
    uint8_t outlen  = 0;
    int8_t  cmd_idx = -1;
    uint8_t cmd_len = 0;

    while (*cmd_list) {
        const char *pos = strchr(cmd_list, ',');
        cmd_len         = (pos != NULL) ? pos - cmd_list : strlen(cmd_list);

        cmd_idx = cmd_found(CMD_TBL, cmd_list, cmd_len);
        if (cmd_idx == -1 || !(CMD_TBL[cmd_idx].att.props & B)) {
            outlen += sprintf(outbuf + outlen, "error");
            if (separator[1] != '\0') {
                outbuf[outlen++] = separator[1];
            }
        } else {
            outlen += sprintf(outbuf + outlen, "%s", CMD_TBL[cmd_idx].cmd);
            if (separator[0] != '\0') {
                outbuf[outlen++] = separator[0];
            }

            /*增加单位转换*/
            float temp = 0;
            if (is_need_conver_unit(CMD_TBL[cmd_idx].cmd)) {

                if (CMD_TBL[cmd_idx].rule.type == FMT_F || CMD_TBL[cmd_idx].rule.type == FMT_F6) {
                    memcpy(&temp, CMD_TBL[cmd_idx].arg, sizeof(float));
                }
                /*进行单位转换*/
                temp = convert_uint(temp, TEMP_UNIT_INTERNAL, sys_para.temp.unit);
                outlen += data_to_string((void *)&temp, CMD_TBL[cmd_idx].rule.type, outbuf + outlen);
            } else {
                /*不需要单位转换*/
                outlen += data_to_string(CMD_TBL[cmd_idx].arg, CMD_TBL[cmd_idx].rule.type, outbuf + outlen);
            }

            if (separator[1] != '\0') {
                outbuf[outlen++] = separator[1];
            }
        }
        if (*(cmd_list + cmd_len) == '\0') {
            break;
        }
        cmd_list += cmd_len + 1;
    }

    /* 添加结束符 */
    sprintf(outbuf + outlen - 1, "%s", "\r\n");
    outlen++;
    return outlen;
}
