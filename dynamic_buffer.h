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
 * @brief 写入字符串，并根据当前的dynamic_buffer的使用情况，决定是否扩容
 * @note
 * 总是write_pos >= read_pos，因为无法读取还未写入的数据。
 * 如果已经读取了一部分内容，那么则可以将读取过的内容作为新空间使用。
 * 例如：| 已读 | 已写 | 空闲 | -> | 已写 | 已读(释放) + 空闲 |
 *
 * 所以，有以下几种情况：
 * 1. 剩余空间够用，不整理空间，也不扩容
 * 2. 剩余空间不够，整理之后，再扩容
 */
int dynamic_buffer_append_str(dynamic_buffer_t *dynamic_buffer, const char *buf);
// todo: 添加一个函数dynamic_buffer_append_data，该函数可以接受一个字符串长度的参数，用于一些特殊场景，例如read每次按长度读取，但是结尾不一定是\0

/// @param size 新的字符串的大小
int dynamic_buffer_expand(dynamic_buffer_t *dynamic_buffer, int str_size);
/// @brief 获取可用的读空间，也就是返回当前读位置的指针
char *dynamic_buffer_availabel_read_data(dynamic_buffer_t *dynamic_buffer);
/// @brief 获取可用的写空间，也就是返回当前写位置的指针
char *dynamic_buffer_availabel_write_data(dynamic_buffer_t *dynamic_buffer);
/// @brief 查找字符串str在dynamic_buffer.data中第一次出现的位置
/// @return 返回该位置的地址
/// @note 该函数会在最后更新read_pos为 read_pos+查找到的字符串位置+条件字符串的长度
/// @note 这么做是因为该函数返回一个可读的字符串，视为读操作，读过的字符串自然要从缓冲区中取出，方便后续的读操作
char *dynamic_buffer_find_pos(dynamic_buffer_t *dynamic_buffer, char *str);
int dynamic_buffer_append_from(dynamic_buffer_t *dest_buffer, dynamic_buffer_t *src_buffer);