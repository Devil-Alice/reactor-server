#include "HTTP_response.hpp"

std::string http_status_description_map[][2] = {
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

std::string content_type_map[][2] =
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

HTTP_response::HTTP_response(TCP_connection *tcp_connection)
{
    this->tcp_connection = tcp_connection;
    HTTP_version = "HTTP/1.1";
    status = HTTP_STATUS::UNKNOWN;
}

HTTP_response::~HTTP_response()
{
}

void HTTP_response::add_header(std::string key, std::string value)
{
    HTTP_response_headers.push(std::make_pair(key, value));
}

int HTTP_response::build_header(HTTP_STATUS status, std::string content_type, long int content_length)
{
    // 构建
    status = status;
    status_description = get_status_description(status);

    add_header("Content-Type", content_type);
    add_header("Content-Length", std::to_string(content_length));

    // 写入buffer
    Dynamic_buffer *write_buffer = tcp_connection->get_write_buf();
    std::string buf;

    // 构建响应行
    buf = HTTP_version + " " + std::to_string((int)status) + " " + status_description + "\r\n";
    write_buffer->append_string(buf.data());

    // 构建响应头
    while (!HTTP_response_headers.empty())
    {
        std::pair<std::string, std::string> node = HTTP_response_headers.front();

        buf = node.first + ": " + node.second + "\r\n";
        write_buffer->append_string(buf.data());

        HTTP_response_headers.pop();
    }

    // 构建空行
    write_buffer->append_string("\r\n");

    // 构建响应体
    // dynamic_buffer_append_from(write_buffer, HTTP_response->write_buffer);

    return 0;
}

std::string HTTP_response::get_status_description(HTTP_STATUS status)
{
    std::string stat_str = std::to_string((int)status);

    int map_size = sizeof(http_status_description_map) / sizeof(http_status_description_map[0]);

    for (int i = 0; i < map_size; i++)
        if (stat_str.compare(http_status_description_map[i][0]) == 0)
            return http_status_description_map[i][1];

    return "";
}

std::string HTTP_response::get_content_type(const std::string file_name)
{
    if (file_name.empty())
        return "text/plain";

    // 通过filename获取文件后缀
    int dot_pos = file_name.rfind('.');
    std::string extention = file_name.substr(dot_pos + 1);
    if (extention.empty())
        return "text/plain";

    int map_size = sizeof(content_type_map) / sizeof(content_type_map[0]);
    for (int i = 0; i < map_size; i++)
        if (extention.compare(content_type_map[i][0]) == 0)
            return content_type_map[i][1];

    return "text/plain";
}
