/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：para.h
 * 文件描述：系统参数管理代码
 * 作    者：和其光电嵌软团队
 * 备    注：1）使用编译器提供的对齐属性（4字节对齐）
 *           2）分为默认参数(default-FLASH)、工厂参数(factory-FLASH)、用户参数(user-FLASH)、备份参数(backup-EEPRM)
 *           3）如果没有PPEROM，则无需备份参数
 * 更新记录：
 *           V1.0 2024/01/26 陈军   初始版本
 *           V1.1 2024/04/10 李兆越 增加参数备份逻辑
 *           V1.2 2024/08/15 段政宏 梳理参数保存机制，简化参数操作逻辑
 *           V1.3 2024/12/04 李兆越 优化参数保存逻辑，适配没有EEPROM的情况
 *           V1.4 2025/01/17 李兆越 参数保存按页擦除；eeprom作为备份区
 *           V1.5 2025/02/17 张晓博 修复支持EEPROM时校验的问题；校验错误即退出
 *           V1.6 2025/03/13 陈军   增加EEPROM自检功能，兼容裸机，去掉对齐操作
 *           V1.7 2025/05/18 李兆越 增加参数全局配置，规范参数保存模式
 *           V1.8 2025/05/22 李兆越 宏选择异常优化
 */

#ifndef _PARA_H_
#define _PARA_H_

#include "platform.h"

/* 参数状态 */
#define PARA_ST_OK     0
#define PARA_ST_ERR    1

#define SUPPORT_BACKUP 1
#define PARAM_MAGIC    0x00080296

/* 存储设备的物理介质类型 */
typedef enum {
    STORAGE_MEDIA_INT_FLASH,  ///< 内部 FLASH
    STORAGE_MEDIA_EXT_EEPROM, ///< 外部 EEPROM
    STORAGE_MEDIA_SD_CARD,    ///< SD卡
    STORAGE_MEDIA_USB,        ///< USB存储
    STORAGE_MEDIA_CNT         ///< 总数量
} storage_media_t;

/* 存储设备的逻辑类型（用途） */
typedef enum {
    STORAGE_TYPE_USER,    ///< 用户参数区
    STORAGE_TYPE_BACKUP,  ///< 备份参数区
    STORAGE_TYPE_FACTORY, ///< 工厂参数区
    STORAGE_TYPE_DEFAULT, ///< 默认参数（内存中）
    STORAGE_TYPE_CNT      ///< 总数量
} storage_type_t;

// 前向声明
struct storage_device_t;

// 定义存储操作的函数指针类型
typedef int (*storage_read_f)(uint32_t addr, void *buff, int len);
typedef int (*storage_write_f)(uint32_t addr, void *buff, int len);

/* 存储设备描述结构体 */
typedef struct storage_device_t {
    storage_type_t  type;    // 逻辑类型：用户/备份/工厂
    storage_media_t media;   // 物理介质
    uint32_t        address; // 存储地址或偏移
    size_t          size;    // 存储大小
    storage_read_f  read;
    storage_write_f write;
} storage_device_t;

/* ========================================================================== */
/*                      PMM (Parameter Management Module) API                 */
/* ========================================================================== */

/**
 * @brief 初始化参数管理模块 (PMM)
 * @note  系统启动时必须调用的第一个初始化函数。
 *        它会执行复杂的分级加载和恢复逻辑。
 * @return 0=成功, 其他=失败
 */
int para_init(void);

/**
 * @brief 保存单个修改后的参数组
 * @note  当上位机或内部逻辑修改了某个参数组后，调用此函数进行持久化。
 * @param type 要保存的参数组类型 (例如 PARA_NET, PARA_VIDEO)
 */
void pmm_save_group(int type);

/**
 * @brief 将当前运行参数整体恢复为系统默认值
 * @note  此操作会覆盖用户区和备份区。
 */
void pmm_restore_to_default(void);

/**
 * @brief 将当前运行参数整体恢复为出厂设置
 * @note  此操作会覆盖用户区和备份区。
 */
void pmm_restore_to_factory(void);

/**
 * @brief 将当前有效的运行参数设置为新的出厂参数
 * @note  此操作会覆盖工厂区，请谨慎使用。
 */
void pmm_save_to_factory(void);

/**
 * @brief 将当前有效的运行参数设置为一键覆写到eeprom作为备份参数
 * @note  此操作会覆盖备份区，用于结构体被修改导致数据错位后直接完全覆写。
 */
void pmm_save_to_backup(void);

/**
 * @brief 打印当前参数日志
 * @note  调试函数
 */
void dump_sys_para(void);
#endif /* _PARA_H_ */
