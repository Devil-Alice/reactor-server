#include "HTTP_response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *http_status_description_map[][2] = {
    // 未定义状态
    {"0", "Unknown Status"},

    // Informational 1xx
    {"100", "Continue"},
    {"101", "Switching Protocols"},
    {"102", "Processing"},

    // Success 2xx
    {"200", "OK"},
    {"201", "Created"},
    {"202", "Accepted"},
    {"203", "Non-Authoritative Information"},
    {"204", "No Content"},
    {"205", "Reset Content"},
    {"206", "Partial Content"},

    // Redirection 3xx
    {"300", "Multiple Choices"},
    {"301", "Moved Permanently"},
    {"302", "Found"},
    {"303", "See Other"},
    {"304", "Not Modified"},
    {"307", "Temporary Redirect"},
    {"308", "Permanent Redirect"},

    // Client Error 4xx
    {"400", "Bad Request"},
    {"401", "Unauthorized"},
    {"402", "Payment Required"},
    {"403", "Forbidden"},
    {"404", "Not Found"},
    {"405", "Method Not Allowed"},
    {"406", "Not Acceptable"},
    {"407", "Proxy Authentication Required"},
    {"408", "Request Timeout"},
    {"409", "Conflict"},
    {"410", "Gone"},
    {"411", "Length Required"},
    {"412", "Precondition Failed"},
    {"413", "Payload Too Large"},
    {"414", "URI Too Long"},
    {"415", "Unsupported Media Type"},
    {"416", "Range Not Satisfiable"},
    {"417", "Expectation Failed"},
    {"426", "Upgrade Required"},

    // Server Error 5xx
    {"500", "Internal Server Error"},
    {"501", "Not Implemented"},
    {"502", "Bad Gateway"},
    {"503", "Service Unavailable"},
    {"504", "Gateway Timeout"},
    {"505", "HTTP Version Not Supported"},

};

char *content_type_map[][2] =
    {
        {"html", "text/html"},
        {"htm", "text/html"},
        {"css", "text/css"},
        {"js", "application/javascript"},
        {"json", "application/json"},
        {"xml", "application/xml"},
        {"txt", "text/plain"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"ico", "image/x-icon"},
        {"svg", "image/svg+xml"},
        {"pdf", "application/pdf"},
        {"zip", "application/zip"},
        {"gz", "application/gzip"},
        {"mp3", "audio/mpeg"},
        // {"mp4", "video/mp4"},
        {"mp4", "text/plain"},
        {"wav", "audio/wav"},
        {"webm", "video/webm"},
        {"ogg", "audio/ogg"},
        {"woff", "font/woff"},
        {"woff2", "font/woff2"},
        {"ttf", "font/ttf"},
        {"otf", "font/otf"}};

HTTP_response_header_t *HTTP_response_header_create(char *key, char *value)
{
    HTTP_response_header_t *HTTP_response_header = (HTTP_response_header_t *)malloc(sizeof(HTTP_response_header_t));
    strcpy(HTTP_response_header->key, key);
    strcpy(HTTP_response_header->value, value);
    return HTTP_response_header;
}

void HTTP_response_header_destroy(HTTP_response_header_t *HTTP_response_header)
{
}

HTTP_response_t *HTTP_response_create(TCP_connection_t *TCP_connection)
{
    HTTP_response_t *HTTP_response = (HTTP_response_t *)malloc(sizeof(HTTP_response_t));
    HTTP_response->TCP_connection = TCP_connection;
    strcpy(HTTP_response->HTTP_version, "HTTP/1.1");
    HTTP_response->status = HTTP_STATUS_UNKNOWN;
    memset(HTTP_response->status_description, 0, sizeof(HTTP_response->status_description));
    HTTP_response->HTTP_response_headers = linked_list_create();

    return HTTP_response;
}

void HTTP_response_destroy(HTTP_response_t *HTTP_response)
{
    linked_list_destroy(HTTP_response->HTTP_response_headers, (void *)HTTP_response_header_destroy);
    return;
}

int HTTP_response_add_header(HTTP_response_t *HTTP_response, char *key, char *value)
{
    linked_list_push_tail(HTTP_response->HTTP_response_headers, HTTP_response_header_create(key, value));
    return 0;
}

int HTTP_response_build_header(HTTP_response_t *HTTP_response, int status, char *content_type, long int content_length)
{
    // 构建
    HTTP_response->status = status;
    sprintf(HTTP_response->status_description, "%s", HTTP_response_get_status_description(status));
    HTTP_response_add_header(HTTP_response, "Content-Type", content_type);
    char len_buf[64] = {0};
    sprintf(len_buf, "%ld", content_length);
    HTTP_response_add_header(HTTP_response, "Content-Length", len_buf);

    // 写入buffer
    dynamic_buffer_t *write_buffer = HTTP_response->TCP_connection->write_buf;
    char buf[1024] = {0};

    // 构建响应行
    sprintf(buf, "%s %d %s\r\n", HTTP_response->HTTP_version, HTTP_response->status, HTTP_response->status_description);
    dynamic_buffer_append_str(write_buffer, buf);
    // 构建响应头
    linked_list_node_t *node = HTTP_response->HTTP_response_headers->head;
    while (node != NULL)
    {
        HTTP_response_header_t *header = (HTTP_response_header_t *)(node->data);
        sprintf(buf, "%s: %s\r\n", header->key, header->value);
        dynamic_buffer_append_str(write_buffer, buf);
        node = node->next;
    }

    // 构建空行
    dynamic_buffer_append_str(write_buffer, "\r\n");
    // 构建响应体
    // dynamic_buffer_append_from(write_buffer, HTTP_response->write_buffer);

    return 0;
}

char *HTTP_response_get_status_description(int status)
{
    char stat_str[16];
    sprintf(stat_str, "%d", status);
    int map_size = sizeof(http_status_description_map) / sizeof(http_status_description_map[0]);

    for (int i = 0; i < map_size; i++)
        if (strcmp(stat_str, http_status_description_map[i][0]) == 0)
            return http_status_description_map[i][1];

    return NULL;
}

char *HTTP_response_get_content_type(const char *file_name)
{
    if (file_name == NULL)
        return "text/plain";
        
    // 通过filename获取文件后缀
    char *dot = strrchr(file_name, '.');
    char *ext = (dot + 1);
    int map_size = sizeof(content_type_map) / sizeof(content_type_map[0]);

    for (int i = 0; i < map_size; i++)
        if (strcasecmp(ext, content_type_map[i][0]) == 0)
            return content_type_map[i][1];

    return "text/plain";
}