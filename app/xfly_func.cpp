#include <cstring>
#include "xfly_func.h"

//
// Created by zaiheshi on 1/20/23.
//
void lStrip(char *str) {
    if (str == nullptr)
        return;
    size_t length = strlen(str);
    if (length == 0)
        return;
    // 定位第一个非空白字符
    const char *skipCharacter = "\n\t\r ";
    int p;
    for (p = 0; p < length; ++p) {
        if (strchr(skipCharacter, str[p]) == nullptr)
            break;
    }

    // 全是空白字符
    if (p == length) {
        *str = '\0';
        return;
    }
    // 其它情况
    char* tmp = str+p;
    while (*tmp) {
        *str = *tmp;
        str++;
        tmp++;
    }
    *str = '\0';
}

void rStrip(char *str) {
    if (str == nullptr) return;
    size_t length = strlen(str);
    if (length == 0)
        return;
    const char *skipCharacter = "\n\t\r ";
    char *tmp = str + length - 1;
    while (strchr(skipCharacter, *tmp) && tmp != str)
        tmp--;
    // 全是空白字符
    if (tmp == str && strchr(skipCharacter, *tmp)) {
        *tmp = '\0';
    }
    tmp++;
    *tmp = '\0';
}
