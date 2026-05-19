#pragma once

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <string>
#include <string_view>

namespace logger {

namespace http = boost::beast::http;
namespace json = boost::json;

// Лог HTTP-запроса: IP, URI, метод.
struct RequestLog {
    RequestLog(std::string ip_addr, const http::request<http::string_body>& req)
        : ip(std::move(ip_addr))
        , url(std::string(req.target()))
        , method(std::string(req.method_string())) {}

    std::string ip;
    std::string url;
    std::string method;
};

// Лог HTTP-ответа: IP, время, код, тип содержимого.
template <typename Body, typename Fields>
struct ResponseLog {
    ResponseLog(std::string ip_addr,
                long response_time,
                const http::response<Body, Fields>& res)
        : ip(std::move(ip_addr))
        , response_time(response_time)
        , code(res.result_int())
        , content_type(res.count(http::field::content_type) 
             ? std::string(res[http::field::content_type]) 
             : "null") {}

    std::string ip;
    int32_t response_time;
    int code;
    std::string content_type;
};

// Лог адреса и порта сервера.
struct ServerAddrPortLog {
    std::string address;
    uint16_t port;
};

// Лог исключения: код, текст, место.
struct ExceptionLog {
    ExceptionLog(int code, std::string text, std::string where)
        : code(code), text(std::move(text)), where(std::move(where)) {}

    int code;
    std::string text;
    std::string where;
};

// Лог кода выхода программы.
struct ExitCodeLog {
    int code;
};

// Сериализация в JSON.
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

}  // namespace logger