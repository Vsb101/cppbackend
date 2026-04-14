#include "request_handler.h"
#include "json_builder.h"

#include <boost/asio/dispatch.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_map>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace sys = boost::system;
namespace json = boost::json;
using namespace std::literals;

// === КОНСТРУКТОР ===

RequestHandler::RequestHandler(model::Game& game, const std::filesystem::path& static_files_root)
    : game_(game)
    , static_files_root_(static_files_root)
{
}

// === КЭШИРОВАНИЕ ===

void RequestHandler::InvalidateCache() {
    map_json_cache_.clear();
    maps_list_cache_valid_ = false;
}

// === API ОБРАБОТЧИКИ ===

void RequestHandler::HandleApiMaps(
    http::request<http::string_body>&& req,
    std::function<void(http::response<http::string_body>&&)>&& send,
    std::string_view path_suffix) {
    if (path_suffix.empty() || path_suffix == "/") {
        send(HandleMapsList(req));
    }
    else if (path_suffix[0] == '/') {
        send(HandleMapById(req, path_suffix.substr(1)));
    }
    else {
        send(MakeApiErrorResponse(req, ApiError::BAD_REQUEST));
    }
}

http::response<http::string_body> RequestHandler::HandleMapsList(
    const http::request<http::string_body>& req) {
    if (!maps_list_cache_valid_) {
        maps_list_json_ = json_builder::BuildMapsList(game_);
        maps_list_cache_valid_ = true;
    }
    return MakeResponse(http::status::ok, maps_list_json_, "application/json", req);
}

http::response<http::string_body> RequestHandler::HandleMapById(
    const http::request<http::string_body>& req,
    std::string_view map_id) {
    const model::Map* map = game_.FindMap(model::Map::Id{std::string(map_id)});
    if (!map) {
        return MakeApiErrorResponse(req, ApiError::MAP_NOT_FOUND);
    }

    std::string map_id_str(map_id);
    auto it = map_json_cache_.find(map_id_str);
    if (it == map_json_cache_.end()) {
        it = map_json_cache_.emplace(map_id_str, json_builder::BuildMap(*map)).first;
    }

    return MakeResponse(http::status::ok, it->second, "application/json", req);
}

// === СТАТИЧЕСКИЕ ФАЙЛЫ ===

void RequestHandler::HandleStaticFile(
    const http::request<http::string_body>& req,
    std::function<void(http::response<http::string_body>&&)>&& send,
    std::string_view target) {
    
    std::string decoded_target = UrlDecode(target);
    std::filesystem::path requested_path(decoded_target);

    if (decoded_target.empty() || decoded_target.back() == '/' || !requested_path.has_filename()) {
        requested_path /= "index.html";
    }

    std::filesystem::path full_path = static_files_root_ / requested_path;

    try {
        full_path = std::filesystem::weakly_canonical(full_path);

        if (!IsSubPath(full_path)) {
            send(MakeApiErrorResponse(req, ApiError::BAD_REQUEST));
            return;
        }

        if (std::filesystem::is_directory(full_path)) {
            full_path /= "index.html";
        }

        if (!std::filesystem::is_regular_file(full_path)) {
            send(MakeApiErrorResponse(req, ApiError::FILE_NOT_FOUND));
            return;
        }

        if (auto content = ReadFileContent(full_path)) {
            send(MakeFileResponse(*content, full_path, req));
        } else {
            send(MakeApiErrorResponse(req, ApiError::FILE_NOT_FOUND));
        }

    } catch (const std::filesystem::filesystem_error&) {
        send(MakeApiErrorResponse(req, ApiError::FILE_NOT_FOUND));
    }
}

std::string RequestHandler::GetMimeType(std::string_view path) {
    std::string extension;
    auto pos = path.find_last_of('.');
    if (pos != std::string_view::npos) {
        extension = std::string(path.substr(pos));
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char c) { return std::tolower(c); });
    }

    static const std::unordered_map<std::string_view, std::string_view> mime_types = {
        {".htm", "text/html"},       {".html", "text/html"},
        {".css", "text/css"},        {".txt", "text/plain"},
        {".js", "text/javascript"},  {".json", "application/json"},
        {".xml", "application/xml"}, {".png", "image/png"},
        {".jpg", "image/jpeg"},      {".jpe", "image/jpeg"},
        {".jpeg", "image/jpeg"},     {".gif", "image/gif"},
        {".bmp", "image/bmp"},       {".ico", "image/vnd.microsoft.icon"},
        {".tiff", "image/tiff"},     {".tif", "image/tiff"},
        {".svg", "image/svg+xml"},   {".svgz", "image/svg+xml"},
        {".mp3", "audio/mpeg"}
    };

    auto it = mime_types.find(extension);
    if (it != mime_types.end()) {
        return std::string(it->second);
    }
    return "application/octet-stream";
}

std::string RequestHandler::UrlDecode(std::string_view str) {
    std::string decoded;
    decoded.reserve(str.size());

    auto from_hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];
        if (c == '+') {
            decoded += ' ';
        } else if (c == '%' && i + 2 < str.size()) {
            int hi = from_hex(str[i + 1]);
            int lo = from_hex(str[i + 2]);
            if (hi != -1 && lo != -1) {
                decoded += static_cast<char>((hi << 4) | lo);
                i += 2;
                continue;
            }
            decoded += c;
        } else {
            decoded += c;
        }
    }

    return decoded;
}

bool RequestHandler::IsSubPath(const std::filesystem::path& path) const {
    std::error_code ec;
    std::filesystem::path full = std::filesystem::weakly_canonical(path, ec);
    if (ec) return false;

    std::filesystem::path root = std::filesystem::weakly_canonical(static_files_root_, ec);
    if (ec) return false;

    std::string full_str = full.string();
    std::string root_str = root.string();

    return full_str.size() >= root_str.size() &&
           full_str.compare(0, root_str.size(), root_str) == 0;
}

std::optional<std::string> RequestHandler::ReadFileContent(const std::filesystem::path& path) const {
    std::ifstream file(path.string(), std::ios::binary);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

http::response<http::string_body> RequestHandler::MakeFileResponse(
    const std::string& content,
    const std::filesystem::path& path,
    const http::request<http::string_body>& req) {
    return MakeResponse(http::status::ok, content, GetMimeType(path.string()), req);
}

// === УНИВЕРСАЛЬНЫЕ ОТВЕТЫ ===

template<typename Body>
http::response<http::string_body> RequestHandler::MakeResponse(
    http::status status,
    std::string_view body,
    std::string_view content_type,
    const http::request<Body>& req) {
    http::response<http::string_body> response;
    response.result(status);
    response.set(http::field::content_type, content_type);
    response.content_length(body.size());
    response.keep_alive(req.keep_alive());

    if (req.method() != http::verb::head) {
        response.body() = std::string(body);
    }

    return response;
}

template http::response<http::string_body> RequestHandler::MakeResponse(
    http::status, std::string_view, std::string_view, const http::request<http::string_body>&);

// === ОБРАБОТКА ОШИБОК ===

template<typename Body>
http::response<http::string_body> RequestHandler::MakeApiErrorResponse(
    const http::request<Body>& req, ApiError error) {
    using namespace detail;
    const auto& info = GetErrorInfo(error);

    json::object body;
    body["code"] = std::string(info.code);
    body["message"] = std::string(info.message);

    return MakeResponse(info.status, json::serialize(body), "application/json", req);
}

template http::response<http::string_body> RequestHandler::MakeApiErrorResponse(
    const http::request<http::string_body>&, ApiError);

template<typename Body>
http::response<http::string_body> RequestHandler::MakeMethodNotAllowedResponse(
    const http::request<Body>& req) {
    auto response = MakeApiErrorResponse(req, ApiError::METHOD_NOT_ALLOWED);
    response.set(http::field::allow, "GET, HEAD");
    return response;
}

template http::response<http::string_body> RequestHandler::MakeMethodNotAllowedResponse(
    const http::request<http::string_body>&);

}  // namespace http_handler