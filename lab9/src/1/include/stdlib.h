#ifndef STDLIB_H
#define STDLIB_H

#include "os_type.h"

template<typename T>
void swap(T &x, T &y);

/*
 * 将一个非负整数转换为指定进制表示的字符串。
 * num: 待转换的非负整数。
 * mod: 进制。
 * numStr: 保存转换后的字符串，其中，numStr[0]保存的是num的高位数字，以此类推。
 */

void itos(char *numStr, uint32 num, uint32 mod);
// 将从memory开始的length个字节设置为value
void memset(void *memory, char value, int length);
// 上取整
int ceil(const int dividend, const int divisor);
// 内存复制，将src开始的length个字节复制到dst中
void memcpy(void *src, void *dst, uint32 length);
// 字符串复制
void strcpy(const char *src, char *dst);
// 字符串长度
int strlen(const char *str);
// 字符串比较
int strcmp(const char *s1, const char *s2);
// 字符串前缀比较
bool strncmp(const char *s1, const char *s2, int n);
#endif