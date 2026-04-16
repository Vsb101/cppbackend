#include "request_handler.h"
#include <fstream>
#include <string>
#include <boost/beast/http.hpp>
#include <boost/algorithm/string.hpp>

namespace http_handler {

namespace http = boost::beast::http;

RequestHandler::RequestHandler(app::Application& app, const std::filesystem::path& static_root)
    : api_handler_(app)
    , static_root_(static_root) {
}

void RequestHandler::operator()(http::request<http::string_body>&& req, SendCallback send) {
    RouteRequest(std::move(req), std::move(send));
}

void RequestHandler::RouteRequest(http::request<http::string_body>&& req, SendCallback send) {
    std::string target{req.target()};
    
    // API routes
    if (boost::algorithm::starts_with(target, "/api/v1/")) {
        std::string_view path = target;
        path.remove_prefix(8);  // убираем "/api/v1/"
        
        if (boost::algorithm::starts_with(path, "game/join")) {
            api_handler_.HandleJoinGame(std::move(req), std::move(send));
        } else if (boost::algorithm::starts_with(path, "game/players")) {
            api_handler_.HandleGetPlayers(std::move(req), std::move(send));
        } else if (boost::algorithm::starts_with(path, "maps")) {
            path.remove_prefix(4);  // убираем "maps"
            if (path.empty() || path == "/") {
                api_handler_.HandleMapsList(std::move(req), std::move(send));
            } else if (path.front() == '/') {
                path.remove_prefix(1);
                api_handler_.HandleMapById(std::move(req), std::move(send), path);
            } else {
                send(MakeNotFoundResponse(req));
            }
        } else {
            send(MakeNotFoundResponse(req));
        }
    } else {
        // Статические файлы
        HandleStaticFile(target, std::move(req), std::move(send));
    }
}

void RequestHandler::HandleStaticFile(const std::string& path, http::request<http::string_body>&& req, SendCallback send) {
    // Безопасный путь
    std::string safe_path = path;
    if (safe_path.empty() || safe_path == "/") {
        safe_path = "/index.html";
    }
    
    std::filesystem::path full_path = static_root_ / safe_path.substr(1);
    
    // Проверка на выход за пределы root
    auto canonical_root = std::filesystem::weakly_canonical(static_root_);
    auto canonical_full = std::filesystem::weakly_canonical(full_path);
    if (canonical_full.string().find(canonical_root.string()) != 0) {
        send(MakeNotFoundResponse(req));
        return;
    }
    
    // Чтение файла
    std::ifstream file(full_path, std::ios::binary);
    if (!file) {
        send(MakeNotFoundResponse(req));
        return;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    // Определение Content-Type
    std::string content_type = "application/octet-stream";
    if (boost::algorithm::ends_with(full_path.string(), ".html")) {
        content_type = "text/html";
    } else if (boost::algorithm::ends_with(full_path.string(), ".css")) {
        content_type = "text/css";
    } else if (boost::algorithm::ends_with(full_path.string(), ".js")) {
        content_type = "application/javascript";
    } else if (boost::algorithm::ends_with(full_path.string(), ".json")) {
        content_type = "application/json";
    }
    
    http::response<http::string_body> response{http::status::ok, req.version()};
    response.set(http::field::content_type, content_type);
    response.content_length(content.size());
    response.keep_alive(req.keep_alive());
    if (req.method() != http::verb::head) {
        response.body() = content;
    }
    send(std::move(response));
}

template<typename Body>
http::response<http::string_body> RequestHandler::MakeNotFoundResponse(const http::request<Body>& req) const {
    http::response<http::string_body> response{http::status::not_found, req.version()};
    response.set(http::field::content_type, "text/plain");
    response.body() = "Not Found";
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    return response;
}

template http::response<http::string_body> RequestHandler::MakeNotFoundResponse(const http::request<http::string_body>&) const;

}  // namespace http_handler
