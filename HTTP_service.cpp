#include "HTTP_service.hpp"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include "log.h"

HTTP_service::HTTP_service()
{
}

HTTP_service::~HTTP_service()
{
}

int HTTP_service::service_regist(const std::string url, const std::string method, HTTP_service_map::process_func func)
{
    HTTP_service_map service_map;
    service_map.url = url;
    service_map.method = method;
    service_map.service_process_callback = func;
    HTTP_service_list.push_back(service_map);
    return 0;
}

HTTP_service_map::process_func HTTP_service::get_process_func(std::string url, std::string method)
{
    int service_cnt = HTTP_service_list.size();
    for (int i = 0; i < service_cnt; i++)
    {
        HTTP_service_map service_map = HTTP_service_list[i];
        if (service_map.url.compare(url) == 0 && service_map.method.compare(method) == 0)
            return service_map.service_process_callback;
    }

    return nullptr;
}

int HTTP_service::process(HTTP_request *request)
{

    // 接收到了数据后，就需要对接收到的请求进行解析
    // 解析函数,解析http请求的内容
    HTTP_response *response = new HTTP_response(request->get_tcp_connection());

    request->parse_reqest();
    // 到目前为止HTTP_request已经有了请求的各种数据，例如method,url等，接下来就是处理这个请求，该如何给客户端发送响应
    // 处理http请求：

    LOG_DEBUG("HTTP service process requet(%s, %s)", request->get_method().data(), request->get_url().data());

    // 处理请求数据，完善response中的内容
    // 检查
    if (request == nullptr)
    {
        LOG_ERROR("HTTP_request_process_request");
        return false;
    }

    HTTP_STATUS ret = HTTP_STATUS::UNKNOWN;
    do
    {

        // 处理静态资源
        ret = process_static(request, response);
        if (ret == HTTP_STATUS::OK)
            break;

        // 处理自定义请求
        HTTP_service_map::process_func process_func = get_process_func(request->get_url(), request->get_method());
        if (process_func == NULL)
        {
            ret = HTTP_STATUS::NOT_FOUND;
            break;
        }

        ret = process_func(request);

    } while (0);

    // 请求处理完毕，资源不存在
    if (ret != HTTP_STATUS::OK)
    {
        response->set_status(ret);
        response_error(response);
    }

    delete response;

    return 0;
}

HTTP_STATUS HTTP_service::process_static(HTTP_request *request, HTTP_response *response)
{
    LOG_DEBUG("HTTP_service process static resource(.%s)", request->get_url().data());
    // 此时的url视为文件目录，具体位置为工作目录下的url位置，所以在解析路径，要在最前面加上 .
    std::string path = "." + request->get_url();
    struct stat file_stat;
    int ret = stat(path.data(), &file_stat);
    // 文件不存在，返回
    if (ret == -1)
    {
        LOG_DEBUG("static resource(%s) doesn't exist", path.data());
        return HTTP_STATUS::NOT_FOUND;
    }

    // 排除目录
    if (S_ISDIR(file_stat.st_mode) == 1)
    {
        LOG_DEBUG("static resource(%s) is a directory", path.data());
        return HTTP_STATUS::NOT_FOUND;
    }

    // 生成响应体数据
    int fd = open(path.data(), O_RDONLY);
    if (fd == -1)
    {
        LOG_DEBUG("HTTP_service_process_static open error");
        return HTTP_STATUS::INTERNAL_SERVER_ERROR;
    }

    // 构建响应头数据
    response->build_header(HTTP_STATUS::OK, response->get_content_type(path), file_stat.st_size);

    // 读取文件，写入buffer中
    int len = -1;
    char buf[1024] = {0};
    long int total_len = 0;
    while ((len = read(fd, buf, sizeof(buf))) > 0)
    {
        response->get_tcp_connection()->get_write_buf()->append_string(buf, len);
        total_len += len;
    }

    return HTTP_STATUS::OK;
}

HTTP_STATUS HTTP_service::response_error(HTTP_response *response)
{
    std::string path = "./" + std::to_string((int)response->get_status()) + ".html";
    struct stat file_stat;
    stat(path.data(), &file_stat);

    // 构建响应头数据
    response->build_header(response->get_status(), HTTP_response::get_content_type(path), file_stat.st_size);

    // 读取文件，写入buffer中
    int fd = open(path.data(), O_RDONLY);
    int len = -1;
    char buf[1024] = {0};
    while ((len = read(fd, buf, sizeof(buf))) > 0)
        response->get_tcp_connection()->get_write_buf()->append_string(buf, len);
    return response->get_status();
}
