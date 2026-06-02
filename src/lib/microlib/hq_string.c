/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：hq_string.c
 * 文件描述：字符串库
 * 作    者：和其光电嵌软团队
 * 备    注：函数参数未经检查，所以必须是有效的指针
 * 更新记录：
 *           V1.0 2024/01/26 陈军    初始版本
 *           V1.1 2025/03/10 张晓博  加入 string_to_double_len
 *           V1.2 2025/03/18 李兆越  加入 simple_strrchr，查找最后一次出现的字符
 *           V1.3 2025/03/21 李兆越  加入 is_pure_comma_digit
 *           V1.4 2025/04/03 李兆越  加入 is_pure_digit_n
 *           V1.5 2025/04/24 李兆越  add is_upper
 */

#include "hq_string.h"

/*
 * 功能描述：此函数返回一个以'\0'结尾的字符串的长度
 * 入参说明：str --- 要计算长度的字符串
 * 返 回 值：长度
 */
int string_length(const char *str) {
    const char *start = str;

    while (*str) {
        str++;
    }

    return (str - start);
}
/*
 * 功能描述：此函数将一个字符串复制到另一个字符串
 * 入参说明：dest --- 目标字符串
 *           src --- 源字符串
 * 返 回 值：指向dest末尾('\0')的指针
 */
char *string_copy(char *dest, const char *src) {
    while (*src) {
        *dest++ = *src++;
    }

    *dest = 0;

    return dest;
}
/*
 * 功能描述：此函数将一个字符串复制到另一个字符串（指定长度）
 * 入参说明：dest --- 目标字符串
 *           src --- 源字符串
 *           len --- 要复制的长度
 * 返 回 值：指向dest末尾('\0')的指针
 */
char *string_copy_n(char *dest, const char *src, int len) {
    /* 复制 */
    while (len > 0) {
        if (*src == 0) {
            break;
        }
        *dest++ = *src++;
        len--;
    }

    *dest = 0;

    return dest;
}
/*
 * 功能描述：此函数比较两个字符串
 * 入参说明：str1, str2 --- 要比较的两个字符串
 * 返 回 值：0 --- str1 == str2，-1 --- str1 < str2，1 --- str1 > str2
 */
int string_comp(const char *str1, const char *str2) {
    while (1) {
        /* str1终止 */
        if (*str1 == 0) {
            if (*str2 == 0) {
                return 0; /* str2终止, str1 == str2 */
            } else {
                return -1; /* str2未终止, str1 < str2 */
            }
        }

        /* str2终止，str1未终止，str1 > str2 */
        if (*str2 == 0) {
            return 1;
        }

        /* str1和str2无终止，进行比较 */
        if (*str1 < *str2) {
            return -1;
        } else if (*str1 > *str2) {
            return 1;
        }

        str1++;
        str2++;
    }
}
/*
 * 功能描述：此函数比较两个字符串（指定长度）
 * 入参说明：str1, str2 --- 要比较的两个字符串
 *           len --- 比较的长度
 * 返 回 值：0 --- str1 == str2，-1 --- str1 < str2，1 --- str1 > str2
 * 备    注：长度不能大于string_length(str1)和string_length(str2)的最小值
 */
int string_comp_n(const char *str1, const char *str2, int len) {
    while (len > 0) {
        if (*str1 < *str2) {
            return -1;
        } else if (*str1 > *str2) {
            return 1;
        }

        str1++;
        str2++;
        len--;
    }

    return 0;
}
/*
 * 功能描述：此函数反转一个字符串
 * 入参说明：str --- 要反转的字符串
 * 返 回 值：无
 */
void string_reverse(char *str) {
    char *end = str;
    char  temp;

    /* end指向最后一个字符 */
    while (*end) {
        end++;
    }
    end--;

    while (str < end) {
        /* 反转*str和*end  */
        temp = *str;
        *str = *end;
        *end = temp;

        /* 到下一对 */
        str++;
        end--;
    }
}

/*
 * 功能描述：此函数从一个分隔字符串中提取字符串
 * 入参说明：dest --- 目标字符串
 *           src --- 源字符串
 *           sp --- 分隔字符
 */
const char *string_extract(char *dest, const char *src, char sp) {
    const char *start = src;
    const char *end;

    for (end = start; *end; end++) {
        if (*end == sp) {
            break;
        }
    }

    string_copy_n(dest, start, end - start);

    end++;
    return end;
}
/*
 * 功能描述：此函数在一个字符串中搜索子字符串
 * 入参说明：str --- 要搜索的字符串，以'\0'结尾
 *           sub --- 子字符串，以'\0'结尾
 * 返 回 值：指向子字符串第一次出现的位置的指针，如果未找到则返回0
 */
const char *string_search(const char *str, const char *sub) {
    char *pos;

    pos = (char *)str;

    while (*pos) {
        if (string_comp(pos, sub) == 0) {
            break;
        }
        pos++;
    }

    return (*pos == 0) ? 0 : pos;
}

/*
 * 功能描述：此函数将一个字符串转换为十进制整数
 * 入参说明：dest --- 用于保存结果的指针
 *           str --- 要转换的字符串
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int string_to_dec(unsigned long *dest, const char *str) {
    unsigned long result = 0;

    while (*str) {
        if ((*str >= '0') && (*str <= '9')) {
            result = result * 10 + (*str - '0');
        } else if (*str != ' ') {
            return -1; /* 非法字符，错误 */
        }
        str++;
    }

    *dest = result;

    return 0;
}

/*
 * 功能描述：此函数将一个字符串转换为十进制整数（指定长度）
 * 入参说明：dest --- 用于保存结果的指针
 *           str --- 要转换的字符串
 *           len --- 字符串长度
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int string_to_dec_n(unsigned int *dest, const char *str, int len) {
    unsigned int result = 0;

    while (len > 0) {
        if ((*str >= '0') && (*str <= '9')) {
            result = result * 10 + (*str - '0');
        } else {
            return -1; /* 非法字符，错误 */
        }
        str++;
        len--;
    }

    *dest = result;

    return 0;
}

/*
 * 功能描述：此函数将一个字符串转换为十进制长整数（指定长度）
 * 入参说明：dest --- 用于保存结果的指针
 *           str --- 要转换的字符串
 *           len --- 字符串长度
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int string_to_lldec_n(long long int *dest, const char *str, int len) {
    long long int result = 0;
    int           flag   = 1;

    if (*str == '-') {
        flag = -1;
        str++;
        len--;
    }

    while (len > 0) {
        if ((*str >= '0') && (*str <= '9')) {
            result = result * 10 + (*str - '0');
        } else {
            return -1; /* 非法字符，错误 */
        }
        str++;
        len--;
    }

    *dest = flag * result;

    return 0;
}

/*
 * 功能描述：此函数将一个字符串转换为十六进制整数
 * 入参说明：dest --- 用于保存结果的指针
 *           str --- 要转换的字符串
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int string_to_hex(unsigned long *dest, const char *str) {
    unsigned long result = 0;

    while (*str) {
        if ((*str >= '0') && (*str <= '9')) {
            result = (result << 4) + (*str - '0');
        } else if ((*str >= 'a') && (*str <= 'f')) {
            result = (result << 4) + (*str - 'a' + 10);
        } else if ((*str >= 'A') && (*str <= 'F')) {
            result = (result << 4) + (*str - 'A' + 10);
        } else {
            return -1; /* 非法字符，错误 */
        }

        str++;
    }

    *dest = result;

    return 0;
}

/*
 * 功能描述：此函数将一个字符串转换为十进制浮点数
 * 入参说明：dest --- 用于保存结果的指针
 *           str --- 要转换的字符串
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int string_to_float(float *dest, const char *str) {
    int flag   = 0;
    int result = 0;
    int div    = 1; /* 计算'.'后的数字字符个数 */
    int symbol = 1;

    if (*str == '-') {
        str++;
        symbol *= -1;
    }

    while (*str) {
        if (flag == 1) {
            div *= 10;
        }

        if (*str == '.') {
            flag = 1;                                /* 启用计算'.'后的数字字符个数 */
        } else if ((*str >= '0') && (*str <= '9')) { /* 仅处理数字字符：'0' 到 '9' */
            result = result * 10 + (*str - '0');
        } else {
            return -1; /* 非法字符，错误 */
        }

        str++;
    }

    *dest = symbol * (float)result / (float)div;

    return 0;
}

/*
 * 功能描述：此函数将一个字符串转换为十进制浮点数（指定长度）
 * 入参说明：dest --- 用于保存结果的指针
 *           str --- 要转换的字符串
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int string_to_float_len(float *dest, const char *str, int len) {
    int flag   = 0;
    int result = 0;
    int div    = 1; /* 计算'.'后的数字字符个数 */

    while (len--) {
        if (flag == 1) {
            div *= 10;
        }

        if (*str == '.') {
            flag = 1;                                /* 启用计算'.'后的数字字符个数 */
        } else if ((*str >= '0') && (*str <= '9')) { /* 仅处理数字字符：'0' 到 '9' */
            result = result * 10 + (*str - '0');
        } else {
            return -1; /* 非法字符，错误 */
        }

        str++;
    }

    *dest = (float)result / (float)div;

    return 0;
}

/*
 * 功能描述：此函数将一个字符串转换为十进制双精度浮点数
 * 入参说明：dest --- 用于保存结果的指针
 *           str --- 要转换的字符串
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int string_to_double(double *dest, const char *str) {
    int       flag   = 0;
    long long result = 0;
    int       div    = 1; /* 计算'.'后的数字字符个数 */

    while (*str) {
        if (flag == 1) {
            div *= 10;
        }

        if (*str == '.') {
            flag = 1;                                /* 启用计算'.'后的数字字符个数 */
        } else if ((*str >= '0') && (*str <= '9')) { /* 仅处理数字字符：'0' 到 '9' */
            result = result * 10 + (*str - '0');
        } else {
            return -1; /* 非法字符，错误 */
        }

        str++;
    }

    *dest = (double)result / (double)div;

    return 0;
}

/*
 * 功能描述：此函数将一个字符串转换为十进制浮点数（指定长度）
 * 入参说明：dest --- 用于保存结果的指针
 *           str --- 要转换的字符串
 * 返 回 值：0 --- 成功 其他 --- 失败
 */
int string_to_double_len(double *dest, const char *str, int len) {
    int flag   = 0;
    int result = 0;
    int div    = 1; /* 计算'.'后的数字字符个数 */
    int symbol = 1;

    if (*str == '-') {
        str++;
        symbol *= -1;
        len--;
    }

    while (len--) {
        if (flag == 1) {
            div *= 10;
        }

        if (*str == '.') {
            flag = 1;                                /* 启用计算'.'后的数字字符个数 */
        } else if ((*str >= '0') && (*str <= '9')) { /* 仅处理数字字符：'0' 到 '9' */
            result = result * 10 + (*str - '0');
        } else {
            return -1; /* 非法字符，错误 */
        }

        str++;
    }

    *dest = symbol * (double)result / (double)div;

    return 0;
}

/*
 * 功能描述：此函数将十进制整数转换为字符串
 * 入参说明：str --- 目标字符串
 *           value --- 要转换的十进制整数
 * 返 回 值：指向目标字符串末尾('\0')的指针
 */
char *dec_to_string(char *str, unsigned long value) {
    char *start = str;
    char *end;
    char  ch;

    /* 处理0 */
    if (value == 0) {
        *str++ = '0';
        *str   = '\0';
        return str;
    }

    /* 转换（从低到高） */
    while (value > 0) {
        ch     = value % 10;
        value  = value / 10;
        *str++ = ch + '0';
    }

    *str = '\0'; /* 终止符 */
    end  = str - 1;

    /* 反转 */
    while (start < end) {
        /* 交换*start和*end */
        ch     = *start;
        *start = *end;
        *end   = ch;

        /* 到下一对 */
        start++;
        end--;
    }

    /* str指向目标字符串末尾('\0') */
    return str;
}

/*
 * 功能描述：此函数将十六进制整数转换为字符串
 * 入参说明：str --- 目标字符串
 *           value --- 要转换的十六进制整数
 *           digits --- 十六进制位数
 * 返 回 值：指向目标字符串末尾('\0')的指针
 */
char *hex_to_string(char *str, unsigned long value, int digits) {
    char ch;

    for (--digits; digits >= 0; digits--) {
        ch = (value >> (digits << 2)) & 0x0f; /* ( value >> (digits * 4) ) & 0x0f */

        if (ch < 10) {
            *str++ = ch + '0';
        } else if (ch < 16) {
            *str++ = ch - 10 + 'A';
        }
    }

    *str = '\0'; /* 终止符 */

    return str;
}

/*
 * 功能描述：此函数将十进制浮点数转换为字符串
 * 入参说明：str --- 目标字符串
 *           value --- 有效值
 *           decimal --- 小数点后的位数
 * 返 回 值：指向目标字符串末尾('\0')的指针
 */
char *float_to_string(char *str, unsigned long value, int decimal) {
    char *start = str;
    char *end;
    char  ch;
    int   i;

    /* 转换（从低到高） */
    i = 0;
    while ((i < decimal) || (value > 0)) {
        ch     = value % 10;
        value  = value / 10;
        *str++ = ch + '0';
        i++;
        if (i == decimal) {
            *str++ = '.';
        }
    }

    if (i == decimal) {
        *str++ = '0';
    }

    *str = '\0'; /* 终止符 */
    end  = str - 1;

    /* 反转 */
    while (start < end) {
        /* 交换*start和*end */
        ch     = *start;
        *start = *end;
        *end   = ch;

        /* 到下一对 */
        start++;
        end--;
    }

    /* str指向目标字符串末尾('\0') */
    return str;
}

/*
 * 功能描述：此函数检查一个字符串是否全部为大写字母
 * 入参说明：buf --- 缓冲区指针
 *           len --- 字符串长度
 * 返 回 值：0 --- 全部为大写字母，其他 --- 失败
 */
int check_uppercase(char *buf, int len) {
    int i;

    for (i = 0; i < len; i++) {
        if (isupper(buf[i]) == 0) {
            return -1;
        }
    }

    return 0;
}

/*
 * 功能描述：此函数检查一个字符串是否全部为数字
 * 入参说明：buf --- 缓冲区指针
 *           len --- 字符串长度
 * 返 回 值：0 --- 全部为数字，其他 --- 失败
 */
int check_numcase(char *buf, int len) {
    int i;

    for (i = 0; i < len; i++) {
        if (isalnum(buf[i]) == 0) {
            return -1;
        }
    }

    return 0;
}

/*
 * 功能描述：此函数用于判断一个以'\0'结尾的字符串是否全部由数字字符组成
 * 入参说明：str --- 要判断的字符串
 * 返 回 值：true 如果字符串全部由数字组成；否则返回 false。
 */
bool is_pure_digit(const char *str) {
    /* 检查空字符串 */
    if (str == NULL || *str == '\0') {
        return -1;
    }

    /* 遍历字符串中的每个字符 */
    while (*str && *str != '\r') {
        if (*str < '0' || *str > '9') {
            return false;
        }
        str++;
    }

    return true;
}

/*
 * 功能描述：此函数用于判断一个以'\0'结尾的字符串是否全部由数字字符组成
 * 入参说明：str --- 要判断的字符串
 * 返 回 值：如果字符串是纯数字，则返回0；否则，包括空字符串在内，返回-1
 */
bool is_pure_digit_n(const char *str, int len) {
    /* 检查空指针或非法长度 */
    if (str == NULL || len <= 0) {
        return false;
    }

    /* 遍历字符串中的每个字符 */
    for (int i = 0; i < len; i++) {
        /* 如果字符不在 '0' 到 '9' 范围内，则返回 -1 */
        if (str[i] < '0' || str[i] > '9') {
            return false;
        }
    }

    return true;
}

/*
 * 功能描述：此函数用于查找字符串中第一次出现指定字符的位置
 * 入参说明：str --- 要判断的字符串
 *           int --- 最后的位置，即长度
 * 返 回 值：如果字符串位置不为'\0'，则返回当前位置；否则，返回NULL
 */
char *simple_strchr(const char *str, int c) {
    char ch = (char)c;
    while (*str != '\0') {
        if (*str == ch) {
            return (char *)str;
        }
        str++;
    }
    if (ch == '\0') {
        return (char *)str;
    }
    return NULL;
}

/*
 * 功能描述：此函数用于查找字符串中最后一次出现指定字符的位置
 * 入参说明：str --- 要判断的字符串
 *           c   --- 要查找的字符
 * 返 回 值：如果找到匹配字符，则返回其在字符串中的最后位置；否则，返回NULL
 */
char *simple_strrchr(const char *str, int c) {
    char  ch       = (char)c;
    char *last_pos = NULL; /* 记录最后出现的位置 */

    while (*str != '\0') {
        if (*str == ch) {
            last_pos = (char *)str; /* 记录最新的匹配位置 */
        }
        str++;
    }

    /* 如果要找的是 '\0'，则返回字符串结尾 */
    if (ch == '\0') {
        return (char *)str;
    }

    return last_pos;
}

/*
 * 功能描述：此函数用于根据一组指定的分隔符来将字符串分割成多个子字符串
 * 入参说明：str --- 要判断的字符串
 * 返 回 值：如果字符串位置不为'\0'，则返回当前位置；否则，返回NULL
 */
char *simple_strtok(char *str, const char *delim) {
    static char *last = NULL;
    if (str) {
        last = str;
    }
    if (last == NULL || *last == '\0') {
        return NULL;
    }
    char *start = last;
    while (*start && simple_strchr(delim, *start)) {
        start++;
    }
    if (*start == '\0') {
        last = start;
        return NULL;
    }
    char *end = start;
    while (*end && !simple_strchr(delim, *end)) {
        end++;
    }
    if (*end == '\0') {
        last = end;
    } else {
        *end = '\0';
        last = end + 1;
    }
    return start;
}

/*
 * 功能描述：此函数用于判断字符是否为数字
 * 入参说明：ch --- 要判断的字符
 * 返 回 值：如果字符为数字返回1，不为返回0
 */
int is_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

/*
 * 功能描述：此函数读取未对齐的 float 数据
 * 入参说明：addr --- 要读取的参数
 * 返 回 值：读取到的值
 */
float read_unaligned_float(char *addr) {
    float         value;
    unsigned int *value_ptr = (unsigned int *)&value;

    // 手动组合字节，确保使用正确的顺序
    *value_ptr = (addr[0] | (addr[1] << 8) | (addr[2] << 16) | (addr[3] << 24));

    return value;
}

/*
 * 功能描述：此函数用于判断一个以'\0'结尾的字符串是否全部由数字或者','字符组成
 * 入参说明：str --- 要判断的字符串
 * 返 回 值：如果字符串是数字或者','，则返回0；否则，包括空字符串在内，返回-1
 */
int is_pure_comma_digit(const char *str) {
    /* 检查空字符串 */
    if (*str == '\0') {
        return -1;
    }
    /* 遍历字符串中的每个字符 */
    while (*str) {
        if (*str < '0' || *str > '9') {
            if (',' == *str) {
                /* 不操作 */
            } else {
                return -1;
            }
        }
        str++;
    }

    return 0;
}

/*
 * 功能描述：此函数用于判断字符是否为大写字母
 * 入参说明：ch --- 要判断的字符
 * 返 回 值：如果字符为大写字母返回1，不为返回0
 */
int is_upper(char ch) {
    return ch >= 'A' && ch <= 'Z';
}

/*
 * 功能描述：这个函数处理RS485波特率命令
 * 入参说明：pkt_parse --- 输入UPP包指针
 *           outbuf --- 输出UPP包指针
 *           outlen --- 输出UPP包长度指针
 * 返 回 值：0 -- 成功，其它 -- 失败
 */
bool is_check_fmt_valid(const char *str) {
    char chk_str[4] = {0};
    strncpy(chk_str, str, 3);
    const char *valid_formats = "8N18O18E18n18o18e1";
    if (str[0] != '8' || strstr(valid_formats, chk_str) == NULL) {
        return false;
    }
    return true;
}

/*
 * 功能描述：这个函数检验设置的ip地址是否合法
 * 入参说明：ip  传入的地址字符串
 * 返 回 值：true -- 成功，false-- 失败
 */
bool is_valid_ip(const char *ip) {
    char *str[256];
    memcpy(str, ip, strlen(ip) + 1);

    char *token;
    char *next      = NULL;
    int   num_parts = 0;
    int   part;
    int   token_len     = 0;
    int   token_len_sum = 0;
    int   ip_len        = strlen(ip);

    token = strtok((char *)str, ".");
    while (token != NULL) {
        token_len = strlen(token);
        token_len_sum += token_len;
        if (token_len > 3 || token_len < 1) {
            return false;
        }
        if (is_dec_number(token, token_len) != 1 || token[0] == '-') {
            return false;
        }

        num_parts++;
        next = strtok(NULL, ".");
        if (sscanf(token, "%d", &part) != 1 || part < 0 || part > 255) {
            return false;
        }
        token = next;
    }

    if (num_parts != 4 || (token_len_sum + 3 != ip_len)) {
        return false;
    }

    return true;
}

/*
 * 功能描述：判断字符串是合法的十进制数
 * 入参说明：ip  传入的地址字符串
 * 返 回 值：0 -- 不是十进制数, 1 -- 是十进制整数，2-- 是十进制浮点数
 */
int is_dec_number(const char *str, const int strlen) {
    int number_len = 0;
    int cnt_point  = 0;
    int cnt_sign   = 0;
    /*禁止小数点开头*/
    if (str[0] == '.') {
        return 0;
    }
    /*禁止负号后不是数字*/
    if (str[0] == '-') {
        if (strlen < 2) {
            return 0;
        }
        if (str[1] < '0' || str[1] > '9') {
            return 0;
        }
        cnt_sign++;
    }

    for (int i = 0; i < strlen; i++) {
        if (str[i] - '0' <= 9 && str[i] - '0' >= 0) {
            number_len++;
        } else if (str[i] == '.') {
            cnt_point++;
        }
    }

    if ((number_len + cnt_sign == strlen) && (cnt_point == 0)) {
        return 1;
    }
    if ((cnt_point + number_len + cnt_sign == strlen) && (cnt_point == 1)) {
        return 2;
    }
    return 0;
}
