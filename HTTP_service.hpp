#pragma once
#include "HTTP_request.hpp"
#include "TCP_connection.hpp"
#include <vector>

class HTTP_service_map
{
public:
    using process_func = HTTP_STATUS (*)(HTTP_request *);
    std::string url;
    std::string method;
    process_func service_process_callback;
};

/// @brief HTTP_service采用单例模式
class HTTP_service
{
private:
    std::vector<HTTP_service_map> HTTP_service_list;
    HTTP_service();
    /// @brief 处理静态资源的函数，资源如果存在则写入response中
    /// @param HTTP_request
    /// @param HTTP_response
    /// @return 如果资源存在返回200，不存在返回对应的错误码
    HTTP_STATUS process_static(HTTP_request *request, HTTP_response *response);
    HTTP_STATUS response_error(HTTP_response *response);

public:
    // 单例模式
    static HTTP_service &instance()
    {
        static HTTP_service instance_;
        return instance_;
    }
    ~HTTP_service();
    int service_regist(const std::string url, const std::string method, HTTP_service_map::process_func func);
    HTTP_service_map::process_func get_process_func(std::string url, std::string method);
    int process(HTTP_request *request);
};

class HTTP_service_register
{
public:
    HTTP_service_register(const std::string url, const std::string method, HTTP_service_map::process_func func)
    {
        HTTP_service::instance().service_regist(url, method, func);
    }
};

#define HTTP_service_regist(URL, METHOD, FUNC) \
    static HTTP_service_register HTTP_service_##FUNC(URL, METHOD, FUNC);
