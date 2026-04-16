#pragma once

#include "http_handler/api_handler.h"
#include <filesystem>
#include <string>
#include <functional>
#include <boost/beast/http.hpp>

namespace http_handler {

namespace http = boost::beast::http;

/**
 * @brief Роутер HTTP-запросов
 * 
 * Направляет запросы либо к ApiHandler (для /api/v1/*),
 * либо к обработчику статических файлов.
 */
class RequestHandler {
public:
    using SendCallback = std::function<void(http::response<http::string_body>&&)>;

    RequestHandler(app::Application& app, const std::filesystem::path& static_root);

    void operator()(http::request<http::string_body>&& req, SendCallback send);

private:
    ApiHandler api_handler_;
    std::filesystem::path static_root_;

    void RouteRequest(http::request<http::string_body>&& req, SendCallback send);
    void HandleStaticFile(const std::string& path, http::request<http::string_body>&& req, SendCallback send);
    
    template<typename Body>
    http::response<http::string_body> MakeNotFoundResponse(const http::request<Body>& req) const;
};

}  // namespace http_handler
