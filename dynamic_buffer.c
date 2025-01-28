#define _GNU_SOURCE
#include "dynamic_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "log.h"

dynamic_buffer_t *dynamic_buffer_create(int capasicy)
{
    dynamic_buffer_t *dynamic_buffer = (dynamic_buffer_t *)malloc(sizeof(dynamic_buffer_t));
    if (dynamic_buffer == NULL)
    {
        perror("dynamic_buffer_create");
        return NULL;
    }

    dynamic_buffer->data = (char *)malloc(sizeof(char) * capasicy);
    memset(dynamic_buffer->data, 0, capasicy);
    dynamic_buffer->capacity = capasicy;
    dynamic_buffer->read_pos = 0;
    dynamic_buffer->write_pos = 0;
    pthread_mutex_init(&dynamic_buffer->mutex, NULL);
    return dynamic_buffer;
}

int dynamic_buffer_destroy(dynamic_buffer_t *dynamic_buffer)
{
    if (dynamic_buffer == NULL)
        return 0;

    if (dynamic_buffer->data != NULL)
    {
        free(dynamic_buffer->data);
        dynamic_buffer->data = NULL;
    }

    pthread_mutex_destroy(&dynamic_buffer->mutex);

    free(dynamic_buffer);
    dynamic_buffer = NULL;

    return 0;
}

int dynamic_buffer_available_write_size(dynamic_buffer_t *dynamic_buffer)
{
    return dynamic_buffer->capacity - dynamic_buffer->write_pos;
}

int dynamic_buffer_available_read_size(dynamic_buffer_t *dynamic_buffer)
{
    return dynamic_buffer->write_pos - dynamic_buffer->read_pos;
}

int dynamic_buffer_append_str(dynamic_buffer_t *dynamic_buffer, const char *buf)
{
    return dynamic_buffer_append_data(dynamic_buffer, buf, strlen(buf));
}

int dynamic_buffer_append_data(dynamic_buffer_t *dynamic_buffer, const char *buf, int buf_size)
{
    if (buf == NULL || dynamic_buffer == NULL)
    {
        perror("dynamic_buffer_append_str");
        return -1;
    }

    int size = buf_size;
    // 这里不判断长度是否足够，因为expand中会判断
    //  if (dynamic_buffer_available_write_size(dynamic_buffer) < size)
    int ret = dynamic_buffer_expand(dynamic_buffer, size);
    if (ret == -1)
    {
        perror("dynamic_buffer_append_str");
        return -1;
    }

    // 数据拷贝
    memcpy(dynamic_buffer->data + dynamic_buffer->write_pos, buf, size);
    dynamic_buffer->write_pos += size;
    return 0;
}

int dynamic_buffer_expand(dynamic_buffer_t *dynamic_buffer, int str_size)
{
    if (dynamic_buffer == NULL)
    {
        perror("dynamic_buffer_expand");
        return -1;
    }

    // 检查
    if (dynamic_buffer_available_write_size(dynamic_buffer) >= str_size)
        return 0;

    // 重新计算所需要的大小，正确的大小应该为str_size - 当前可以写入的大小
    int expand_size = str_size - dynamic_buffer_available_write_size(dynamic_buffer);

    int old_avilable_read_size = dynamic_buffer_available_read_size(dynamic_buffer);
    // 有已读的空间
    if (dynamic_buffer->read_pos != 0)
    {
        // 将后面的可读内容移至最前面
        memcpy(dynamic_buffer->data, dynamic_buffer->data + dynamic_buffer->read_pos, old_avilable_read_size);
        // 更新pos
        dynamic_buffer->read_pos = 0;
        dynamic_buffer->write_pos = old_avilable_read_size;
        // 将原先的置空
        memset(dynamic_buffer->data + dynamic_buffer->write_pos, 0, dynamic_buffer_available_write_size(dynamic_buffer));
    }

    // 整理空间后，扩容
    char *new_data = (char *)realloc(dynamic_buffer->data, dynamic_buffer->capacity + expand_size);
    if (new_data == NULL)
    {
        perror("dynamic_buffer_expand");
        return -1;
    }
    // 扩容成功的话，在对扩容的部分初始化
    memset(new_data + dynamic_buffer->capacity, 0, expand_size);
    dynamic_buffer->data = new_data;
    dynamic_buffer->capacity += expand_size;

    return 0;
}

char *dynamic_buffer_availabel_read_data(dynamic_buffer_t *dynamic_buffer)
{
    if (dynamic_buffer == NULL)
        return NULL;
    return dynamic_buffer->data + dynamic_buffer->read_pos;
}

char *dynamic_buffer_availabel_write_data(dynamic_buffer_t *dynamic_buffer)
{
    if (dynamic_buffer == NULL)
        return NULL;
    return dynamic_buffer->data + dynamic_buffer->write_pos;
}

char *dynamic_buffer_find_pos(dynamic_buffer_t *dynamic_buffer, char *str)
{
    // todo: 添加错误检查，例如：如果读取到了结尾、未找到字符串等

    // 使用menmen来获取要查找的数据在内存中的位置
    // 或者使用strstr也可以，不过strstr必须保证结尾是\0
    char *data_start = dynamic_buffer_availabel_read_data(dynamic_buffer);
    char *pos = memmem(data_start, dynamic_buffer_available_read_size(dynamic_buffer),
                       str, strlen(str));

    // 更新read_pos，相当于舍弃了data_start一直到str的这部分字符串
    if (pos != NULL)
        dynamic_buffer->read_pos += (pos - data_start) + strlen(str);

    return pos;
}

int dynamic_buffer_append_from(dynamic_buffer_t *dest_buffer, dynamic_buffer_t *src_buffer)
{
    // 每次按照1024的大小追加
    int size = 1024;
    while (1)
    {
        pthread_mutex_lock(&dest_buffer->mutex);
        // 获取实际可读的大小
        int available_src_size = dynamic_buffer_available_read_size(src_buffer);
        if (available_src_size == 0)
        {
            pthread_mutex_unlock(&dest_buffer->mutex);
            break;
        }
        // 最大1024
        size = available_src_size < size ? available_src_size : size;
        int ret = dynamic_buffer_append_data(dest_buffer, src_buffer->data, size);
        if (ret == -1)
        {
            LOG_ERROR("dynamic_buffer_append_from");
            return -1;
        }
        src_buffer->read_pos += size;
        pthread_mutex_unlock(&dest_buffer->mutex);
    }

    return 0;
}
