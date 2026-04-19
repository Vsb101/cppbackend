#pragma once

/**
 * @file logging_handler.h
 * @brief Структуры для JSON-логирования
 */

#include <boost/beast/http.hpp>
#include <boost/json.hpp>

#include <string>
#include <string_view>

namespace logger {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

using HttpRequest = http::request<http::string_body>;

// Ключи JSON для логирования
constexpr std::string_view kIp = "ip";
constexpr std::string_view kUrl = "URI";
constexpr std::string_view kMethod = "method";
constexpr std::string_view kResponseTime = "response_time";
constexpr std::string_view kCode = "code";
constexpr std::string_view kContentType = "content_type";
constexpr std::string_view kPort = "port";
constexpr std::string_view kAddress = "address";
constexpr std::string_view kText = "text";
constexpr std::string_view kWhere = "where";

/**
 * @brief Данные HTTP-запроса для логирования
 */
struct RequestLog {
    RequestLog(std::string_view ip_addr, const HttpRequest& req)
        : ip(ip_addr)
        , url(req.target())
        , method(std::string(req.method_string())) {}

    std::string ip;  // Владей данными (безопаснее string_view)
    std::string_view url;
    std::string method;
};

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                const RequestLog& request);

/**
 * @brief Данные HTTP-ответа для логирования
 */
template <typename Body, typename Fields>
struct ResponseLog {
    ResponseLog(std::string ip_addr,
                long response_time,
                const http::response<Body, Fields>& res)
        : ip(std::move(ip_addr))
        , response_time(response_time)
        , code(res.result_int())
        , content_type(std::string(res[http::field::content_type])) {}

    std::string ip;
    long response_time;
    int code;
    std::string content_type;
};

template <typename Body, typename Fields>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                const ResponseLog<Body, Fields>& response) {
    json_value = {{kIp, json::value_from(response.ip)},
                  {kResponseTime, json::value_from(response.response_time)},
                  {kCode, json::value_from(response.code)},
                  {kContentType, json::value_from(response.content_type)}};
}

/**
 * @brief Лог адреса и порта сервера
 */
struct ServerAddrPortLog {
    ServerAddrPortLog(std::string addr, uint32_t port)
        : address(std::move(addr)), port(port) {}

    std::string address;
    uint32_t port;
};

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                const ServerAddrPortLog& server_address);

/**
 * @brief Лог исключения
 */
struct ExceptionLog {
    ExceptionLog(int code, std::string_view text, std::string_view where)
        : code(code), text(text), where(where) {}

    int code;
    std::string_view text;
    std::string_view where;
};

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                const ExceptionLog& exception);

/**
 * @brief Лог кода выхода
 */
struct ExitCodeLog {
    explicit ExitCodeLog(int c) : code(c) {}

    int code;
};

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                const ExitCodeLog& exit_code);

}  // namespace logger

