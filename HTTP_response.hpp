#pragma once
#include "TCP_connection.hpp"
#include "dynamic_buffer.hpp"
#include <iostream>
#include <map>
#include <queue>

class TCP_connection;

enum class HTTP_STATUS
{
    // 未定义状态
    UNKNOWN = 0,

    // Informational 1xx
    CONTINUE = 100,
    SWITCHING_PROTOCOLS = 101,
    PROCESSING = 102,

    // Success 2xx
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NON_AUTHORITATIVE_INFORMATION = 203,
    NO_CONTENT = 204,
    RESET_CONTENT = 205,
    PARTIAL_CONTENT = 206,

    // Redirection 3xx
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    TEMPORARY_REDIRECT = 307,
    PERMANENT_REDIRECT = 308,

    // Client Error 4xx
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    PAYMENT_REQUIRED = 402,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    NOT_ACCEPTABLE = 406,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    REQUEST_TIMEOUT = 408,
    CONFLICT = 409,
    GONE = 410,
    LENGTH_REQUIRED = 411,
    PRECONDITION_FAILED = 412,
    PAYLOAD_TOO_LARGE = 413,
    URI_TOO_LONG = 414,
    UNSUPPORTED_MEDIA_TYPE = 415,
    RANGE_NOT_SATISFIABLE = 416,
    EXPECTATION_FAILED = 417,
    UPGRADE_REQUIRED = 426,

    // Server Error 5xx
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505
};

/// @brief http响应结构体
/// @note 使用固定大小的char，而不用指针的原因：，第一方便使用，第二因为response是我们自己构建的，一般大小不超过一个固定值；但是request有些header非常长，所以选择char指针
/// @note 一个简单的http响应包含：
/// @note http版本 状态码 状态码描述
/// @note Content-Type: 内容类型，用于浏览器识别格式
/// @note Content-Length: 内容长度
/// @note 空行
/// @note 内容数据...
/// @note
/// @note 例如：
/// @note HTTP/1.1 200 OK
/// @note Content-Type: text/plain
/// @note Content-Length: 13
/// @note \r\\n
/// @note Hello, world!
class HTTP_response
{
private:
    TCP_connection *tcp_connection;

    // 状态行: 版本 状态码 状态码描述
    std::string HTTP_version;
    HTTP_STATUS status;
    std::string status_description;

    // 响应头，使用链表来存储，内部存放HTTP_response_header*
    std::queue<std::pair<std::string, std::string>> HTTP_response_headers;
    // dynamic_buffer_t *write_buffer;

public:
    HTTP_response(TCP_connection *tcp_connection);
    ~HTTP_response();

    TCP_connection *get_tcp_connection() { return tcp_connection; }
    HTTP_STATUS get_status() { return status; }
    void set_status(HTTP_STATUS status) { this->status = status; }

    void add_header(std::string key, std::string value);
    /// @brief 用于构建响应数据的函数，也就是将HTTP_response所有的数据构建成一个完整的字符串，存入自身的write_buffer中（这个buffer指向外界）
    /// @return 成功返回0，失败返回-1
    /// @note 构建的数据不包括响应体，响应体数据由于可能是大文件数据，必须由使用者手动构建写入
    int build_header(HTTP_STATUS status, std::string content_type, long int content_length);
    static std::string get_status_description(HTTP_STATUS status);
    static std::string get_content_type(const std::string file_name);
};
