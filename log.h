#ifndef __LOG_H_
#define __LOG_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#define LOG_ENABLE 1

#if LOG_ENABLE
#define LOG(type, fmt, ...)                                                          \
    do                                                                               \
    {                                                                                \
        time_t now = time(NULL);                                                     \
        struct tm *t = localtime(&now);                                              \
        char time_buf[20];                                                           \
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", t);                \
        printf("[%s:%d]: In function \'%s\'\r\n", __FILE__, __LINE__, __FUNCTION__); \
        printf("\t%s %s info: ", time_buf, type);                                    \
        printf(fmt, ##__VA_ARGS__);                                                  \
        printf("\r\n");                                                              \
    } while (0);
#define LOG_DEBUG(fmt, ...) LOG("DEBUG", fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)               \
    do                                    \
    {                                     \
        LOG("ERROR", fmt, ##__VA_ARGS__); \
        perror("error no");             \
        exit(1);                          \
    } while (0);

#else
#define LOG(type, fmt, ...)
#define LOG_DEBUG(fmt, ...)
#define LOG_ERROR(fmt, ...)

#endif

#endif