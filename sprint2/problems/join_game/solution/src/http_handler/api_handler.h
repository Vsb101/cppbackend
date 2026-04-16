#pragma once

#include "app/application.h"
#include <boost/beast/http.hpp>
#include <functional>
#include <string>

namespace http_handler {

namespace http = boost::beast::http;

/**
 * @brief Обработчик API запросов
 * 
 * Преобразует HTTP-запросы в вызовы Application и обратно.
 * Не знает о внутренней структуре model.
 */
class ApiHandler {
public:
    using SendCallback = std::function<void(http::response<http::string_body>&&)>;

    explicit ApiHandler(app::Application& app);

    void HandleJoinGame(http::request<http::string_body>&& req, SendCallback send);
    void HandleGetPlayers(http::request<http::string_body>&& req, SendCallback send);
    void HandleMapsList(http::request<http::string_body>&& req, SendCallback send);
    void HandleMapById(http::request<http::string_body>&& req, SendCallback send, std::string_view map_id);

private:
    app::Application& app_;

    template<typename Body>
    http::response<http::string_body> MakeResponse(
        http::status status,
        std::string_view body,
        std::string_view content_type,
        const http::request<Body>& req) const;

    template<typename Body>
    http::response<http::string_body> MakeError(
        const http::request<Body>& req,
        std::string_view code,
        std::string_view message,
        http::status status) const;
};

}  // namespace http_handler
