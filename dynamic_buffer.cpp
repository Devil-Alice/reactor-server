// #define _GNU_SOURCE
#include "dynamic_buffer.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include "log.h"
#include "dynamic_buffer.hpp"

Dynamic_buffer::Dynamic_buffer(int capacity)
{
    this->data = (char *)malloc(sizeof(char) * capacity);
    // memset(dynamic_buffer->data, 0, capacity);
    bzero(data, capacity);
    this->capacity = capacity;
    this->read_pos = 0;
    this->write_pos = 0;
}

Dynamic_buffer::~Dynamic_buffer()
{
    if (data != NULL)
    {
        free(data);
        data = NULL;
    }
}

int Dynamic_buffer::append_string(const char *str)
{
    return append_string(str, strlen(str));
}

int Dynamic_buffer::append_string(const char *str, int str_len)
{
    if (str == NULL)
    {
        LOG_ERROR("dynamic_buffer_append_str error");
        return -1;
    }

    mutex.lock();
    int size = str_len;
    // 这里不判断长度是否足够，因为expand中会判断
    //  if (dynamic_buffer_available_write_size(dynamic_buffer) < size)
    int ret = expand(size);
    if (ret == -1)
    {
        LOG_ERROR("dynamic_buffer_append_str error");
        return -1;
    }

    // 数据拷贝
    memcpy(data + write_pos, str, size);
    write_pos += size;
    mutex.unlock();
    return 0;
}

int Dynamic_buffer::expand(int str_len)
{
    // 检查
    if (available_write_size() >= str_len)
        return 0;

    // 重新计算所需要的大小，正确的大小应该为str_size - 当前可以写入的大小
    int expand_size = str_len - available_write_size();

    int old_avilable_read_size = available_read_size();
    // 有已读的空间
    if (read_pos != 0)
    {
        // 将后面的可读内容移至最前面
        memcpy(data, data + read_pos, old_avilable_read_size);
        // 更新pos
        read_pos = 0;
        write_pos = old_avilable_read_size;
        // 将原先的置空
        memset(data + write_pos, 0, available_write_size());
    }

    // 整理空间后，扩容
    char *new_data = (char *)realloc(data, capacity + expand_size);
    if (new_data == NULL)
    {
        LOG_ERROR("dynamic_buffer_expand");
        return -1;
    }
    // 扩容成功的话，在对扩容的部分初始化
    memset(new_data + capacity, 0, expand_size);
    data = new_data;
    capacity += expand_size;

    return 0;
}

char *Dynamic_buffer::find_pos(std::string str)
{
    // todo: 添加错误检查，例如：如果读取到了结尾、未找到字符串等

    // 使用menmen来获取要查找的数据在内存中的位置
    // 或者使用strstr也可以，不过strstr必须保证结尾是\0
    char *data_start = availabel_read_data();
    char *pos = (char *)memmem(data_start, available_read_size(),
                               str.data(), strlen(str.data()));

    // 更新read_pos，相当于舍弃了data_start一直到str的这部分字符串
    if (pos != NULL)
        read_pos += (pos - data_start) + strlen(str.data());

    return pos;
}
