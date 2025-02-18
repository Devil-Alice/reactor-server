#include "my_service.hpp"

HTTP_service_regist("/test", "GET", HTTP_service_process_test);
HTTP_STATUS HTTP_service_process_test(HTTP_request *request)
{
    HTTP_response *response = new HTTP_response(request->get_tcp_connection());
    response->build_header(HTTP_STATUS::OK, HTTP_response::get_content_type(""), strlen("text"));

    response->get_tcp_connection()->get_write_buf()->append_string("test");
    return response->get_status();
}