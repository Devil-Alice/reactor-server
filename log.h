#ifndef __LOG_H_
#define __LOG_H_
#include <stdio.h>
#include <stdarg.h>

#define LOG_ENABLE 1

#if LOG_ENABLE
#define LOG(type, fmt, ...)                                                        \
    do                                                                             \
    {                                                                              \
        printf("%s:%d: In function \'%s\'\r\n", __FILE__, __LINE__, __FUNCTION__); \
        printf("%s info: ", type);                                                 \
        printf(fmt, ##__VA_ARGS__);                                                \
        printf("\r\n");                                                            \
    } while (0);
#define LOG_DEBUG(fmt, ...) LOG("DEBUG", fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)               \
    do                                    \
    {                                     \
        LOG("ERROR", fmt, ##__VA_ARGS__); \
        exit(1);                          \
    } while (0);

#else
#define LOG(type, fmt, ...)
#define LOG_DEBUG(fmt, ...)
#define LOG_ERROR(fmt, ...)

#endif

#endif