/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：para.c
 * 文件描述：系统参数管理PMM(parameter management module)代码
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

#include "storage.h"
#include "para.h"

static sys_para_t temp_para; // 上电加载的临时存储变量
/* ========================================================================== */
/*                             内部函数声明                                   */
/* ========================================================================== */
static void repair_invalid_storage(const uint32_t *para_state, const uint32_t corrent_state);
static int  try_restore_from_storage(storage_device_t *loc, uint32_t *p_needed_mask, uint32_t *para_state);
static int  para_save_to_storage(const sys_para_t *src, storage_type_t type, uint32_t para_state);
static void para_recalculate_all_crc(sys_para_t *para);

/* ========================================================================== */
/*                             全局配置定义                                   */
/* ========================================================================== */
/* 用户参数区（放在片内Flash）*/
storage_device_t user_loc = {
    .type    = STORAGE_TYPE_USER,
    .media   = STORAGE_MEDIA_INT_FLASH,
    .address = USER_PARA_ADDR,
    .size    = sizeof(sys_para_t),
    .read    = flash_read,
    .write   = flash_write,
};

#if SUPPORT_BACKUP
/* 备份参数区（放在外部EEPROM）*/
storage_device_t backup_loc = {
    .type    = STORAGE_TYPE_BACKUP,
    .media   = STORAGE_MEDIA_EXT_EEPROM,
    .address = BACKUP_PARA_ADDR, // EEPROM地址
    .size    = sizeof(sys_para_t),
    .read    = eeprom_read_lock,
    .write   = eeprom_write_lock,
};
#endif

/* 工厂参数区（片内Flash另一区域）*/
storage_device_t factory_loc = {
    .type    = STORAGE_TYPE_FACTORY,
    .media   = STORAGE_MEDIA_INT_FLASH,
    .address = FACTORY_PARA_ADDR, // 工厂区地址
    .size    = sizeof(sys_para_t),
    .read    = flash_read,
    .write   = flash_write,
};

uint16_t crc_user[PARA_TYPE_CNT];
uint16_t crc_backup[PARA_TYPE_CNT];

/* ========================================================================== */
/*             PMM (Parameter Management Module) API实现                      */
/* ========================================================================== */
/*
 * 功能描述：验证参数结构体：magic + 各组CRC
 * 入参说明：para - 指向 sys_para_t 结构的指针
 * 返 回 值：32位状态码：bit31: 1 = magic有效, bit0~30: 对应组CRC是否通过（1=通过）
 * 备    注：如果magic无效，直接返回0，表示整个结构无效。
 */
uint32_t pmm_validate_para(sys_para_t *para) {
    uint32_t status = 0;

    if (para == NULL) {
        return 0;
    }

    /* === 1. 检查 Magic ===  */
    if (para->magic == PARAM_MAGIC) {
        status |= (1U << 31); // bit31 = 1，结构有效
    } else {
        return 0;
    }

    /* === 2. 循环校验每一组 ===   */
    for (int i = 0; i < PARA_TYPE_CNT; i++) {
        const para_item_t *info = &PARA_INFO_TBL[i];

        uint16_t calc_crc = (uint16_t)~crc16_RTU((uint8_t *)para + info->offset, info->size);
        if (calc_crc == para->crc[i]) {
            status |= (1U << i);
        }
    }
    return status;
}

/*
 * 功能描述：初始化系统参数单元
 * 入参说明：无
 * 返 回 值：0 --- 成功，其它 --- 失败
 * 备    注：采用“按需、分级、按组恢复”的健壮策略。
 *           恢复顺序: User -> Backup -> Factory -> Default.
 *           只恢复上一级未通过校验的参数组。
 */

int para_init(void) {

    uint32_t para_state[STORAGE_TYPE_CNT] = {0};

    sys_para = SYS_DEF_PARA;

    /*定义一个掩码，表示哪些参数组还需要被有效数据填充。初始时，所有组都需要。*/
    uint32_t needed_mask = 0; // bit 0-30, 忽略 magic 位
    for (int index = 0; index < PARA_TYPE_CNT; index++) {
        needed_mask |= (1U << index);
    }
    uint32_t corrent_mask = needed_mask | (1U << 31); // corrent_mask包含magic位

    /*按优先级从高到低（User -> Backup -> Factory）逐级加载和校验*/
    if (needed_mask > 0 || 1) {
        try_restore_from_storage(&user_loc, &needed_mask, &para_state[STORAGE_TYPE_USER]);
        sys_state.para_err_status[STORAGE_TYPE_USER] = para_state[STORAGE_TYPE_USER];
        sys_state.para_err_status[STORAGE_TYPE_USER] |= (1U << 31);
        /*记录用户校验*/
        memcpy(crc_user, (void *)temp_para.crc, sizeof(crc_user));
    }
#if SUPPORT_BACKUP
    if (needed_mask > 0 || 1) {
        try_restore_from_storage(&backup_loc, &needed_mask, &para_state[STORAGE_TYPE_BACKUP]);
        sys_state.para_err_status[STORAGE_TYPE_BACKUP] = para_state[STORAGE_TYPE_BACKUP];
        sys_state.para_err_status[STORAGE_TYPE_BACKUP] |= (1U << 31);
        /*记录备份校验*/
        memcpy(crc_backup, (void *)temp_para.crc, sizeof(crc_user));
    }
#endif
    if (needed_mask > 0 || 1) {
        try_restore_from_storage(&factory_loc, &needed_mask, &para_state[STORAGE_TYPE_FACTORY]);
        sys_state.para_err_status[STORAGE_TYPE_FACTORY] = para_state[STORAGE_TYPE_FACTORY];
        sys_state.para_err_status[STORAGE_TYPE_FACTORY] |= (1U << 31);
    }
    if (needed_mask > 0) {
        dbg_info("Para: Groups [0x%08X] are restored from default.\r\n", needed_mask);
    }
    /*用户正确的情况下直接用用户写备份，不正确的情况下直接走刷新无效存储区*/
    if (sys_state.para_err_status[STORAGE_TYPE_USER] == corrent_mask) {
        for (int i = 0; i < PARA_TYPE_CNT; i++) {
            if (crc_user[i] != crc_backup[i]) {
                dbg_info("Para: Groups [%d] is different from user.\r\n", i);
                pmm_save_to_backup();
                break;
            }
        }
    } else {
        repair_invalid_storage(sys_state.para_err_status, corrent_mask);
    }

    /*关键参数再校验*/
    if (sys_para.dev.addr > DEV_MAX_ADDR || sys_para.dev.addr < DEV_MIN_ADDR) {
        dbg_error("device address(%d) error.\r\n", sys_para.dev.addr);
        sys_para.dev.addr = SYS_DEF_PARA.dev.addr;
        pmm_save_group(PARA_DEV);
    }

    /* 打印最终的参数信息 */
    dump_sys_para();

    return 0;
}

/*
 * 功能描述：保存单个修改后的参数组
 * 入参说明：type - 要保存的参数组类型 (例如 PARA_NET)
 * 返 回 值：无
 * 备    注：计算指定组的CRC，然后将整个 sys_para 保存到用户区和备份区。
 */
void pmm_save_group(int type) {
    ENTER_CRITICAL();
    /* 计算指定组的CRC，并更新到全局 sys_para */
    uint8_t *data_ptr  = (uint8_t *)&sys_para + PARA_INFO_TBL[type].offset;
    uint16_t crc_calc  = (uint16_t)~crc16_RTU(data_ptr, PARA_INFO_TBL[type].size);
    sys_para.crc[type] = crc_calc;

    /*更新magic*/
    sys_para.magic = PARAM_MAGIC;

    /*将更新后的 sys_para 保存到用户区和备份区*/
    if (para_save_to_storage(&sys_para, STORAGE_TYPE_USER, 1 << type) == 0) {
        dbg_info("Para_USER: Group '%s' saved.\r\n", PARA_INFO_TBL[type].desc);
    } else {
        dbg_info("Para_USER: Group '%s' saved error.\r\n", PARA_INFO_TBL[type].desc);
    }
#if SUPPORT_BACKUP
    if (para_save_to_storage(&sys_para, STORAGE_TYPE_BACKUP, 1 << type) == 0) {
        dbg_info("Para_BACKUP: Group '%s' saved.\r\n", PARA_INFO_TBL[type].desc);
    } else {
        dbg_info("Para_BACKUP: Group '%s' saved error.\r\n", PARA_INFO_TBL[type].desc);
    }
#endif
    EXIT_CRITICAL();
}

/*
 * 功能描述：将当前运行参数整体恢复为系统默认值
 * 入参说明：无
 * 返 回 值：无
 * 备    注：此操作会覆盖用户区和备份区。
 */
void pmm_restore_to_default(void) {
    ENTER_CRITICAL();
    sys_para = SYS_DEF_PARA;

    /* 重新计算所有参数组的CRC */
    para_recalculate_all_crc(&sys_para);

    /* 将准备好的 sys_para 一次性写入用户区和备份区 */
    para_save_to_storage(&sys_para, STORAGE_TYPE_USER, 0x7fffffff);
#if SUPPORT_BACKUP
    para_save_to_storage(&sys_para, STORAGE_TYPE_BACKUP, 0x7fffffff);
#endif
    EXIT_CRITICAL();

    dbg_info("Para: System restored to default parameters.\r\n");
}

/*
 * 功能描述：将当前运行参数整体恢复为出厂设置
 * 入参说明：无
 * 返 回 值：无
 * 备    注：此操作会覆盖用户区和备份区。
 */
void pmm_restore_to_factory(void) {
    ENTER_CRITICAL();
    /* 从内存中的工厂参数副本加载 */
    uint32_t needed_mask = 0; // bit 0-30, 忽略 magic 位
    for (int index = 0; index < PARA_TYPE_CNT; index++) {
        needed_mask |= (1U << index);
    }
    uint32_t corrent_mask       = needed_mask | (1U << 31);
    uint32_t factory_para_state = 0x00000000;
    try_restore_from_storage(&factory_loc, &needed_mask, &factory_para_state);
    if (factory_para_state != corrent_mask) {
        dbg_error("Para: System restored from factory failed!!! factory para check result: %x\r\n", factory_para_state);
        return;
    }

    /* 将其写入用户区和备份区 */
    para_save_to_storage(&sys_para, STORAGE_TYPE_USER, 0x7fffffff);
#if SUPPORT_BACKUP
    para_save_to_storage(&sys_para, STORAGE_TYPE_BACKUP, 0x7fffffff);
#endif
    EXIT_CRITICAL();

    dbg_info("Para: System restored to factory parameters.\r\n");
}

/*
 * 功能描述：将当前有效的运行参数设置为新的出厂参数
 * 入参说明：无
 * 返 回 值：无
 * 备    注：此操作会覆盖工厂区，请谨慎使用。
 */
void pmm_save_to_factory(void) {
    ENTER_CRITICAL();
    para_save_to_storage(&sys_para, STORAGE_TYPE_FACTORY, 0x7fffffff);
    EXIT_CRITICAL();

    dbg_info("Para: Current parameters have been set as new factory parameters.\r\n");
}

/*
 * 功能描述：将当前有效的运行参数设置为一键覆写到eeprom作为备份参数
 * 入参说明：无
 * 返 回 值：无
 * 备    注：此操作会覆盖备份区。
 */
void pmm_save_to_backup(void) {
    ENTER_CRITICAL();
    para_save_to_storage(&sys_para, STORAGE_TYPE_BACKUP, 0x7fffffff);
    EXIT_CRITICAL();

    dbg_info("Para: Current parameters have been set as new backup parameters.\r\n");
}
/* ========================================================================== */
/*                             内部静态函数实现                               */
/* ========================================================================== */
/*
 * 功能描述：核心保存函数，将参数结构体写入指定存储区
 * 入参说明：src - 指向源数据 sys_para_t 的指针
 *           type - 目标存储区类型
 *           para_state - 一个位掩码，指示哪些参数组需要被保存 (1 << group_index)
 * 返 回 值：0=成功, -1=失败
 * 备    注：内部函数，封装了物理写入操作。
 */
static int para_save_to_storage(const sys_para_t *src, storage_type_t type, uint32_t para_state) {
    storage_device_t *loc = NULL;
    switch (type) {
        case STORAGE_TYPE_USER: loc = &user_loc; break;
        case STORAGE_TYPE_BACKUP: loc = &backup_loc; break;
        case STORAGE_TYPE_FACTORY: loc = &factory_loc; break;
        default: return -1;
    }

    /*生成正确掩码*/
    uint32_t corrent_mask = 0; // bit 0-30, 忽略 magic 位
    int      index        = 0;
    for (index = 0; index < PARA_TYPE_CNT; index++) {
        corrent_mask |= (1U << index);
    }
    // corrent_mask |= (1U << 31);

    if (!src || !loc->write) {
        return -1;
    }

    /*保存前进行校验*/
    para_recalculate_all_crc(&sys_para);

    if ((para_state & corrent_mask) == corrent_mask) {
        return loc->write(loc->address, (void *)src, sizeof(sys_para_t));
    }

    uint8_t *buffer = (uint8_t *)src;
    int      result = 0;
    if (loc->media == STORAGE_MEDIA_INT_FLASH) {
        /*  flash 参数写入  */
        result = loc->write(loc->address, (void *)src, sizeof(sys_para_t));
        if (result != 0) {
            /* 如果写入失败，则返回错误*/
            return result;
        }
    } else if (loc->media == STORAGE_MEDIA_EXT_EEPROM) {
        /*  eeprom 参数写入  */
        for (int i = 0; i < PARA_TYPE_CNT; i++) {
            if (para_state & (1U << i)) {
                // 保存第 i 个参数组
                uint32_t offset     = PARA_INFO_TBL[i].offset;
                uint32_t size       = PARA_INFO_TBL[i].size;
                uint32_t crc_offset = offset_of(crc, sys_para_t);

                result = loc->write(loc->address + offset, buffer + offset, size);
                if (result != 0) {
                    // 如果写入失败，则返回错误
                    return result;
                }
                /*写校验位置*/
                result = loc->write(loc->address + crc_offset + sizeof(uint16_t) * i, buffer + crc_offset + sizeof(uint16_t) * i, sizeof(uint16_t));
                if (result != 0) {
                    // 如果写入失败，则返回错误
                    return result;
                }
            }
        }
    }

    return result;
}

/*
 * 功能描述：为一个参数结构体重新计算所有组的CRC
 * 入参说明：para - 指向待计算的 sys_para_t 结构的指针
 * 返 回 值：无
 * 备    注：用于恢复默认/出厂设置时确保数据一致性。
 */
static void para_recalculate_all_crc(sys_para_t *para) {
    para->magic = PARAM_MAGIC;
    for (int i = 0; i < PARA_TYPE_CNT; i++) {
        uint8_t *data_ptr = (uint8_t *)para + PARA_INFO_TBL[i].offset;
        para->crc[i]      = (uint16_t)~crc16_RTU(data_ptr, PARA_INFO_TBL[i].size);
    }
}

/*
 * 功能描述：将无效的存储区修复为有效状态
 * 入参说明：para_state - 各存储区参数组的状态
 * 返 回 值：无
 * 备    注：内部函数，用于修复启动时发现的无效存储区。
 */
static void repair_invalid_storage(const uint32_t *para_state, const uint32_t corrent_state) {
    /*写入前缺包数据通过校验*/
    if ((para_state[STORAGE_TYPE_USER] != corrent_state) && (para_state[STORAGE_TYPE_USER] != 0)) {
        dbg_info("Para: Committing final valid config to User storage.\r\n");
        para_save_to_storage(&sys_para, STORAGE_TYPE_USER, ~para_state[STORAGE_TYPE_USER]);
    }

#if SUPPORT_BACKUP
    /* 检查备份区的最终状态 */
    if ((para_state[STORAGE_TYPE_BACKUP] != corrent_state) && (para_state[STORAGE_TYPE_BACKUP] != 0)) {
        dbg_info("Para: Committing final valid config to Backup storage.\r\n");
        para_save_to_storage(&sys_para, STORAGE_TYPE_BACKUP, ~para_state[STORAGE_TYPE_BACKUP]);
    }
#endif
}

/*
 * 功能描述：加载并校验一个指定存储级别的数据，并按需更新全局 sys_para
 * 入参说明：loc - 要加载的存储设备
 *           p_needed_mask - 指向“需要被填充的参数组”掩码的指针
 *           para_state - 校验状态
 * 返 回 值：0=成功, -1=失败
 * 备    注：内部函数，是分级恢复逻辑的核心。
 */
static int try_restore_from_storage(storage_device_t *loc, uint32_t *p_needed_mask, uint32_t *para_state) {

    memset(&temp_para, 0, sizeof(temp_para));

    /* 从存储介质加载到临时变量 */
    if (loc->read(loc->address, &temp_para, sizeof(sys_para)) != 0) {
        *para_state = 0; // 加载失败，该级别完全无效
        return -1;
    }

    /* 对临时变量进行完整性校验 */
    uint32_t current_level_status = pmm_validate_para(&temp_para);
    *para_state                   = current_level_status; // 记录该级别的校验结果

    if ((current_level_status & (1U << 31)) == 0) {
        dbg_info("Para: Magic invalid for storage type %d.\r\n", loc->type);
        return -1;
    }

    for (int i = 0; i < PARA_TYPE_CNT; i++) {
        uint32_t group_mask = (1U << i);

        /* 如果这个参数组(i)是需要的 AND 在当前存储级别中是有效的 */
        if ((*p_needed_mask & group_mask) && (current_level_status & group_mask)) {
            const para_item_t *info = &PARA_INFO_TBL[i];

            /* 从临时变量中拷贝这个有效的参数组到全局 sys_para */
            memcpy((uint8_t *)&sys_para + info->offset, (uint8_t *)&temp_para + info->offset, info->size);

            /* 更新CRC值到全局 sys_para */
            sys_para.crc[i] = temp_para.crc[i];

            /* 标记这个参数组为“已填充”，从 needed_mask 中移除 */
            *p_needed_mask &= ~group_mask;
        }
    }

    /* 更新全局 sys_para 的 magic (如果它是第一个被成功加载的) */
    if (sys_para.magic != PARAM_MAGIC) {
        sys_para.magic = PARAM_MAGIC;
    }

    return 0;
}

/*
 * 功能描述：打印当前系统参数信息
 * 入参说明：无
 * 返 回 值：无
 * 备    注：内部调试函数。
 */
void dump_sys_para(void) {
    dbg_info("\r\n%s\r\n", "System para: ");
    dbg_info("---Device address (0-%d): %d\r\n", DEV_ADDR_MAX, sys_para.dev.addr);

#if SUPPORT_EEPROM
    dbg_info("%s\r\n", "---Storage use eeprom?(Y/N): Y");
#else
    dbg_info("%s\r\n", "---Storage use eeprom?(Y/N): N");
#endif
    if (sys_state.para_err_status[STORAGE_TYPE_USER] & 0x80000000) {
        dbg_info("%s\r\n", "---User para status: ");
        /*该组参数被校验*/
        for (int i = 0; i < PARA_TYPE_CNT; i++) {
            dbg_info("------%s: %s\r\n", PARA_INFO_TBL[i].desc, (sys_state.para_err_status[STORAGE_TYPE_USER] & (1U << i)) ? "OK" : "ERR");
        }
    }

#if SUPPORT_BACKUP
    if (sys_state.para_err_status[STORAGE_TYPE_BACKUP] & 0x80000000) {
        dbg_info("%s\r\n", "---Backup para status: ");
        for (int i = 0; i < PARA_TYPE_CNT; i++) {
            dbg_info("------%s: %s\r\n", PARA_INFO_TBL[i].desc, (sys_state.para_err_status[STORAGE_TYPE_BACKUP] & (1U << i)) ? "OK" : "ERR");
        }
    }
    for (int i = 0; i < PARA_TYPE_CNT; i++) {
        if (crc_user[i] != crc_backup[i]) {
            dbg_info("Para: Current Groups [%d] is different from user.\r\n", i);
            dbg_info("Para: Current backup is upgraded to user.\r\n", i);
            break;
        }
    }

#else
    dbg_info("%s\r\n", "No backup storage support.");
#endif
    if (sys_state.para_err_status[STORAGE_TYPE_FACTORY] & 0x80000000) {
        dbg_info("%s\r\n", "---Factory para status: ");
        for (int i = 0; i < PARA_TYPE_CNT; i++) {
            dbg_info("------%s: %s\r\n", PARA_INFO_TBL[i].desc, (sys_state.para_err_status[STORAGE_TYPE_FACTORY] & (1U << i)) ? "OK" : "ERR");
        }
    }
    dbg_info("%s\r\n", "---running para status: ");
    for (int i = 0; i < PARA_TYPE_CNT; i++) {
        if (sys_state.para_err_status[STORAGE_TYPE_USER] & (1U << i)) {
            dbg_info("------%s: %s\r\n", PARA_INFO_TBL[i].desc, "USER");
        } else if (sys_state.para_err_status[STORAGE_TYPE_BACKUP] & (1U << i)) {
            dbg_info("------%s: %s\r\n", PARA_INFO_TBL[i].desc, "BACKUP");
        } else if (sys_state.para_err_status[STORAGE_TYPE_FACTORY] & (1U << i)) {
            dbg_info("------%s: %s\r\n", PARA_INFO_TBL[i].desc, "FACTORY");
        } else {
            dbg_info("------%s: %s\r\n", PARA_INFO_TBL[i].desc, "DEFAULT");
        }
    }
}
