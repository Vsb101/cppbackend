#pragma once
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <optional>
#include <string>
#include <string_view>

#include "../app/app.h"

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

class ApiHandler {
public:
    explicit ApiHandler(app::Application& app) : app_(app) {}

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto response = HandleRequest(req);
        send(std::move(response));
    }

private:
    app::Application& app_;

    // Основной роутер
    http::response<http::string_body> HandleRequest(const http::request<http::string_body>& req);

    // Обработчики эндпоинтов
    http::response<http::string_body> HandleGetMaps();
    http::response<http::string_body> HandleGetMapById(std::string_view id);
    http::response<http::string_body> HandleJoinGame(std::string_view body_str);
    http::response<http::string_body> HandleGetPlayers(std::string_view auth_header);
    http::response<http::string_body> HandleGetGameState(std::string_view auth_header); // <--- Проверь это
    http::response<http::string_body> HandlePlayerAction(std::string_view auth_header, std::string_view body_str);
    http::response<http::string_body> HandleTick(std::string_view body_str);

    // Вспомогательные функции
    http::response<http::string_body> MakeJsonResponse(http::status status, json::value body);
    http::response<http::string_body> MakeJsonResponseWithAllow(http::status status, json::value body, std::string_view allow);
    
    // Для проверки токенов
    std::optional<app::Token> TryExtractToken(std::string_view auth_header);
};

} // namespace http_handler
