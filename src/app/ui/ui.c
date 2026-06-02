/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：ui.c
 * 文件描述：ui菜单界面管理
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/03/19 李兆越 初始版本
 *           V1.1 2025/09/04 张赛   重做ui控制操作逻辑
 */

#include "ui.h"
#include "ostimer.h"
#include "system.h"
#include "para.h"
#include "agent.h"
#include "upgrade.h"
#include "message.h"
#include "display.h"
#include "oled.h"
#include "tm1639.h"
#include "key.h"

/* 菜单项结构体定义 */
struct menu_item_t {
    char  menu_count;               /* 当前菜单层级的项目总数 */
    char *display_string;           /* 在OLED上显示的菜单文本 */
    void (*subs)();                 /* 功能函数指针 */
    struct menu_item_t *son_ms;     /* 子菜单指针 */
    struct menu_item_t *dad_ms;     /* 父菜单指针 */
    int                 para_num;   /* 界面参数个数 */
    int                 para_index; /* 界面参数索引 */
};

/* 菜单项和子菜单结构体声明 */
struct menu_item_t m0_root[20];       /* main主菜单 */
struct menu_item_t m1_infomation[20]; /* 信息菜单 */
struct menu_item_t m1_base_cfg[20];   /* 设备配置菜单 */
struct menu_item_t m1_para_cfg[20];   /* 测温配置子菜单 */
struct menu_item_t m1_interface[20];  /* 接口菜单 */
struct menu_item_t m1_analog[20];     /* 模拟量菜单 */

/* 当前菜单指针 */
struct menu_item_t *current_menu = m0_root;

/* 函数前置声明 */
void menu_navigate(void); /* 导航菜单 */

/* clang-format off */
/* 主菜单初始化 (LEVEL 0) */
struct menu_item_t m0_root[] = {
    { 5, "1 INFORMATION",   oled_show_menu,  m1_infomation,   NULL , 0, 0},
    { 5, "2 CONFIGURATION", oled_show_menu,  m1_base_cfg,     NULL , 0, 0},
    { 5, "3 PARA CFG",    oled_show_menu,  m1_para_cfg,       NULL , 0, 0},
    { 5, "4 INTERFACE",    oled_show_menu,  m1_interface,     NULL , 0, 0},
    { 5, "5 ANALOG",    oled_show_menu,  m1_analog,     NULL , 0, 0},
};

/* INFORMATION 子菜单初始化 (LEVEL 1) */
struct menu_item_t m1_infomation[] = {
    { 9, "1 CONDENSEN INFOS ", oled_show_info,NULL,m0_root , 0, 0},
    { 9, "2 INTERNAL TEMP",    oled_show_info,NULL,m0_root , 0, 0},/* 内部温度 */
    { 9, "3 ATTENUATION",      oled_show_info,NULL,m0_root , 0, 0},/* 衰减比率 */
    { 9, "4 LOW_LIMIT",        oled_show_info,NULL,m0_root , 0, 0},/* 温度下限 */
    { 9, "5 HIGH LIMIT",       oled_show_info,NULL,m0_root , 0, 0},/* 温度上限 */
    { 9, "6 SENSOR IDENT",     oled_show_info,NULL,m0_root , 0, 0},/* 测温仪型号 */
    { 9, "7 SENSOR REVISION",  oled_show_info,NULL,m0_root , 0, 0},/* 测温仪固件版本 */
    { 9, "8 SERIAL NUMBER",    oled_show_info,NULL,m0_root , 0, 0},/* 序列号 */
    { 9, "9 MAC ADDRESS",      oled_show_info,NULL,m0_root , 0, 0} /* MAC 地址 */
};

/* DEV_CFG 子菜单初始化 (LEVEL 1) */
struct menu_item_t m1_base_cfg[] = {
    { 5, "1 MODE",            do_temp_mode, NULL,   m0_root , 1, 0},     /* 测温模式 */
    { 5, "2 TEMP UNITS",     do_temp_units, NULL,  m0_root , 1, 0},    /* U 温度单位 */
    { 5, "3 RELAY MODE",      do_relay_mode, NULL,  m0_root , 1, 0},    /* 报警模式 */
    { 5, "4 AIM DEVICE",      do_aim_device, NULL,  m0_root , 1, 0},    /* 瞄准设备 */
    { 5, "5 FACTORY DEFAULT", do_fact_default, NULL,m0_root , 1, 0},  /* 出厂默认 */
};

/* TEMP_CFG 子菜单初始化 (LEVEL 1) */
struct menu_item_t m1_para_cfg[] = {
    { 5, "1 SLOPE       ",       do_slope ,  NULL, m0_root , 1, 0}, /* 坡度 */
    { 5, "2 EMISSIVITY",       do_emissivity ,  NULL, m0_root , 1, 0}, /* EL 发射率窄带 */
    { 5, "3 TRANSMISSIVITY N",   do_transmissivity_low ,  NULL, m0_root , 1, 0}, /* TL 透过率窄带 */
    { 5, "4 TRANSMISSIVITY W",   do_transmissivity_high ,  NULL, m0_root , 1, 0}, /* TH 透过率宽带 */
    { 5, "5 TEMP FILT SIZE",     do_temp_filt_size ,  NULL, m0_root , 1, 0} /* A 温度滤波BUF大小 */
};


/* INTERFACE 子菜单初始化 (LEVEL 1) */
struct menu_item_t m1_interface[] = {
  { 6, "1 BAUD RATE",         do_baud_rate,  NULL, m0_root , 1, 0},/* B  波特率 */
    { 6, "2 MULTI ADDR",        do_multi_addr,  NULL, m0_root , 1, 0},/* M 多点地址 */
    { 6, "3 LOCAL IP",      do_local_ip,NULL, m0_root , 4, 0}, /* 设备IP地址显示 */
    { 6, "4 GATEWAY",  do_eth_gw,NULL, m0_root , 4, 0}, /* 设备默认网关显示 */
    { 6, "5 NET MASK",  do_eth_nm,NULL, m0_root , 4, 0}, /* 设备子网掩码显示 */
    { 6, "6 PORT",  do_eth_port,NULL, m0_root , 1, 0}, /* 设备网络端口显示 */
};            

/* ANALOG 子菜单初始化 (LEVEL 1) */
struct menu_item_t m1_analog[] = {
    { 3, "1 ANALOG MODE",       do_analog_mode ,  NULL, m0_root , 1, 0}, /* L 模拟输出下限 */
    { 3, "2 ANALOG OUT L",       do_analog_out_low ,  NULL, m0_root , 1, 0}, /* L 模拟输出下限 */
    { 3, "3 ANALOG OUT H",       do_analog_out_high ,  NULL, m0_root , 1, 0},/* H 模拟输出上限 */
};

/* clang-format on */

ui_t ui = {
    .main_page_id       = 0,
    .sub_page_id        = 0,
    .set_value          = 1,
    .current_para_index = 0,
    .current_para_num   = 0,
    .cfg_states         = false,
    .set_states         = false,
    .is_blinking        = false,
};

/*
 * 功能描述：显示初始化，用于注册显示任务
 * 入参说明：无
 * 返 回 值：无
 */
void ui_init(void) {

    /* 全局变量初始化 */
    sys_state.panel_lock = 'U';

    /* 按键、数码管驱动初始化 */
    key_init();
    tm1639_init();

    /* OLED 初始化 */
    oled_init();

    /* 注册按键扫描任务 */
    ostimer_register(OSTIMER_KEY_SCAN_TASK, key_task, (void *)0, AGENT_PRIO_MD, KEY_SCAN_RATE_HZ);

    /* 注册界面显示任务 */
    ostimer_register(OSTIMER_UI_TASK, ui_task, (void *)0, AGENT_PRIO_MD, UI_TASK_RATE_HZ);
}

/*
 * 功能描述：ui与key交互任务
 * 入参说明：*arg - 任务入参（未使用）
 * 返 回 值：无
 * 备    注：最快可11ms
 */
void ui_task(void *arg) {

    menu_navigate(); /* 菜单漫游及OLED显示 2.3-10ms */

    dis_led_tube_process(); /* 数码管显示，最大0.23ms */
    dis_write_laser_led();  /* 面板指示灯控制0.07ms */
}

/*
 * 功能描述：菜单导航
 * 入参说明：current - 当前菜单项
 *           key - 按下的键
 * 返 回 值：无
 */
void menu_navigate(void) {
    int count = 0;
    if (current_menu->son_ms != NULL) {
        count = current_menu->son_ms->menu_count;
    }

    static char menu_mode         = GUIDE_MODE; /* 当前菜单模式 */
    int8_t      last_main_page_id = ui.main_page_id;
    int8_t      last_sub_page_id  = ui.sub_page_id;
    bool        all_return_flag   = false;
    static bool is_menu           = true;

    if (key_idle_flag == 1) { /* 超时时间后无按键按下 */
        ui.main_page_id = 0;
        ui.sub_page_id  = 0;
        current_menu    = &m0_root[ui.main_page_id]; /* 更新当前菜单为新的主菜单 */

        ui.is_blinking = false;
        is_menu        = false;
        menu_mode      = GUIDE_MODE;
    } else if ((key_idle_flag == 2) && (is_menu == true)) {
        ui.main_page_id = ui.main_page_id % current_menu->menu_count;
        ui.sub_page_id  = 0;                         /* 重置当前子菜单的索引 */
        current_menu    = &m0_root[ui.main_page_id]; /* 更新当前菜单为新的主菜单 */
        if (m0_root[ui.main_page_id].son_ms != NULL) {
            is_menu = false;
        }
        ui.is_blinking = false;
    }
    /* 导航模式 */
    if (menu_mode == GUIDE_MODE) {
        if (current_menu->son_ms != NULL) {
            ui.current_para_num                             = current_menu->son_ms[ui.sub_page_id].para_num;
            current_menu->son_ms[ui.sub_page_id].para_index = 0;
            ui.current_para_index                           = 0;
        } else {
            ui.current_para_num   = 0;
            ui.current_para_index = 0;
        }
        if (key_get_short_flag(KEY_RIGHT)) {
            /* 切换主菜单 */
            ui.main_page_id = (ui.main_page_id + 1) % current_menu->menu_count;
            ui.sub_page_id  = 0;                         /* 重置当前子菜单的索引 */
            current_menu    = &m0_root[ui.main_page_id]; /* 更新当前菜单为新的主菜单 */
            is_menu         = true;
            ui.is_blinking  = false;
        }
        if (key_get_short_flag(KEY_DOWN)) { /* 向上翻子菜单 */
            if (ui.sub_page_id < count - 1) {
                ui.sub_page_id++; /* 向下翻一项 */
            } else {
                ui.sub_page_id = 0; /* 如果到达底部，回到顶部 */
            }
            if (is_menu && (m0_root[ui.main_page_id].son_ms != NULL)) {
                ui.main_page_id = ui.main_page_id % current_menu->menu_count;
                ui.sub_page_id  = 0;                         /* 重置当前子菜单的索引 */
                current_menu    = &m0_root[ui.main_page_id]; /* 更新当前菜单为新的主菜单 */
                is_menu         = false;
            }
        }
        if (key_get_short_flag(KEY_UP)) { /* 向下翻子菜单 */
            if (ui.sub_page_id > 0) {
                ui.sub_page_id--; /* 向上翻一项 */
            } else if (count > 0) {
                ui.sub_page_id = count - 1; /* 如果到达顶部，回到底部 */
            }
        }
        if (key_get_short_flag(KEY_SET)) { /* 进入下级or执行功能 */
            if (is_menu) {
                ui.main_page_id = ui.main_page_id % current_menu->menu_count;
                ui.sub_page_id  = 0;                         /* 重置当前子菜单的索引 */
                current_menu    = &m0_root[ui.main_page_id]; /* 更新当前菜单为新的主菜单 */
                if (m0_root[ui.main_page_id].son_ms != NULL) {
                    is_menu = false;
                }
            } else {
                if (sys_para.dev.panel_lock != 'U') {
                } else if (ui.current_para_num > 0) {
                    menu_mode = EDIT_MODE;
                    if (current_menu->son_ms != NULL) {
                        (current_menu->son_ms[ui.sub_page_id]).subs(); /* 执行子菜单的 subs 函数 */
                    }
                    if (ui.main_page_id != 0) {
                        ui.is_blinking = true;
                    }
                }
            }
            key_init();
        }
    }
    /* 编辑模式 */
    else if (menu_mode == EDIT_MODE) {
        if (key_get_short_flag(KEY_RIGHT)) { /* 切换主菜单 */

            /*界面切换*/
            ui.main_page_id = (ui.main_page_id + 1) % current_menu->menu_count;
            ui.sub_page_id  = 0;                         /* 重置当前子菜单的索引 */
            current_menu    = &m0_root[ui.main_page_id]; /* 更新当前菜单为新的主菜单 */
            menu_mode       = GUIDE_MODE;
            key_init();
            is_menu        = true;
            ui.is_blinking = false;
        }

        if (key_get_short_flag(KEY_UP)) {
            ui.set_value  = 1;
            ui.set_states = 1;
        }
        if (key_get_long_flag(KEY_UP)) {
            ui.set_value  = KEY_TABLES[KEY_UP].step_value;
            ui.set_states = 1;
        }

        if (key_get_short_flag(KEY_DOWN)) {
            ui.set_value  = -1;
            ui.set_states = 1;
        }
        if (key_get_long_flag(KEY_DOWN)) {
            ui.set_value  = -KEY_TABLES[KEY_DOWN].step_value;
            ui.set_states = 1;
        }
        if (key_get_short_flag(KEY_SET)) {
            if (current_menu->son_ms != NULL) {
                ui.current_para_num   = current_menu->son_ms[ui.sub_page_id].para_num;
                ui.current_para_index = current_menu->son_ms[ui.sub_page_id].para_index;
            } else {
                ui.current_para_num   = 0;
                ui.current_para_index = 0;
            }
            if (ui.current_para_num > 0) {
                /*参数切换*/
                current_menu->son_ms[ui.sub_page_id].para_index = (current_menu->son_ms[ui.sub_page_id].para_index + 1);
                ui.current_para_index                           = current_menu->son_ms[ui.sub_page_id].para_index;
            }
            /* 参数保存 */
            ui.cfg_states = 1;
            (current_menu->son_ms + ui.sub_page_id)->subs(); /* 执行子菜单的 subs 函数 */
            if (ui.current_para_index == ui.current_para_num) {
                menu_mode = GUIDE_MODE; /* 退出编辑模式 */
                if (ui.current_para_num > 0) {
                    ui.is_blinking = false;
                }
            }
            key_init();
        }
    }

    if ((last_main_page_id != ui.main_page_id) || (last_sub_page_id != ui.sub_page_id)) {
        ui.set_states = 0;
    }

    /* OLED刷新显示 mean:2.3ms min:1.2ms max:9.6ms */
    if (is_menu && ((last_main_page_id != ui.main_page_id) || (last_sub_page_id != ui.sub_page_id) || (ui.set_states == 1) ||
                    (ui.main_page_id == 0) || all_return_flag)) {
        memset(OLED_GRAM, 0, sizeof(OLED_GRAM)); /* 先把GRAM清空 */
        current_menu->subs();
        oled_refresh_diff();
    }
    if (!is_menu && (current_menu->son_ms != NULL)) {
        memset(OLED_GRAM, 0, sizeof(OLED_GRAM));         /* 先把GRAM清空 */
        (current_menu->son_ms + ui.sub_page_id)->subs(); /* 执行子菜单的 subs 函数 */
        oled_refresh_diff();
    }
}
