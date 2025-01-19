#include "HTTP_response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

HTTP_response_t *HTTP_response_create()
{
    HTTP_response_t *HTTP_response = (HTTP_response_t *)malloc(sizeof(HTTP_response));
    strcpy(HTTP_response->HTTP_version, "HTTP/1.1");
    strcpy(HTTP_response->status, "HTTP/1.1");
    HTTP_response->HTTP_response_headers = linked_list_create();

    return HTTP_response;
}

void HTTP_response_destroy(HTTP_response_t *HTTP_response)
{
    linked_list_destroy(HTTP_response->HTTP_response_headers, HTTP_response_header_destroy);
    return;
}

int HTTP_response_add_header(HTTP_response_t *HTTP_response, char *key, char *value)
{
    linked_list_push_tail(HTTP_response->HTTP_response_headers, HTTP_response_header_create(key, value));
    return 0;
}

int HTTP_response_build(HTTP_response_t *HTTP_response, int fd, dynamic_buffer_t *write_buffer)
{
    char buf[1024] = {0};

    // 构建响应行
    sprintf(buf, "%s %s %s\r\n", HTTP_response->HTTP_version, HTTP_response->status, HTTP_response->status_description);
    dynamic_buffer_append(write_buffer, buf);
    // 构建响应头
    linked_list_node_t *node = HTTP_response->HTTP_response_headers->head;
    while (node != NULL)
    {
        HTTP_response_header_t *header = (HTTP_response_header_t *)(node->data);
        sprintf(buf, "%s: %s\r\n", header->key, header->value);
        dynamic_buffer_append(write_buffer, buf);
    }

    // 构建空行
    dynamic_buffer_append(write_buffer, "\r\n");

    // todo: 构建响应体
    //  dynamic_buffer_append(write_buffer, content_body);

    return 0;
}
