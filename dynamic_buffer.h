#pragma once

typedef struct dynamic_buffer
{
    char *data;
    int capacity;
    int read_pos;
    int write_pos;
} dynamic_buffer_t;

dynamic_buffer_t *dynamic_buffer_create(int capasicy);
int dynamic_buffer_destroy(dynamic_buffer_t *dynamic_buffer);
/**
 * @brief 计算可用的写空间
 * @note 此处可用的写空间，指的是包括释放后的已读空间
 */
int dynamic_buffer_available_write_size(dynamic_buffer_t *dynamic_buffer);
int dynamic_buffer_available_read_size(dynamic_buffer_t *dynamic_buffer);
/**
 * @brief 写入数据，并根据当前的dynamic_buffer的使用情况，决定是否扩容
 * @note
 * 总是write_pos >= read_pos，因为无法读取还未写入的数据。
 * 如果已经读取了一部分内容，那么则可以将读取过的内容作为新空间使用。
 * 例如：| 已读 | 已写 | 空闲 | -> | 已写 | 已读(释放) + 空闲 |
 *
 * 所以，有以下几种情况：
 * 1. 剩余空间够用，不整理空间，也不扩容
 * 2. 剩余空间不够，整理之后，再扩容
 */
int dynamic_buffer_append(dynamic_buffer_t *dynamic_buffer, const char *buf);
/**
 * @param size 扩容的大小
 */
int dynamic_buffer_expand(dynamic_buffer_t *dynamic_buffer, int size);