#pragma once
#include "linked_list.h"
#include "TCP_connection.h"
#include "dynamic_buffer.h"

typedef struct TCP_connection TCP_connection_t;

enum HTTP_STATUS
{
    // 未定义状态
    HTTP_STATUS_UNKNOWN = 0,

    // Informational 1xx
    HTTP_STATUS_CONTINUE = 100,
    HTTP_STATUS_SWITCHING_PROTOCOLS = 101,
    HTTP_STATUS_PROCESSING = 102,

    // Success 2xx
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED = 201,
    HTTP_STATUS_ACCEPTED = 202,
    HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION = 203,
    HTTP_STATUS_NO_CONTENT = 204,
    HTTP_STATUS_RESET_CONTENT = 205,
    HTTP_STATUS_PARTIAL_CONTENT = 206,

    // Redirection 3xx
    HTTP_STATUS_MULTIPLE_CHOICES = 300,
    HTTP_STATUS_MOVED_PERMANENTLY = 301,
    HTTP_STATUS_FOUND = 302,
    HTTP_STATUS_SEE_OTHER = 303,
    HTTP_STATUS_NOT_MODIFIED = 304,
    HTTP_STATUS_TEMPORARY_REDIRECT = 307,
    HTTP_STATUS_PERMANENT_REDIRECT = 308,

    // Client Error 4xx
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_PAYMENT_REQUIRED = 402,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_NOT_ACCEPTABLE = 406,
    HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED = 407,
    HTTP_STATUS_REQUEST_TIMEOUT = 408,
    HTTP_STATUS_CONFLICT = 409,
    HTTP_STATUS_GONE = 410,
    HTTP_STATUS_LENGTH_REQUIRED = 411,
    HTTP_STATUS_PRECONDITION_FAILED = 412,
    HTTP_STATUS_PAYLOAD_TOO_LARGE = 413,
    HTTP_STATUS_URI_TOO_LONG = 414,
    HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE = 415,
    HTTP_STATUS_RANGE_NOT_SATISFIABLE = 416,
    HTTP_STATUS_EXPECTATION_FAILED = 417,
    HTTP_STATUS_UPGRADE_REQUIRED = 426,

    // Server Error 5xx
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_NOT_IMPLEMENTED = 501,
    HTTP_STATUS_BAD_GATEWAY = 502,
    HTTP_STATUS_SERVICE_UNAVAILABLE = 503,
    HTTP_STATUS_GATEWAY_TIMEOUT = 504,
    HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED = 505
};

typedef struct HTTP_response_header
{
    char key[64];
    char value[64];
} HTTP_response_header_t;

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
typedef struct HTTP_response
{
    TCP_connection_t *TCP_connection;

    // 状态行: 版本 状态码 状态码描述
    char HTTP_version[16];
    enum HTTP_STATUS status;
    char status_description[128];

    // 响应头，使用链表来存储，内部存放HTTP_response_header*
    linked_list_t *HTTP_response_headers;
    // dynamic_buffer_t *write_buffer;

} HTTP_response_t;

HTTP_response_header_t *HTTP_response_header_create(char *key, char *value);
void HTTP_response_header_destroy(HTTP_response_header_t *HTTP_response_header);
/// @brief 创建一个响应结构体
/// @param write_buffer 传入一个buffer，用于大文件的发送，达成边写边发的功能
/// @return
HTTP_response_t *HTTP_response_create(TCP_connection_t *TCP_connection);
void HTTP_response_destroy(HTTP_response_t *HTTP_response);
int HTTP_response_add_header(HTTP_response_t *HTTP_response, char *key, char *value);
/// @brief 用于构建响应数据的函数，也就是将HTTP_response所有的数据构建成一个完整的字符串，存入自身的write_buffer中（这个buffer指向外界）
/// @return 成功返回0，失败返回-1
/// @note 构建的数据不包括响应体，响应体数据由于可能是大文件数据，必须由使用者手动构建写入
int HTTP_response_build_header(HTTP_response_t *HTTP_response, int status, char* content_type, long int content_length);
char *HTTP_response_get_status_description(int status);
char *HTTP_response_get_content_type(const char *file_name);