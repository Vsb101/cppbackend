#pragma once

#include <filesystem>
#include <boost/beast/http.hpp>
#include "api/api_handler.h" // Относительный путь

namespace http_handler {

namespace http = boost::beast::http;

class RequestHandler {
public:
    explicit RequestHandler(app::Application& application,
                            std::filesystem::path static_content_path)
        : application_{application}
        , static_content_path_{std::move(static_content_path)}
        , api_handler_{application} {}

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        if (req.target().starts_with("/api/")) {
            return api_handler_(std::forward<decltype(req)>(req), std::forward<Send>(send));
        }
        
        // Пока static_handler не создан, вызываем заглушку
        HandleStatic(std::forward<decltype(req)>(req), std::forward<Send>(send));
    }

private:
    template <typename Body, typename Allocator, typename Send>
    void HandleStatic(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Временная заглушка: возвращаем 404
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::content_type, "text/plain");
        res.body() = "Static content handler is under construction...";
        res.prepare_payload();
        send(std::move(res));
    }

    app::Application& application_;
    std::filesystem::path static_content_path_;
    http_handler::ApiHandler api_handler_; 
};

} // namespace http_handler