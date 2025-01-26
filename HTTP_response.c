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
    HTTP_response_t *HTTP_response = (HTTP_response_t *)malloc(sizeof(HTTP_response_t));
    strcpy(HTTP_response->HTTP_version, "HTTP/1.1");
    HTTP_response->status = HTTP_STATUS_UNKNOWN;
    memset(HTTP_response->status_description, 0, sizeof(HTTP_response->status_description));
    HTTP_response->HTTP_response_headers = linked_list_create();
    HTTP_response->content = dynamic_buffer_create(1024);

    return HTTP_response;
}

void HTTP_response_destroy(HTTP_response_t *HTTP_response)
{
    linked_list_destroy(HTTP_response->HTTP_response_headers, (void *)HTTP_response_header_destroy);
    dynamic_buffer_destroy(HTTP_response->content);
    return;
}

int HTTP_response_add_header(HTTP_response_t *HTTP_response, char *key, char *value)
{
    linked_list_push_tail(HTTP_response->HTTP_response_headers, HTTP_response_header_create(key, value));
    return 0;
}

int HTTP_response_build(HTTP_response_t *HTTP_response, dynamic_buffer_t *write_buffer)
{
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
    }

    // 构建空行
    dynamic_buffer_append_str(write_buffer, "\r\n");
    // 构建响应体
    dynamic_buffer_append_from(write_buffer, HTTP_response->content);

    return 0;
}
