#include "HTTP_service.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include "log.h"

HTTP_service_t HTTP_service;

/// @brief http服务的注册函数
/// @param url
/// @param method
/// @param service_process_callback
/// @return
int HTTP_service_add_map(char *url, char *method, HTTP_service_process_func service_process_callback)
{
    HTTP_service_map_t *HTTP_service_map = (HTTP_service_map_t *)malloc(sizeof(HTTP_service_map_t));
    HTTP_service_map->url = url;
    HTTP_service_map->method = method;
    HTTP_service_map->service_process_callback = service_process_callback;
    linked_list_push_tail(HTTP_service.HTTP_service_list, HTTP_service_map);
    return 0;
}

HTTP_service_process_func HTTP_service_get_map(char *url, char *method)
{
    linked_list_node_t *node = HTTP_service.HTTP_service_list->head;
    while (node != NULL)
    {
        HTTP_service_map_t *HTTP_service_map = (HTTP_service_map_t *)node->data;
        // LOG_DEBUG("map url(%s)-url(%s), map method(%s)-method(%s)", HTTP_service_map->url, url, HTTP_service_map->method, method);
        if (strcasecmp(HTTP_service_map->url, url) == 0 && strcasecmp(HTTP_service_map->method, method) == 0)
            return HTTP_service_map->service_process_callback;

        node = node->next;
    }
    return NULL;
}

int HTTP_service_run()
{
    HTTP_service.HTTP_service_list = linked_list_create();
    HTTP_service_add_map("/test", "GET", HTTP_service_process_test);
    return 0;
}

int HTTP_service_process(HTTP_request_t *HTTP_request)
{

    // 接收到了数据后，就需要对接收到的请求进行解析
    // 解析函数,解析http请求的内容
    HTTP_response_t *HTTP_response = HTTP_response_create(HTTP_request->TCP_connection);

    HTTP_request_parse_reqest(HTTP_request);
    // 到目前为止HTTP_request已经有了请求的各种数据，例如method,url等，接下来就是处理这个请求，该如何给客户端发送响应
    // 处理http请求：

    LOG_DEBUG("processing requet > method(%s), url(%s)", HTTP_request->method, HTTP_request->url);

    // 处理请求数据，完善response中的内容
    // 检查
    if (HTTP_request == NULL)
    {
        perror("HTTP_request_process_request");
        return false;
    }

    int ret = 0;
    do
    {

        // 处理静态资源
        ret = HTTP_service_process_static(HTTP_request, HTTP_response);
        if (ret == HTTP_STATUS_OK)
            break;

        // 处理自定义请求
        HTTP_service_process_func process_func = HTTP_service_get_map(HTTP_request->url, HTTP_request->method);
        if (process_func == NULL)
        {
            ret = HTTP_STATUS_NOT_FOUND;
            break;
        }

        ret = process_func(HTTP_request);

    } while (0);

    // 请求处理完毕，资源不存在
    if (ret != HTTP_STATUS_OK)
    {
        HTTP_response->status = ret;
        HTTP_service_response_error(HTTP_response);
    }
    HTTP_response_destroy(HTTP_response);

    return 0;
}

int HTTP_service_process_static(HTTP_request_t *HTTP_request, HTTP_response_t *HTTP_response)
{
    LOG_DEBUG("processing static resource(.%s)", HTTP_request->url);
    // 此时的url视为文件目录，具体位置为工作目录下的url位置，所以在解析路径，要在最前面加上 .
    char path[128] = {0};
    sprintf(path, ".%s", HTTP_request->url);
    struct stat file_stat;
    int ret = stat(path, &file_stat);
    // 文件不存在，返回
    if (ret == -1)
    {
        LOG_DEBUG("static resource(%s) doesn't exist", path);
        return HTTP_STATUS_NOT_FOUND;
    }

    // 排除目录
    if (S_ISDIR(file_stat.st_mode) == 1)
    {
        LOG_DEBUG("static resource(%s) is a directory", path);
        return HTTP_STATUS_NOT_FOUND;
    }

    // 生成响应体数据
    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        LOG_DEBUG("HTTP_service_process_static open error");
        return -1;
    }

    // 构建响应头数据
    HTTP_response_build_header(HTTP_response, HTTP_STATUS_OK, HTTP_response_get_content_type(path), file_stat.st_size);

    // 读取文件，写入buffer中
    int len = -1;
    char buf[1024] = {0};
    while ((len = read(fd, buf, sizeof(buf))) > 0)
        dynamic_buffer_append_data(HTTP_response->TCP_connection->write_buf, buf, len);

    return HTTP_STATUS_OK;
}

int HTTP_service_response_error(HTTP_response_t *HTTP_response)
{
    char path[128] = {0};
    sprintf(path, "./%d.html", HTTP_response->status);
    struct stat file_stat;
    stat(path, &file_stat);

    // 构建响应头数据
    HTTP_response_build_header(HTTP_response, HTTP_response->status, HTTP_response_get_content_type(path), file_stat.st_size);

    // 读取文件，写入buffer中
    int fd = open(path, O_RDONLY);
    int len = -1;
    char buf[1024] = {0};
    while ((len = read(fd, buf, sizeof(buf))) > 0)
        dynamic_buffer_append_data(HTTP_response->TCP_connection->write_buf, buf, len);
    return 0;
}

int HTTP_service_process_test(HTTP_request_t *HTTP_request)
{
    HTTP_response_t *HTTP_response = HTTP_response_create(HTTP_request->TCP_connection);
    HTTP_response_build_header(HTTP_response, HTTP_STATUS_OK, HTTP_response_get_content_type(NULL), strlen("text"));

    dynamic_buffer_append_str(HTTP_response->TCP_connection->write_buf, "test");
    return 0;
}