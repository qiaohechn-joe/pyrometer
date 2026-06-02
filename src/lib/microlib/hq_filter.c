/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：hq_filter.c
 * 文件描述：滤波算法
 * 作    者：和其光电嵌软团队
 * 备    注：
 * 更新记录：
 *           V1.0 2025/02/10 张晓博   初始版本
 *           V1.1 2025/03/26 陈  军   修改去极值滑移均值算法
 */

#include "hq_filter.h"

exp_mov_avg_t ema;

static void      swap(long long *a, long long *b);
static int       partition(long long arr[], int low, int high);
static long long quick_select(long long arr[], int low, int high, int k);
static long long calculate_filtered_average(long long data[], int size, float filter_rate);
// static long long buff_copy[SENS_CH_SPEC_CNT][FILTER_LEN_MAX];
/*
 * 功能描述：初始化EMA结构体
 * 入参说明：avg --- 指向 EMA 结构体的指针
 *         decay --- 衰减因子（0~1）
 * 返 回 值：无
 */
void init_ema(exp_mov_avg_t *avg, double decay, int biased) {
    avg->shadow         = 0;
    avg->decay          = decay;
    avg->is_initialized = 0; /* 0 表示未初始化 */
    avg->is_biased      = biased;
}

/*
 * 功能描述：更新EMA的值
 * 入参说明：avg --- 指向EMA结构体的指针
 *         data --- 新输入的数据
 * 返 回 值：更新后的EMA值
 */
double update_ema(exp_mov_avg_t *avg, float data) {
    static double decay_power_cache = 1.0; // 优化：保存幂结果，避免重复计算

    decay_power_cache *= avg->decay;

    if (!avg->is_initialized) {
        avg->shadow         = data; /* 首次输入数据直接赋值 */
        avg->is_initialized = 1;

        return avg->shadow;
    } else {
        avg->shadow = avg->decay * data + (1 - avg->decay) * avg->shadow;
    }

    return avg->is_biased ? (avg->shadow / (1 - decay_power_cache)) : avg->shadow;
}
/*
 * 功能描述：计算数组中数据的滑动平均值
 * 入参说明：slid_remove_volt --- 保存滑动平均所需的状态信息
 *           size --- 数组的大小，指定滑动窗口的长度
 *           new_data --- 新加入的数据
 * 返 回 值：滑动平均值
 * 备    注：这个函数用于在一个固定大小的窗口中计算平均值。每次调用时，它会添加一个新的数据点，
 *           并可能从窗口中移除最旧的数据点（如果窗口已满）。
 */

long long sliding_remove_average(slid_remove_t *slid_remove_volt, int size, long long new_data, long long *buff_copy) {
    // 限制窗口大小不超过最大值
    if (size > FILTER_LEN_MAX) {
        size = FILTER_LEN_MAX;
    }

    // 处理size中途被修改的情况 - 移除多余的数据
    if (slid_remove_volt->count > size) {
        int remove_count        = slid_remove_volt->count - size;
        slid_remove_volt->front = (slid_remove_volt->front + remove_count) % FILTER_LEN_MAX;
        slid_remove_volt->count = size;
    }

    // 添加新数据到队列
    int write_pos                     = (slid_remove_volt->front + slid_remove_volt->count) % FILTER_LEN_MAX;
    slid_remove_volt->buff[write_pos] = new_data;

    // 更新队列状态
    if (slid_remove_volt->count >= size) {
        // 队列已满，覆盖最旧的数据
        slid_remove_volt->front = (slid_remove_volt->front + 1) % FILTER_LEN_MAX;
    } else {
        // 队列未满，增加计数
        slid_remove_volt->count++;
    }

    // 如果窗口未满，计算已有数据的平均值
    if (slid_remove_volt->count < size) {
        long long sum = 0;
        for (int i = 0; i < slid_remove_volt->count; i++) {
            int index = (slid_remove_volt->front + i) % FILTER_LEN_MAX;
            sum += slid_remove_volt->buff[index];
        }
        return sum / slid_remove_volt->count;
    }

    // 复制当前窗口数据进行滤波计算
    for (int i = 0; i < size; i++) {
        int index    = (slid_remove_volt->front + i) % FILTER_LEN_MAX;
        buff_copy[i] = slid_remove_volt->buff[index];
    }

    // 计算滤波后的平均值
    long long filtered_volt = calculate_filtered_average(buff_copy, size, 0.2);

    // 处理计算失败的情况
    if (filtered_volt == -1) {
        // 计算失败时返回简单平均值
        long long sum = 0;
        for (int i = 0; i < size; i++) {
            int index = (slid_remove_volt->front + i) % FILTER_LEN_MAX;
            sum += slid_remove_volt->buff[index];
        }
        return sum / size;
    }

    return filtered_volt;
}

/*
 * 功能描述：交换两个整数的值
 * 入参说明：a --- 指向第一个整数的指针
 *           b --- 指向第二个整数的指针
 * 返 回 值：无
 */
static void swap(long long *a, long long *b) {
    long long temp = *a;
    *a             = *b;
    *b             = temp;
}

/*
 * 功能描述：对数组进行分区操作
 * 入参说明：arr --- 整数数组
 *           low --- 分区的起始索引
 *           high --- 分区的结束索引
 * 返 回 值：分区索引
 */
static int partition(long long arr[], int low, int high) {
    long long pivot = arr[high];
    int       i     = low - 1;
    for (int j = low; j <= high - 1; j++) {
        if (arr[j] <= pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

/*
 * 功能描述：快速选择算法，找到数组中的第k小元素（使用迭代而非递归实现，避免栈溢出风险）
 * 入参说明：arr --- 整数数组
 *           low --- 数组的起始索引
 *           high --- 数组的结束索引
 *           k --- 目标顺序统计量
 * 返 回 值：第k小元素的值
 */
static long long quick_select(long long arr[], int low, int high, int k) {
    // 验证k的有效性
    if (k <= 0 || k > high - low + 1) {
        return -1;
    }

    // 使用迭代代替递归
    while (low <= high) {
        // 随机选择枢轴元素，提高平均性能
        int random_index = low + rand() % (high - low + 1);
        swap(&arr[random_index], &arr[high]);

        // 分区操作
        int pivot_index = partition(arr, low, high);
        int left_size   = pivot_index - low + 1;

        // 检查是否找到第k小的元素
        if (left_size == k) {
            return arr[pivot_index];
        } else if (k < left_size) {
            // 在左半部分继续查找
            high = pivot_index - 1;
        } else {
            // 在右半部分继续查找
            k -= left_size;
            low = pivot_index + 1;
        }
    }
    return -1; /* 正常不应该到这里 */
}

/*
 * 功能描述：计算数组中去除最大REMOVE个和最小REMOVE个数后剩余数的整数平均值
 * 入参说明：data --- 整数数组
 *           size --- 数组的大小
 *           filter_rate ---
 * 返 回 值：0 -- 成功（返回平均值），其它 -- 失败。
 */
static long long calculate_filtered_average(long long data[], int size, float filter_rate) {
    int       num_to_remove = (int)(filter_rate * 0.5f * size); /* 要去除的数据数量 */
    long long start         = quick_select(data, 0, size - 1, num_to_remove + 1);
    long long end           = quick_select(data, 0, size - 1, size - num_to_remove);

    long long sum   = 0;
    int       count = 0;
    for (int i = 0; i < size; i++) {
        if (data[i] >= start && data[i] <= end) {
            sum += data[i];
            count++;
        }
    }
    if (count > 0) return sum / count;

    return -1;
}

// typedef struct {
//     long long buff[FILTER_LEN_MAX]; /* 数据缓存 */
//     int       front;                /* 队列头部的索引 */
//     long long sum;                  /* 数组中元素的总和 */
//     uint32_t  count;                /* 目前队列中的元素数量 */
// } slid_remove_t;
/*
 * 功能描述：峰值保持计算
 * 入参说明：slid_holding_data --- 数据存储结构体
 * 返 回 值：当前滤波后的值
 * 备    注：这个函数用于在一定时间内保持测量到的峰值。
 */
long long peak_holding_fliter(slid_holding_t *slid_holding_data, int size, long long new_data) {
    if (size == HOLDING_TIME_UPPER && slid_holding_data->count != 0) {
        /*持续保持*/
        return slid_holding_data->data;
    }
    if (size == 0) {
        slid_holding_data->data  = new_data;
        slid_holding_data->count = 0;
    }
    if (slid_holding_data->count == 0) {
        slid_holding_data->data = new_data;
        slid_holding_data->count++;
        // dbg_info("---peak:record new data dbg info\r\n");
        return new_data;
    }
    if (slid_holding_data->data < new_data) {
        slid_holding_data->data  = new_data;
        slid_holding_data->count = 1;
        // dbg_info("---peak:upgrade new data dbg info\r\n");
    }

    return slid_holding_data->data;
}

/*
 * 功能描述：谷值保持计算
 * 入参说明：slid_holding_data --- 数据存储结构体
 * 返 回 值：当前滤波后的值
 * 备    注：这个函数用于在一定时间内保持测量到的谷值。
 */
long long valley_holding_fliter(slid_holding_t *slid_holding_data, int size, long long new_data) {
    if (size == HOLDING_TIME_UPPER && slid_holding_data->count != 0) {
        /*持续保持*/
        return slid_holding_data->data;
    }
    if (size == 0) {
        slid_holding_data->data  = new_data;
        slid_holding_data->count = 0;
    }
    if (slid_holding_data->count == 0) {
        slid_holding_data->data = new_data;
        slid_holding_data->count++;
        // dbg_info("---valley:record new data dbg info\r\n");
        return new_data;
    }

    if (slid_holding_data->data > new_data) {
        slid_holding_data->data  = new_data;
        slid_holding_data->count = 1;
        // dbg_info("---valley:upgrade new data dbg info\r\n");
    }

    return slid_holding_data->data;
}
