/*
 * 版权所有 (C) 2011 西安和其光电科技股份有限公司，保留所有权利。
 * 文 件 名：hq_string.h
 * 文件描述：字符串库
 * 作    者：和其光电嵌软团队
 * 备    注：函数参数未经检查，所以必须是有效的指针
 * 更新记录：
 *        V1.0 2024/01/26 陈  军  初始版本
 *        V1.1 2025/03/10 张晓博  加入string_to_double_len
 *        V1.2 2025/03/18 李兆越  加入 simple_strrchr，查找最后一次出现的字符
 *           V1.3 2025/03/21 李兆越  加入 is_pure_comma_digit
 *           V1.4 2025/04/03 李兆越  加入 is_pure_digit_n
 *           V1.5 2025/04/24 李兆越  add is_upper
 */

#ifndef _HQ_STRING_H_
#define _HQ_STRING_H_

#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

int         string_length(const char *str);
char       *string_copy(char *dest, const char *src);
char       *string_copy_n(char *dest, const char *src, int len);
int         string_comp(const char *str1, const char *str2);
int         string_comp_n(const char *str1, const char *str2, int len);
void        string_reverse(char *str);
const char *string_extract(char *dest, const char *src, char sp);
const char *string_search(const char *str, const char *sub);
int         string_to_dec(unsigned long *dest, const char *str);
int         string_to_dec_n(unsigned int *dest, const char *str, int len);
int         string_to_lldec_n(long long int *dest, const char *str, int len);
int         string_to_hex(unsigned long *dest, const char *str);
int         string_to_float(float *dest, const char *str);
int         string_to_float_len(float *dest, const char *str, int len);
int         string_to_double(double *dest, const char *str);
int         string_to_double_len(double *dest, const char *str, int len);
char       *dec_to_string(char *str, unsigned long value);
char       *hex_to_string(char *str, unsigned long value, int digits);
char       *float_to_string(char *str, unsigned long value, int decimal);
int         check_uppercase(char *buf, int len);
int         check_numcase(char *buf, int len);
bool        is_pure_digit(const char *str);
bool        is_pure_digit_n(const char *str, int len);
int         is_pure_comma_digit(const char *str);
char       *simple_strchr(const char *str, int c);
char       *simple_strrchr(const char *str, int c);
char       *simple_strtok(char *str, const char *delim);
int         is_digit(char ch);
float       read_unaligned_float(char *addr);
int         is_upper(char ch);
bool        is_check_fmt_valid(const char *str);
bool        is_valid_ip(const char *ip);
int         is_dec_number(const char *str, const int strlen);
#endif /* _HQ_STRING_H_ */
