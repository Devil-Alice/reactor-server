#pragma once
#include <iostream>
#include <map>
#include "dynamic_buffer.hpp"
#include "TCP_connection.hpp"
#include "HTTP_response.hpp"

/// @brief 描述http请求的结构体
/// @note 一个http请求如下
/// @note 这是请求行，被空格隔开的分别为：method，request target/url，http version：
/// @note GET /?username=alice&password=admin HTTP/1.1
/// @note 这是请求体，有多行，每一行都是一个健值对：
/// @note Host: 106.15.42.223:55555
/// @note User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/18.1.1 Safari/605.1.15
/// @note Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
/// @note Accept-Encoding: gzip, deflate
/// @note Accept-Language: zh-CN,zh-Hans;q=0.9
/// @note Priority: u=0, i
/// @note Upgrade-Insecure-Requests: 1
class HTTP_request
{
private:
    TCP_connection *tcp_connection;
    /// @brief 请求行的方法，例如GET、POST、DELETE、UPDATE等
    std::string method;
    std::string url;
    std::string HTTP_version;
    /// @brief 请求行的链表，该链表存储header
    std::map<std::string, std::string> request_headers;

public:
    HTTP_request(TCP_connection *connection);
    ~HTTP_request();

    TCP_connection *get_tcp_connection() { return tcp_connection; }
    std::string get_url() { return url; }
    std::string get_method() { return method; }

    void add_header(std::string key, std::string value);
    std::string get_header_value(std::string key);
    bool parse_request_line();
    /// @brief 解析一行请求头
    /// @param HTTP_request http请求对象
    /// @param read_buffer 动态buffer
    /// @return 返回值：1-正常，0-解析到了请求行的末尾空行，-1-解析错误
    int parse_reqest_header();
    /// @brief 解析HTTP请求，并将结果存储在HTTP_request中
    /// @return 是否解析成功
    bool parse_reqest();
};

/// @brief 在str_start到str_end的主字符串中，查找第一个以split_str分割的第一个子字符串（也就是str_start到split_str的子字符串）
/// @param set_str 传出参数，set_str是需要设置的字符串地址，注意这里为二级指针
/// @param str_start 主字符串的开头
/// @param str_end 主字符串的结尾
/// @param split_str 分割条件字符串，如果split_str为空，则返回整个主字符串
/// @return 返回查找到的子字符串的结尾
char *set_first_splited_string_to(std::string *set_str, char *str_start, char *str_end, std::string split_str);
