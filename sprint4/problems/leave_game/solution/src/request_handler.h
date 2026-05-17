#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <iostream>
#include <boost/beast/http.hpp>
#include <boost/algorithm/string.hpp>
#include "api/api_handler.h"
#include "app/app.h"

namespace http_handler {

namespace http = boost::beast::http;
namespace fs = std::filesystem;
using namespace std::literals;

// Вспомогательные функции для работы со статикой
namespace static_utils {

// Определение MIME-типа по расширению (без учета регистра)
inline std::string_view GetMimeType(const fs::path& path) {
    using boost::iequals;
    auto ext = path.extension().string();
    
    if (iequals(ext, ".htm") || iequals(ext, ".html")) return "text/html"sv;
    if (iequals(ext, ".css")) return "text/css"sv;
    if (iequals(ext, ".txt")) return "text/plain"sv;
    if (iequals(ext, ".js")) return "text/javascript"sv;
    if (iequals(ext, ".json")) return "application/json"sv;
    if (iequals(ext, ".xml")) return "application/xml"sv;
    if (iequals(ext, ".png")) return "image/png"sv;
    if (iequals(ext, ".jpg") || iequals(ext, ".jpe") || iequals(ext, ".jpeg")) return "image/jpeg"sv;
    if (iequals(ext, ".gif")) return "image/gif"sv;
    if (iequals(ext, ".bmp")) return "image/bmp"sv;
    if (iequals(ext, ".ico")) return "image/vnd.microsoft.icon"sv;
    if (iequals(ext, ".tiff") || iequals(ext, ".tif")) return "image/tiff"sv;
    if (iequals(ext, ".svg") || iequals(ext, ".svgz")) return "image/svg+xml"sv;
    if (iequals(ext, ".mp3")) return "audio/mpeg"sv;
    
    return "application/octet-stream"sv;
}

// Декодирование URL (%20 -> пробел и т.д.)
inline std::string DecodeUrl(std::string_view url) {
    std::string res;
    res.reserve(url.size());
    for (size_t i = 0; i < url.size(); ++i) {
        if (url[i] == '%' && i + 2 < url.size()) {
            char high = url[i + 1];
            char low = url[i + 2];
            if (std::isxdigit(high) && std::isxdigit(low)) {
                auto hex_to_int = [](char c) {
                    if (c >= '0' && c <= '9') return c - '0';
                    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                    return 0;
                };
                res += static_cast<char>(hex_to_int(high) * 16 + hex_to_int(low));
                i += 2;
                continue;
            }
        }
        res += (url[i] == '+' ? ' ' : url[i]);
    }
    return res;
}

// Проверка, что путь находится внутри корневого каталога
inline bool IsSubPath(fs::path path, fs::path base) {
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) return false;
    }
    return true;
}

} // namespace static_utils

class RequestHandler {
public:
    explicit RequestHandler(app::Application& application, fs::path static_content_path)
        : application_{application}
        , static_content_path_{fs::weakly_canonical(std::move(static_content_path))}
        , api_handler_{application} {}

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        try {
            if (req.target().starts_with("/api/")) {
                // Передаем дальше в API
                return api_handler_(std::forward<decltype(req)>(req), std::forward<Send>(send));
            }
            // Обрабатываем статику
            HandleStatic(std::forward<decltype(req)>(req), std::forward<Send>(send));
        } catch (const std::exception& ex) {
            std::cerr << "RequestHandler exception: " << ex.what() << std::endl;
            
            // Если упало, отправляем 500 ошибку. Важно: используем std::move(send)
            send(MakeErrorResponse(
                http::status::internal_server_error, 
                R"({"code": "serverError", "message": "Internal server error"})"sv, 
                req.version(), 
                "application/json"sv
            ));
        }
    }

private:
    static http::response<http::string_body> MakeErrorResponse(
        http::status status, std::string_view body, unsigned version, std::string_view content_type) {
        http::response<http::string_body> res{status, version};
        res.set(http::field::content_type, content_type);
        res.set(http::field::cache_control, "no-cache"sv);
        res.body() = std::string(body);
        res.prepare_payload();
        return res;
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleStatic(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        const auto version = req.version();
        const auto method = req.method();

        std::string decoded_url = static_utils::DecodeUrl(req.target());
        std::string_view path_view = decoded_url;
        if (auto pos = path_view.find('?'); pos != std::string_view::npos) {
            path_view = path_view.substr(0, pos);
        }

        fs::path rel_path{path_view.substr(1)};
        fs::path abs_path = fs::weakly_canonical(static_content_path_ / rel_path);

        if (fs::is_directory(abs_path)) {
            abs_path /= "index.html"sv;
        }

        if (!static_utils::IsSubPath(abs_path, static_content_path_)) {
            return send(MakeErrorResponse(http::status::bad_request, "Bad Request"sv, version, "text/plain"sv));
        }

        http::file_body::value_type file;
        boost::system::error_code ec;
        file.open(abs_path.string().c_str(), boost::beast::file_mode::read, ec);

        if (ec) {
            if (ec == boost::system::errc::no_such_file_or_directory) {
                return send(MakeErrorResponse(http::status::not_found, "File Not Found"sv, version, "text/plain"sv));
            }
            return send(MakeErrorResponse(http::status::internal_server_error, "Internal Error"sv, version, "text/plain"sv));
        }

        auto size = file.size();
        auto mime_type = static_utils::GetMimeType(abs_path);

        if (method == http::verb::get || method == http::verb::head) {
            if (method == http::verb::head) {
                http::response<http::empty_body> res{http::status::ok, version};
                res.set(http::field::content_type, mime_type);
                res.content_length(size);
                return send(std::move(res));
            }

            http::response<http::file_body> res{
                std::piecewise_construct,
                std::make_tuple(std::move(file)),
                std::make_tuple(http::status::ok, version)};
            res.set(http::field::content_type, mime_type);
            res.content_length(size);
            return send(std::move(res));
        }

        return send(MakeErrorResponse(http::status::method_not_allowed, "Method Not Allowed"sv, version, "text/plain"sv));
    }

    app::Application& application_;
    fs::path static_content_path_;
    http_handler::ApiHandler api_handler_; 
};

} // namespace http_handler