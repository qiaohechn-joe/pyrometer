/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：console.c
 * 文件描述：控制台管理代码
 * 作    者：和其光电嵌软团队
 * 备    注：使用时注意打开OS的配置项configUSE_IDLE_HOOK
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 */

#include "agent.h"
#include "ostimer.h"
#include "system.h"
#include "console.h"

/* 当前命令 */
static char curr_cmd[CON_CMD_BUFF_SIZE];
/* 历史命令 */
static char his_cmd[CON_CMD_HIS_CNT][CON_CMD_BUFF_SIZE];
static int  his_wr; /* 写指示 */
static int  his_rd; /* 读指示 */
static int  his_cnt;

/* 内部函数 */
static void work_thread(void);
static int  get_command(char *buff, int size);
static int  cmd_help(int argc, char **argv);
static int  cmd_ver(int argc, char **argv);
static int  cmd_default(int argc, char **argv);
static int  cmd_debug(int argc, char **argv);
static int  cmd_reboot(int argc, char **argv);
static int  cmd_date(int argc, char **argv);

/* 控制命令列表 */
BEGIN_COMMAND_TABLE(CON_CMD_TABLE)
COMMAND_ENTRY("help", cmd_help)
COMMAND_ENTRY("ver", cmd_ver)
COMMAND_ENTRY("default", cmd_default)
COMMAND_ENTRY("debug", cmd_debug)
COMMAND_ENTRY("reboot", cmd_reboot)
COMMAND_ENTRY("date", cmd_date)
END_COMMAND_TABLE

/*
 * 功能描述：这个函数初始化控制台单元
 * 入参说明：无
 * 返 回 值：0 -- 成功，其它 -- 失败
 */
int console_init(void) {
    int i;

    his_wr = his_rd = his_cnt = 0;
    for (i = 0; i < CON_CMD_HIS_CNT; i++) {
        his_cmd[i][0] = 0;
    }

    return 0;
}

/*
 * 功能描述：这个函数是OS_IdleTask()调用的钩子函数
 * 入参说明：无
 * 返 回 值：无
 */
void vApplicationIdleHook(void) {
    work_thread();
}

/*
 * 功能描述：这个函数是控制台的工作线程
 * 入参说明：无
 * 返 回 值：无
 * 备    注：由 OS_IdleTask() 调用
 */
static void work_thread(void) {
    int len;
    int ret;

    puts(CON_PROMPT);
    len = get_command(curr_cmd, sizeof(curr_cmd));
    if (len == 0) {
        return;
    }

    /* 保存至历史命令 */
    string_copy(his_cmd[his_wr], curr_cmd);
    his_wr++;
    if (his_wr == CON_CMD_HIS_CNT) {
        his_wr = 0;
    }
    if (his_cnt < CON_CMD_HIS_CNT) {
        his_cnt++;
    }
    his_rd = his_wr;

    /* 执行命令 */
    ret = command_execute(curr_cmd, CON_CMD_TABLE);
    if (ret == CMD_PARAMETER) {
        puts("invalid parameter!\r\n");
    } else if (ret == CMD_FAILED) {
        puts("execute failed!\r\n");
    }
}

/*
 * 功能描述：这个函数从控制台指定串口读取命令
 * 入参说明：buff --- 命令缓冲区指针
 *           size --- 命令缓冲区大小
 * 返 回 值：命令长度(以字节计)
 */
static int get_command(char *buff, int size) {
    int pos;
    int ch;

    pos = 0;
    for (;;) {
        ch = getch();

        if ((ch == '\r') || (ch == '\n')) { /* 命令结束 */
            puts("\r\n");
            break;
        } else if (ch == CH_ESC) { /* 历史 */
            ch = getch();
            if (ch == 0x5B) {
                ch = getch();
                if (ch == 0x41) { /* 1B 5B 41 : 向上 */
                    his_rd--;
                    if (his_rd < 0) {
                        his_rd = his_cnt - 1;
                    }
                } else if (ch == 0x42) { /* 1B 5B 42 : 向下 */
                    his_rd++;
                    if (his_rd == his_cnt) {
                        his_rd = 0;
                    }
                }

                if (his_cmd[his_rd][0]) {
                    /* 清除当前命令 */
                    putch('\r');
                    putch(' ');
                    while (pos-- != 0) {
                        putch(' ');
                    }

                    /* 加载历史命令 */
                    string_copy(buff, his_cmd[his_rd]);
                    pos = string_length(buff);
                    puts(CON_PROMPT);
                    puts(buff);
                }
            }
        } else if (ch == CH_DEL) { /* 回退 */
            if (pos > 0) {
                pos--;
                putch(CH_BKSPACE);
                putch(' ');
                putch(CH_BKSPACE);
            }
        } else {
            putch(ch);
            buff[pos++] = ch;
            if (pos == (size - 1)) {
                break;
            }
        }
    }

    buff[pos] = '\0';
    return pos;
}

/********************************************************************************************************
 *                                     命令处理模块
 ********************************************************************************************************/

/* 功能 : 显示帮助信息。
   参数 : 无。
*/
static int cmd_help(int argc, char **argv) {
    if (argc != 1) {
        return CMD_PARAMETER;
    }

    puts("ver       : Show hardware and firmware version\r\n");
    puts("default   : Restore system default settings\r\n");
    puts("debug     : Set/Get current debug level()\r\n");
    puts("reboot    : Reboot system\r\n");
    puts("date      : Set/Get RTC date and time\r\n");
    puts("sn        : Set/Get device SN\r\n");

    return CMD_SUCCESS;
}

/* 功能 : 设置/获取 软硬件版本信息。
   参数 : 无。
*/
static int cmd_ver(int argc, char **argv) {
    if (argc != 1) {
        return CMD_PARAMETER;
    }

    printf("HW: V%d.%d.%d\r\n", HW_VER_MAJOR, HW_VER_MINOR, HW_VER_SERIAL);
    printf("FW: V%d.%d.%d\r\n", version_major(SYS_VERSION.fw), version_minor(SYS_VERSION.fw), version_serial(SYS_VERSION.fw));

    return CMD_SUCCESS;
}

/* 功能 : 恢复默认系统参数。
   参数 : 无。
*/
static int cmd_default(int argc, char **argv) {
    if (argc != 1) {
        return CMD_PARAMETER;
    }

    puts("Restore default setting...");
    // para_set_to_dafault();
    puts("OK!\r\n");

    return CMD_SUCCESS;
}

/*
 * 功能 : 设置/获取调试级别。
 * 参数 : 调试级别字符串，如 "UPG,ERR" 或 "ALL"
 */
static int cmd_debug(int argc, char **argv) {
    if (argc > 2) {
        return CMD_PARAMETER;
    }

    if (argc == 1) { /* 获取 */
        printf("Current Debug Level: %s\r\n", sys_para.port.dbg_level);
    } else if (argc == 2) { /* 设置 */
        char *level_str = argv[1];

        if (strlen(level_str) >= sizeof(sys_para.port.dbg_level)) {
            return CMD_PARAMETER; /* 输入过长 */
        }

        puts("Set debug level...\r\n");

        ENTER_CRITICAL();
        dbg_set_level(level_str);
        pmm_save_group(PARA_PORT); /* 如果你希望保存到flash或EEPROM，可换为 PARA_PORT */
        EXIT_CRITICAL();

        puts("OK!\r\n");
    }

    return CMD_SUCCESS;
}

/* 功能 : 重启系统。
   参数 : 无。
*/
static int cmd_reboot(int argc, char **argv) {
    if (argc != 1) {
        return CMD_PARAMETER;
    }

    puts("Reboot system...\r\n");
    gd32_delay_ms(100);
    NVIC_SystemReset();

    return CMD_SUCCESS;
}

/* 功能 : 设置/获取日期和时间。
   参数 : ASCII字符串，示例 "YYYY-MM-DD  HH:MM:SS"。
*/
static int cmd_date(int argc, char **argv) {
    rtc_time_t time;
    int        val[6];

    if (argc > 3) {
        return CMD_PARAMETER;
    }

    if (argc == 1) { /* 获取 */
        ENTER_CRITICAL();
        gd32_rtc_get_time(&time);
        EXIT_CRITICAL();
        printf("%4d-%02d-%02d %02d:%02d:%02d\r\n", time.year, time.month, time.mday, time.hour, time.minute, time.second);
    } else if (argc == 2) { /* 设置日期 */
        puts("Set date...");
        sscanf(argv[1], "%d-%d-%d", &val[0], &val[1], &val[2]);
        if ((val[0] < 1970) || (val[0] > 2106)) {
            return CMD_PARAMETER;
        }
        if ((val[1] < 1) || (val[1] > 12)) {
            return CMD_PARAMETER;
        }
        if ((val[2] < 1) || (val[2] > 31)) {
            return CMD_PARAMETER;
        }

        ENTER_CRITICAL();
        gd32_rtc_get_time(&time);
        time.year  = val[0];
        time.month = val[1];
        time.mday  = val[2];
        gd32_rtc_set_time(&time);
        EXIT_CRITICAL();

        puts("OK!\r\n");
    } else if (argc == 3) { /* 设置日期和时间 */
        puts("Set date and time...");
        sscanf(argv[1], "%d-%d-%d", &val[0], &val[1], &val[2]);
        if ((val[0] < 1970) || (val[0] > 2106)) {
            return CMD_PARAMETER;
        }
        if ((val[1] < 1) || (val[1] > 12)) {
            return CMD_PARAMETER;
        }
        if ((val[2] < 1) || (val[2] > 31)) {
            return CMD_PARAMETER;
        }

        sscanf(argv[2], "%d:%d:%d", &val[3], &val[4], &val[5]);
        if ((val[3] < 0) || (val[3] > 23)) {
            return CMD_PARAMETER;
        }
        if ((val[4] < 0) || (val[4] > 59)) {
            return CMD_PARAMETER;
        }
        if ((val[5] < 0) || (val[5] > 59)) {
            return CMD_PARAMETER;
        }

        ENTER_CRITICAL();
        gd32_rtc_get_time(&time);
        time.year   = val[0];
        time.month  = val[1];
        time.mday   = val[2];
        time.hour   = val[3];
        time.minute = val[4];
        time.second = val[5];
        gd32_rtc_set_time(&time);
        EXIT_CRITICAL();

        puts("OK!\r\n");
    }

    return CMD_SUCCESS;
}
