#pragma once

#include "request_handler.h"
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
        
        // Получаем IP клиента (используем "127.0.0.1" как заглушку, так как IP недоступен на этом уровне)
        // В реальной реализации IP нужно передавать через дополнительный параметр или получать из socket
        const std::string client_ip = "127.0.0.1";
        
        // Логгируем получение запроса
        LogRequest(req, client_ip);
        
        // Замеряем время обработки
        auto start = std::chrono::steady_clock::now();
        
        // Вызываем оригинальный обработчик
        decorated_(std::move(req), 
            [this, &start, send = std::forward<Send>(send)](auto&& response) mutable {
                // Замеряем время обработки
                auto end = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                
                // Логгируем ответ
                LogResponse(response, duration.count());
                
                // Отправляем ответ
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
        
        // Получаем content_type из заголовков
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
