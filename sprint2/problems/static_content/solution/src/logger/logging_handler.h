#pragma once

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <string>
#include <string_view>

namespace logger {

namespace http = boost::beast::http;
namespace json = boost::json;

// 1. Компактные структуры данных
struct RequestLog {
    RequestLog(std::string ip_addr, const http::request<http::string_body>& req)
        : ip(std::move(ip_addr))
        , url(std::string(req.target()))
        , method(std::string(req.method_string())) {}

    std::string ip;
    std::string url;
    std::string method;
};

template <typename Body, typename Fields>
struct ResponseLog {
    ResponseLog(std::string ip_addr,
                long response_time,
                const http::response<Body, Fields>& res) // res — третий аргумент
        : ip(std::move(ip_addr))
        , response_time(response_time)
        , code(res.result_int())
        , content_type(std::string(res[http::field::content_type])) {}

    std::string ip;
    long response_time;
    int code;
    std::string content_type;
};

struct ServerAddrPortLog {
    std::string address;
    uint16_t port;
};

struct ExceptionLog {
    ExceptionLog(int code, std::string text, std::string where)
        : code(code), text(std::move(text)), where(std::move(where)) {}

    int code;
    std::string text;
    std::string where;
};


struct ExitCodeLog {
    int code;
};

// 2. Правила сериализации (tag_invoke)
// Реализацию для простых структур вынесем в .cpp, а для шаблонов оставим тут

void tag_invoke(json::value_from_tag, json::value& jv, const RequestLog& log);
void tag_invoke(json::value_from_tag, json::value& jv, const ServerAddrPortLog& log);
void tag_invoke(json::value_from_tag, json::value& jv, const ExceptionLog& log);
void tag_invoke(json::value_from_tag, json::value& jv, const ExitCodeLog& log);

template <typename Body, typename Fields>
void tag_invoke(json::value_from_tag, json::value& jv, const ResponseLog<Body, Fields>& log) {
    jv = {
        {"ip", log.ip},
        {"response_time", log.response_time},
        {"code", log.code},
        {"content_type", log.content_type}
    };
}

} // namespace logger
