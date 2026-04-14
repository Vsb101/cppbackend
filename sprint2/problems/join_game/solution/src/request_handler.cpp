#include "request_handler.h"
#include "json_builder.h"
#include <boost/json.hpp>
#include <boost/algorithm/string.hpp>
#include <random>
#include <sstream>
#include <iomanip>

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

// === ВХОД В ИГРУ ===

void RequestHandler::HandleJoinGame(
    http::request<http::string_body>&& req,
    std::function<void(http::response<http::string_body>&&)>&& send) {
    // Проверка метода - только POST
    if (req.method() != http::verb::post) {
        auto response = MakeApiErrorResponse(req, ApiError::INVALID_METHOD);
        response.set(http::field::allow, "POST");
        send(std::move(response));
        return;
    }
    
    // Проверка Content-Type
    auto content_type_it = req.find(http::field::content_type);
    if (content_type_it == req.end() ||
        !boost::algorithm::iequals(content_type_it->value(), "application/json")) {
        send(MakeApiErrorResponse(req, ApiError::BAD_REQUEST));
        return;
    }
    
    // Парсинг JSON
    try {
        json::value json_body = json::parse(req.body());
        
        // Проверка наличия обязательных полей
        if (!json_body.is_object()) {
            send(MakeApiErrorResponse(req, ApiError::INVALID_ARGUMENT));
            return;
        }

        const json::object& obj = json_body.as_object();
        
        auto user_name_it = obj.find("userName");
        if (user_name_it == obj.end() || !user_name_it->value().is_string()) {
            send(MakeApiErrorResponse(req, ApiError::INVALID_ARGUMENT));
            return;
        }

        auto map_id_it = obj.find("mapId");
        if (map_id_it == obj.end() || !map_id_it->value().is_string()) {
            send(MakeApiErrorResponse(req, ApiError::INVALID_ARGUMENT));
            return;
        }

        std::string user_name = std::string(user_name_it->value().as_string());
        std::string map_id = std::string(map_id_it->value().as_string());
        
        // Проверка имени
        if (user_name.empty()) {
            send(MakeApiErrorResponse(req, ApiError::INVALID_ARGUMENT));
            return;
        }
        
        // Обработка входа в игру
        JoinGameResult result = JoinGame(user_name, map_id);
        
        switch (result.result) {
            case JoinResult::SUCCESS: {
                json::object response_body;
                response_body["playerId"] = static_cast<int>(result.player_id.GetValue());
                response_body["authToken"] = result.auth_token;
                
                std::string response_str = json::serialize(response_body);
                send(MakeResponse(http::status::ok, response_str, "application/json", req));
                break;
            }
            case JoinResult::MAP_NOT_FOUND: {
                send(MakeApiErrorResponse(req, ApiError::MAP_NOT_FOUND));
                break;
            }
            case JoinResult::INVALID_NAME: {
                // Создаем кастомный объект ошибки с нужным сообщением
                json::object error_body;
                error_body["code"] = "invalidArgument";
                error_body["message"] = "Invalid name";
                
                std::string error_str = json::serialize(error_body);
                auto response = MakeResponse(http::status::bad_request, error_str, "application/json", req);
                response.set(http::field::cache_control, "no-cache");
                send(std::move(response));
                break;
            }
            case JoinResult::PARSE_ERROR: {
                // Создаем кастомный объект ошибки с нужным сообщением
                json::object error_body;
                error_body["code"] = "invalidArgument";
                error_body["message"] = "Join game request parse error";
                
                std::string error_str = json::serialize(error_body);
                auto response = MakeResponse(http::status::bad_request, error_str, "application/json", req);
                response.set(http::field::cache_control, "no-cache");
                send(std::move(response));
                break;
            }
        }
        
    } catch (const std::exception&) {
        send(MakeApiErrorResponse(req, ApiError::INVALID_ARGUMENT));
    }
}

JoinGameResult RequestHandler::JoinGame(std::string user_name, std::string map_id) {
    // Поиск карты
    const model::Map* map = game_.FindMap(model::Map::Id{map_id});
    if (!map) {
        return {JoinResult::MAP_NOT_FOUND, model::Player::Id{}, model::Player::Token{}};
    }
    
    // Проверка имени
    if (user_name.empty()) {
        return {JoinResult::INVALID_NAME, model::Player::Id{}, model::Player::Token{}};
    }
    
    // Генерация токена
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::string token;
    token.reserve(32);
    for (int i = 0; i < 32; ++i) {
        token += "0123456789abcdef"[dis(gen)];
    }
    
    // Создание пса
    model::Dog* dog = game_.CreateDog(std::move(user_name), map);
    
    // Генерация ID игрока (просто количество игроков)
    model::Player::Id player_id{static_cast<int>(game_.GetPlayersCount())};
    
    // Создание игрока
    model::Player* player = game_.CreatePlayer(player_id, std::move(token), dog);
    
    // Создание сессии, если еще нет
    model::GameSession* session = game_.FindSession(map);
    if (!session) {
        session = &game_.CreateSession(map);
    }
    
    // Добавление игрока в сессию
    session->AddPlayer(*player);
    
    return {JoinResult::SUCCESS, player_id, player->GetToken()};
}

// === ПОЛУЧЕНИЕ СПИСКА ИГРОКОВ ===

void RequestHandler::HandleGetPlayers(
    http::request<http::string_body>&& req,
    std::function<void(http::response<http::string_body>&&)>&& send) {
    // Проверка метода - только GET, HEAD
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        auto response = MakeApiErrorResponse(req, ApiError::INVALID_METHOD);
        response.set(http::field::allow, "GET, HEAD");
        send(std::move(response));
        return;
    }
    
    // Проверка заголовка Authorization
    auto auth_it = req.find(http::field::authorization);
    if (auth_it == req.end()) {
        json::object error_body;
        error_body["code"] = "invalidToken";
        error_body["message"] = "Authorization header is missing";
        
        std::string error_str = json::serialize(error_body);
        auto response = MakeResponse(http::status::unauthorized, error_str, "application/json", req);
        response.set(http::field::cache_control, "no-cache");
        send(std::move(response));
        return;
    }
    
    // Извлечение токена
    std::string auth_header = std::string(auth_it->value());
    const std::string prefix = "Bearer ";
    std::string token;
    if (auth_header.size() > prefix.size() && auth_header.substr(0, prefix.size()) == prefix) {
        token = auth_header.substr(prefix.size());
        if (token.empty()) {
            json::object error_body;
            error_body["code"] = "invalidToken";
            error_body["message"] = "Invalid token";
            
            std::string error_str = json::serialize(error_body);
            auto response = MakeResponse(http::status::unauthorized, error_str, "application/json", req);
            response.set(http::field::cache_control, "no-cache");
            send(std::move(response));
            return;
        }
    } else {
        json::object error_body;
        error_body["code"] = "invalidToken";
        error_body["message"] = "Invalid token";

        std::string error_str = json::serialize(error_body);
        auto response = MakeResponse(http::status::unauthorized, error_str, "application/json", req);
        response.set(http::field::cache_control, "no-cache");
        send(std::move(response));
        return;
    }
    
    // Поиск игрока по токену
    model::Player* player = game_.FindPlayerByToken(token);
    if (!player) {
        json::object error_body;
        error_body["code"] = "unknownToken";
        error_body["message"] = "Player token has not been found";
        
        std::string error_str = json::serialize(error_body);
        auto response = MakeResponse(http::status::unauthorized, error_str, "application/json", req);
        response.set(http::field::cache_control, "no-cache");
        send(std::move(response));
        return;
    }
    
    // Получение сессии
    model::Dog* dog = player->GetDog();
    model::GameSession* session = game_.FindSession(dog->GetMap());
    if (!session) {
        // Это маловероятно, но на всякий случай
        send(MakeApiErrorResponse(req, ApiError::BAD_REQUEST));
        return;
    }
    
    // Формирование ответа
    json::object response_body;
    for (const model::Player& p : session->GetPlayers()) {
        json::object player_info;
        player_info["name"] = p.GetDog()->GetName();
        response_body[std::to_string(static_cast<int>(p.GetId().GetValue()))] = std::move(player_info);
    }
    
    std::string response_str = json::serialize(response_body);
    send(MakeResponse(http::status::ok, response_str, "application/json", req));
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
    response.set(http::field::cache_control, "no-cache");
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
    const http::request<Body>& req, std::string_view allowed_methods) {
    auto response = MakeApiErrorResponse(req, ApiError::METHOD_NOT_ALLOWED);
    response.set(http::field::allow, allowed_methods);
    return response;
}

template<typename Body>
http::response<http::string_body> RequestHandler::MakeMethodNotAllowedResponse(
    const http::request<Body>& req) {
    auto response = MakeApiErrorResponse(req, ApiError::METHOD_NOT_ALLOWED);
    response.set(http::field::allow, "GET, HEAD, POST");
    return response;
}

template http::response<http::string_body> RequestHandler::MakeMethodNotAllowedResponse(
    const http::request<http::string_body>&, std::string_view);

template http::response<http::string_body> RequestHandler::MakeMethodNotAllowedResponse(
    const http::request<http::string_body>&);

}  // namespace http_handler