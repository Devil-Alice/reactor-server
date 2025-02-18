// #define _GNU_SOURCE
#include "HTTP_request.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

HTTP_request::HTTP_request(TCP_connection *connection)
{
    tcp_connection = connection;
    method = "";
    url = "";
    HTTP_version = "";
    request_headers.clear();
}

HTTP_request::~HTTP_request()
{
}

void HTTP_request::add_header(std::string key, std::string value)
{
    request_headers.insert(std::make_pair(key, value));
}

std::string HTTP_request::get_header_value(std::string key)
{
    // 不存在
    if (request_headers.find(key) == request_headers.end())
        return "";

    return request_headers[key];
}

bool HTTP_request::parse_request_line()
{
    Dynamic_buffer *read_buffer = tcp_connection->get_read_buf();
    // 取出buffer中的第一行，此时取出的第一行不包括结尾的\r\n
    char *line_start = read_buffer->availabel_read_data();
    // 查找第一次\r\n也就是第一行的结尾在哪里
    
    char *line_end = read_buffer->find_pos("\r\n");
    // 计算第一行的长度

    if (line_start == NULL || line_end == NULL)
    {
        LOG_ERROR("HTTP_request_parse_request_line");
        return false;
    }

    int line_len = line_end - line_start;

    if (line_len <= 0)
    {
        LOG_ERROR("HTTP_request_parse_request_line line_len");
        return false;
    }

    // 解析请求行的method
    // 找到第一个空格在line中的位置，前面的部分就是method
    // 更新line_start，现在位置为method_end + 1，+1是因为空格
    line_start = set_first_splited_string_to(&method, line_start, line_end, " ");
    // 继续从第一个空格之后查找url
    line_start = set_first_splited_string_to(&url, line_start + 1, line_end, " ");
    // 剩下的就是http_version，结尾是\r\n的前一位
    set_first_splited_string_to(&HTTP_version, line_start + 1, line_end, "");

    return true;
}

int HTTP_request::parse_reqest_header()
{
    Dynamic_buffer *read_buffer = tcp_connection->get_read_buf();
    // 取出一行
    char *line_start = read_buffer->availabel_read_data();
    char *line_end = read_buffer->find_pos("\r\n");

    if (line_end == NULL)
        return -1;

    // 解析一行的健值对
    int line_len = line_end - line_start;
    // 注意：！！！这里冒号后边有一个空格，适用于标准的http协议数据；如果不是标准的，例如用户自定义构建的请求，可能不包含空格
    char *colon_pos = (char *)memmem(line_start, line_len, ": ", 2);

    // 没有搜到冒号，要分情况讨论，如果是请求头的最后一行，那就是只有\r\n的空行，没有冒号，并且这种情况相当于解析结束了
    if (colon_pos == NULL)
    {
        if (strncmp(line_start, "\r\n", 2) == 0)
            return 0;
        else
            return -1;
    }

    int key_len = colon_pos - line_start;
    char *key = (char *)malloc(sizeof(char) * key_len + 1);
    memcpy(key, line_start, colon_pos - line_start);
    key[key_len] = '\0';

    int value_len = line_end - (colon_pos + 2);
    char *value = (char *)malloc(sizeof(char) * value_len + 1);
    memcpy(value, colon_pos + 1, value_len);
    value[value_len] = '\0';

    // 将解析出的key、value添加进链表中
    add_header(key, value);

    return 1;
}

bool HTTP_request::parse_reqest()
{
    // todo: 失败时销毁request

    // 解析请求行
    int flag = parse_request_line();
    if (flag == false)
        return false;

    // 解析请求头
    int ret = 0;
    while (1)
    {
        ret = parse_reqest_header();
        if (ret == -1)
            return false;
        else if (ret == 0)
            break;
    }

    // 解析请求体（非get请求的操作）

    // 解析完毕后，处理这些数据，并发送响应回复

    return true;
}

char *set_first_splited_string_to(std::string *set_str, char *str_start, char *str_end, std::string split_str)
{
    char *sub_str = NULL;
    int str_len = str_end - str_start;

    // 这里以split_str为NULL的情况作为默认情况，因为这样可以提升代码复用
    char *sub_str_end = str_end;

    if (!split_str.empty())
        sub_str_end = (char *)memmem(str_start, str_len, split_str.data(), strlen(split_str.data()));

    int sub_str_len = sub_str_end - str_start;
    sub_str = (char *)malloc(sizeof(char) * sub_str_len + 1);
    memcpy(sub_str, str_start, sub_str_len);
    sub_str[sub_str_len] = '\0';

    // 将字符串设置到set_str中
    *set_str = sub_str;

    return sub_str_end;
}