/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：gd32_rtc.c
 * 文件描述：RTC-BSP层驱动（GD32F4xx）
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2024/01/26 陈军  初始版本
 *           V1.1 2024/05/21 陈军  更新RTC初始化逻辑
 *           V1.2 2024/12/04 陈军  统一结构体定义方式
 */

#include "gd32_ll.h"
#include "gd32_rtc.h"
#include "hq_generic.h"

#define START_YEAR 1970

static rtc_parameter_struct rtc_initpara; /* RTC参数结构体 */

/* 由gregorian_day()函数使用 */
const int month_offset[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
/* 每个月的天数 */
uint8_t month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
/* 仅适用于 1970 - 2106 年 */
#define leap_year(y)    ((((y) & 0x03) == 0) && ((y) != 2100))
#define days_in_year(a) (leap_year(a) ? 366 : 365)

/* 内部函数 */
static void gregorian_day(rtc_time_t *time);

/*
 * 功能描述：此函数初始化 RTC（实时时钟）
 * 入参说明：无
 * 返 回 值：0 --- 成功  其他 --- 失败
 * 备    注：1）默认尝试使用LSE，当LSE启动失败后，切换为LSI；
 *           2）通过BKP寄存器0的值，可以判断RTC使用的是LSE/LSI：
 *              当BKP0==0X7050时，使用的是LSE；
 *              当BKP0==0X7051时，使用的是LSI；
 *           3）注意：切换LSI/LSE将导致时间/日期丢失，切换后需重新设置。
 */
int gd32_rtc_init(void) {
    uint32_t div_a; /* 异步分频值 */
    uint32_t div_s; /* 同步分频值 */
    uint16_t bkp_flag    = 0;
    uint8_t  rtcsrc_flag = 0;

    pmu_backup_write_enable(); /* 备份域写使能 */
    bkp_flag = RTC_BKP0;       /* 读取BKP0的值 */
    rcu_osci_on(RCU_LXTAL);    /* 打开外部低速晶振 */

    if ((rcu_osci_stab_wait(RCU_LXTAL) == ERROR)) /* 开启CK_LXTAL失败? */
    {
        rcu_osci_on(RCU_IRC32K); /* 打开CK_IRC32K时钟 */

        if ((rcu_osci_stab_wait(RCU_IRC32K) == ERROR)) /* 开启CK_IRC32K失败? */
        {
            return -1; /* 开启RTC时钟源失败 */
        }

        rcu_rtc_clock_config(RCU_RTCSRC_IRC32K); /* 选择 CK_IRC32K时钟作为RTC的时钟源 */
        div_s    = 0x13F;                        /* RTC同步分频值(0~7FFF) */
        div_a    = 0x63;                         /* RTC异步分频值(0~0X7F) */
        RTC_BKP0 = 0X7051;                       /* 标记RTC使用LSI */
    } else {
        rcu_rtc_clock_config(RCU_RTCSRC_LXTAL); /* 选择RCU_LXTAL时钟作为RTC的时钟源 */
        div_s    = 0xFF;
        div_a    = 0x7F;
        RTC_BKP0 = 0X7050; /* 标记RTC使用LSE */
    }

    rcu_periph_clock_enable(RCU_RTC); /* 使能RTC时钟 */
    rtc_register_sync_wait();         /* 等待同步 */
    rtc_wakeup_disable();             /* 禁止唤醒 */
    rtc_timestamp_disable();          /* 禁止时间戳 */
    rtc_alarm_disable(RTC_ALARM0);    /* 禁止闹钟 */
    rtc_alarm_disable(RTC_ALARM1);
    rtcsrc_flag = GET_BITS(RCU_BDCTL, 8, 9); /* 获取RTC时钟源选择 */

    /* BKP0的内容既不是0X7050,也不是0X7051,说明是第一次配置,或者RTC时钟源没有配置，需要设置时间日期. */
    if (((bkp_flag != 0x7050) && (bkp_flag != 0x7051)) || (0x00 == rtcsrc_flag)) {
        rtc_initpara.factor_asyn    = div_a;      /* 设置RTC异步分频系数 */
        rtc_initpara.factor_syn     = div_s;      /* 设置RTC同步分频系数 */
        rtc_initpara.display_format = RTC_24HOUR; /* RTC时间格式为24小时格式 */
        rtc_initpara.year           = 0x24;
        rtc_initpara.month          = RTC_JAN;
        rtc_initpara.date           = 0x01;
        rtc_initpara.day_of_week    = (RTC_MONDAY & 0X07);
        rtc_initpara.hour           = 0x10;
        rtc_initpara.minute         = 0x59;
        rtc_initpara.second         = 0x00;
        rtc_initpara.am_pm          = ((uint32_t)RTC_AM & 0X01);

        if (!rtc_init(&rtc_initpara)) return -1; /* 根据参数初始化RTC寄存器 */
    }

    return 0;
}

/*
 * 功能描述：此函数通过自1970-1-1 00:00:00起的秒数来设置 RTC 时间
 * 入参说明：val --- 自1970-1-1 00:00:00起的秒数
 * 返 回 值：0=成功
 */
int gd32_rtc_set_second(uint32_t val) {
    rtc_time_t time;

    gd32_rtc_sec_to_time(val, &time);
    return gd32_rtc_set_time(&time);
}

/*
 * 功能描述：此函数获取从1970-1-1 00:00:00起的秒数
 * 入参说明：无
 * 返 回 值：秒数值，0=失败
 */
uint32_t gd32_rtc_get_second(void) {
    rtc_time_t time;
    uint32_t   ret;

    if (gd32_rtc_get_time(&time)) {
        ret = 0;
    } else {
        ret = gd32_rtc_time_to_sec(&time);
    }

    return ret;
}

/*
 * 功能描述：此函数将当前日期和时间设置到RTC
 * 入参说明：time --- 指向将被设置的 rtc_time_t 结构体的指针
 * 返 回 值：0=成功
 */
int gd32_rtc_set_time(const rtc_time_t *time) {
    ErrStatus err;

    /* 使能进入BKP */
    pmu_backup_write_enable();

    /* 未锁定RTC寄存器 */
    RTC_WPK = RTC_UNLOCK_KEY1;
    RTC_WPK = RTC_UNLOCK_KEY2;

    /* 进入初始化模式 */
    err = rtc_init_mode_enter();
    if (err == ERROR) {
        return -1;
    }

    RTC_DATE = (to_BCD(time->year - 2000) << 16) | (to_BCD(time->month) << 8) | to_BCD(time->mday);
    RTC_TIME = (to_BCD(time->hour) << 16) | (to_BCD(time->minute) << 8) | to_BCD(time->second);

    /* 退入初始化模式 */
    rtc_init_mode_exit();

    /*  等待RSYNF标志置位 */
    err = rtc_register_sync_wait();

    /* 锁RTC寄存器 */
    RTC_WPK = RTC_LOCK_KEY;

    /* 失能进入BKP */
    pmu_backup_write_disable();

    return (err == ERROR) ? -1 : 0;
}

/*
 * 功能描述：此函数从RTC获取当前日期和时间
 * 入参说明：time --- 指向将被设置的 rtc_time_t 结构体的指针
 * 返 回 值：0=成功
 */
int gd32_rtc_get_time(rtc_time_t *time) {
    uint32_t  tr;
    uint32_t  dr;
    ErrStatus err;

    /* 等待RSF标志置位 */
    err = rtc_register_sync_wait();
    if (err == ERROR) {
        return -1;
    }

    /* 获取日期和时间 */
    tr = RTC_TIME;
    dr = RTC_DATE;

    time->year   = from_BCD((dr >> 16) & 0x0ff) + 2000;
    time->month  = from_BCD((dr >> 8) & 0x1f);
    time->mday   = from_BCD(dr & 0x3f);
    time->wday   = (dr >> 13) & 0x07;
    time->hour   = from_BCD((tr >> 16) & 0x3f);
    time->minute = from_BCD((tr >> 8) & 0x7f);
    time->second = from_BCD(tr & 0x7f);

    return 0;
}

/*
 * 功能描述：此函数获取RTC时间差值用于RTC功能校验
 * 入参说明：time1 --- 指向time1对应的结构体的指针
 *           time2 --- 指向time2对应的结构体的指针
 * 返 回 值：时间差值（秒）
 */
int gd32_rtc_diff_time(const rtc_time_t *time1, const rtc_time_t *time2) {
    uint32_t sec1;
    uint32_t sec2;

    sec1 = gd32_rtc_time_to_sec(time1);
    sec2 = gd32_rtc_time_to_sec(time2);

    return sec2 - sec1;
}

/*
 * 功能描述：此函数将rtc_time转换为总秒（UNIX时间戳）
 * 入参说明：time --- 指向time对应的结构体的指针
 * 返 回 值：无
 * 备    注：这个函数将在2106-02-07 06:28:16溢出（对于32位CPU）
 */
uint32_t gd32_rtc_time_to_sec(const rtc_time_t *time) {
    uint32_t year;
    uint32_t month;
    uint32_t day;

    /* 月：1..12 -> 11,12,1..10 */
    if (time->month <= 2) {
        /* 将二月放在最后，因为它有闰日 */
        year  = time->year - 1;
        month = time->month + 10;
    } else {
        year  = time->year;
        month = time->month - 2;
    }

    day = year / 4 - year / 100 + year / 400 + 367 * month / 12 + time->mday;
    day += (year * 365 - 719499);

    return ((day * 24 + time->hour) * 60 + time->minute) * 60 + time->second;
}

/*
 * 功能描述：此函数将总秒（UNIX时间戳）转换为rtc_time
 * 入参说明：sec --- 总秒（1970-1-1 0:00起开始计算）
 *           time --- 指向time对应的结构体的指针
 * 返 回 值：无
 * 备    注：这个函数将在2106-02-07 06:28:16溢出（对于32位CPU）
 */
void gd32_rtc_sec_to_time(uint32_t sec, rtc_time_t *time) {
    uint32_t i;
    uint32_t hms;
    uint32_t day;

    day = sec / 86400U; /* 86400 : seconds in a day */
    hms = sec % 86400U;

    /* 时，分，秒 */
    time->hour   = hms / 3600;
    time->minute = (hms % 3600) / 60;
    time->second = (hms % 3600) % 60;

    /* 年 */
    for (i = START_YEAR; day >= days_in_year(i); i++) {
        day -= days_in_year(i);
    }
    time->year = i;

    /* 日 */
    month_days[1] = leap_year(time->year) ? 29 : 28;
    for (i = 0; day >= month_days[i]; i++) {
        day -= month_days[i];
    }

    /* 月 */
    time->month = i + 1;
    time->mday  = day + 1;

    /* 日对应星期 */
    gregorian_day(time);
}

/*
 * 功能描述：此函数确定rtc_time对应日期下是星期几
 * 入参说明：time --- 指向time对应的结构体的指针
 * 返 回 值：无
 * 备    注：只适用于格里高利历 - 即1752年之后（在英国）
 */
static void gregorian_day(rtc_time_t *time) {
    int leaps_day;
    int last_year;
    int day;

    last_year = time->year - 1;

    /* 截至去年年底需要应用的闰年修正次数 */
    leaps_day = last_year / 4 - last_year / 100 + last_year / 400;

    /* 如果今年能被4整除，则为闰年，但是如果能被100整除，除非能被400整除 */
    if (((time->year % 4) == 0) && (((time->year % 100) != 0) || ((time->year % 400) == 0)) && (time->month > 2)) {
        day = 1;
    } else {
        day = 0;
    }

    day += last_year * 365 + leaps_day + month_offset[time->month - 1] + time->mday;

    time->wday = day % 7;
}
