#pragma once
#include "HTTP_request.h"
#include "TCP_connection.h"

typedef int (*HTTP_service_process_func)(HTTP_request_t *);

typedef struct HTTP_service_map
{
    char *url;
    char *method;
    HTTP_service_process_func service_process_callback;
} HTTP_service_map_t;

typedef struct HTTP_service
{
    linked_list_t *HTTP_service_list;
} HTTP_service_t;

int HTTP_service_run();

int HTTP_service_process(HTTP_request_t *HTTP_request);
/// @brief 处理静态资源的函数，资源如果存在则写入response中
/// @param HTTP_request
/// @param HTTP_response
/// @return 如果资源存在返回200，不存在返回对应的错误码
int HTTP_service_process_static(HTTP_request_t *HTTP_request, HTTP_response_t *HTTP_response);

int HTTP_service_response_error(HTTP_response_t *HTTP_response);

int HTTP_service_process_test(HTTP_request_t *HTTP_request);