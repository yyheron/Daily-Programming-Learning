#pragma once
#include <cstdio>

// 日志级别控制宏（默认精简模式）
#ifndef LOG_LEVEL
#define LOG_LEVEL 1 // 0: 精简日志, 1: 全量日志
#endif

#define LOGERRLINE(fmt, ...) \
    fprintf(stderr, "[%d] " fmt "\n", __LINE__, ##__VA_ARGS__);

#if LOG_LEVEL == 1
#define LOGWARNLINE(fmt, ...) \
    fprintf(stderr, "[%d] " fmt "\n", __LINE__, ##__VA_ARGS__);
#else
#define LOGWARNLINE(fmt, ...) \
    do {} while(0); // 空语句避免编译警告
#endif

#if LOG_LEVEL == 1
#define LOGINFOLINE(fmt, ...) \
    fprintf(stdout, "[%d] " fmt "\n", __LINE__, ##__VA_ARGS__);
#else
#define LOGINFOLINE(fmt, ...) \
    do {} while(0); // 空语句避免编译警告
#endif

#if LOG_LEVEL == 1
#define LOGDEBUGLINE(fmt, ...) \
    fprintf(stderr, "[%d] " fmt "\n", __LINE__, ##__VA_ARGS__);
#else
#define LOGDEBUGLINE(fmt, ...) \
    do {} while(0); // 空语句避免编译警告
#endif

