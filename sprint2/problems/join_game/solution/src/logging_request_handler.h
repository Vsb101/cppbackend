#pragma once

#include "logger.h"

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/json.hpp>
#include <chrono>
#include <functional>
#include <string>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace js = boost::json;
namespace net = boost::asio;

using namespace std::literals;

/**
 * @brief Декоратор для логирования запросов и ответов
 *
 * Обёртка над RequestHandler, которая добавляет логирование:
 * - Получение запроса
 * - Формирование ответа
 *
 * Паттерн: Декоратор
 */
template <typename SomeRequestHandler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(SomeRequestHandler& decorated)
        : decorated_(decorated) {
    }

    template <typename Body, typename Allocator, typename Send>
    void operator()(
        http::request<Body, http::basic_fields<Allocator>>&& req,
        Send&& send) {
        
        const std::string client_ip = "127.0.0.1";
        
        LogRequest(req, client_ip);
        
        auto start = std::chrono::steady_clock::now();
        
        decorated_(std::move(req), 
            [this, &start, send = std::forward<Send>(send)](auto&& response) mutable {
                auto end = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                
                LogResponse(response, duration.count());
                
                send(std::forward<decltype(response)>(response));
            });
    }

private:
    template <typename Body, typename Fields>
    void LogRequest(const http::request<Body, Fields>& req, const std::string& client_ip) {
        js::object data;
        data["ip"] = client_ip;
        data["URI"] = std::string(req.target());
        data["method"] = std::string(req.method_string());
        
        LogJson("request received", data);
    }

    template <typename Body, typename Fields>
    static void LogResponse(const http::response<Body, Fields>& resp, long response_time_ms) {
        js::object data;
        data["response_time"] = response_time_ms;
        data["code"] = resp.result_int();
        
        auto content_type_it = resp.find(http::field::content_type);
        if (content_type_it != resp.end()) {
            data["content_type"] = std::string(content_type_it->value());
        } else {
            data["content_type"] = nullptr;
        }
        
        LogJson("response sent", data);
    }

    SomeRequestHandler& decorated_;
};

}  // namespace http_handler
