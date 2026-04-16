#include "api_handler.h"
#include "json_builder.h"
#include <boost/json.hpp>
#include <boost/algorithm/string.hpp>

namespace http_handler {

namespace js = boost::json;

ApiHandler::ApiHandler(app::Application& app)
    : app_(app) {
}

template<typename Body>
http::response<http::string_body> ApiHandler::MakeResponse(
    http::status status,
    std::string_view body,
    std::string_view content_type,
    const http::request<Body>& req) const {
    http::response<http::string_body> response{status, req.version()};
    response.result(status);
    response.set(http::field::content_type, std::string(content_type));
    response.keep_alive(req.keep_alive());
    if (req.method() != http::verb::head) {
        response.body() = std::string(body);
    }
    response.content_length(body.size());
    return response;
}

template<typename Body>
http::response<http::string_body> ApiHandler::MakeError(
    const http::request<Body>& req,
    std::string_view code,
    std::string_view message,
    http::status status) const {
    js::object error_body;
    error_body["code"] = std::string(code);
    error_body["message"] = std::string(message);
    std::string json = js::serialize(error_body);
    return MakeResponse(status, json, "application/json", req);
}

void ApiHandler::HandleJoinGame(http::request<http::string_body>&& req, SendCallback send) {
    if (req.method() != http::verb::post) {
        auto response = MakeError(req, "invalidMethod", "Only POST method is expected", http::status::method_not_allowed);
        response.set(http::field::allow, "POST");
        send(std::move(response));
        return;
    }

    // Проверка Content-Type
    auto content_type_it = req.find(http::field::content_type);
    if (content_type_it == req.end() ||
        !boost::algorithm::iequals(content_type_it->value(), "application/json")) {
        send(std::move(MakeError(req, "badRequest", "Content-Type must be application/json", http::status::bad_request)));
        return;
    }

    // Парсинг JSON
    js::value json_body;
    try {
        json_body = js::parse(req.body());
    } catch (...) {
        send(std::move(MakeError(req, "badRequest", "Invalid JSON", http::status::bad_request)));
        return;
    }

    // Извлечение параметров
    std::string user_name;
    std::string map_id;
    try {
        const auto& obj = json_body.as_object();
        user_name = std::string(obj.at("userName").as_string().data(), obj.at("userName").as_string().size());
        map_id = std::string(obj.at("mapId").as_string().data(), obj.at("mapId").as_string().size());
    } catch (...) {
        send(std::move(MakeError(req, "invalidArgument", "Missing required fields", http::status::bad_request)));
        return;
    }

    // Вызов сценария использования
    auto result = app_.JoinGame(std::move(user_name), std::move(map_id));

    if (result.success) {
        js::object response_body;
        response_body["playerId"] = static_cast<int>(result.player_id.GetValue());
        response_body["authToken"] = result.auth_token;
        std::string json = js::serialize(response_body);
        send(std::move(MakeResponse(http::status::ok, json, "application/json", req)));
    } else {
        http::status status = http::status::bad_request;
        if (result.error_code == "mapNotFound") {
            status = http::status::not_found;
        }
        send(std::move(MakeError(req, result.error_code, result.message, status)));
    }
}

void ApiHandler::HandleGetPlayers(http::request<http::string_body>&& req, SendCallback send) {
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        auto response = MakeError(req, "invalidMethod", "Invalid method", http::status::method_not_allowed);
        response.set(http::field::allow, "GET, HEAD");
        send(std::move(response));
        return;
    }

    // Проверка токена
    auto auth_header = req.find(http::field::authorization);
    if (auth_header == req.end()) {
        send(std::move(MakeError(req, "invalidToken", "Authorization header is required", http::status::unauthorized)));
        return;
    }

    std::string auth_value{auth_header->value()};
    if (!auth_value.starts_with("Bearer ")) {
        send(std::move(MakeError(req, "invalidToken", "Invalid authorization format", http::status::unauthorized)));
        return;
    }

    std::string token = auth_value.substr(7);
    if (token.empty()) {
        send(std::move(MakeError(req, "invalidToken", "Token is required", http::status::unauthorized)));
        return;
    }

    const app::Player* player = app_.FindPlayerByToken(token);
    if (!player) {
        send(std::move(MakeError(req, "unknownToken", "Unknown token", http::status::unauthorized)));
        return;
    }

    // Получение списка игроков
    auto players = app_.GetPlayers(token);
    js::object response_body;
    response_body["players"] = js::array{};
    std::string json = js::serialize(response_body);
    send(std::move(MakeResponse(http::status::ok, json, "application/json", req)));
}

void ApiHandler::HandleMapsList(http::request<http::string_body>&& req, SendCallback send) {
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        auto response = MakeError(req, "invalidMethod", "Invalid method", http::status::method_not_allowed);
        response.set(http::field::allow, "GET, HEAD");
        send(std::move(response));
        return;
    }

    std::string json = json_builder::BuildMapsList(app_.GetGame());
    auto response = MakeResponse(http::status::ok, json, "application/json", req);
    send(std::move(response));
}

void ApiHandler::HandleMapById(http::request<http::string_body>&& req, SendCallback send, std::string_view map_id) {
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        auto response = MakeError(req, "invalidMethod", "Invalid method", http::status::method_not_allowed);
        response.set(http::field::allow, "GET, HEAD");
        send(std::move(response));
        return;
    }

    const model::Map* map = app_.GetGame().FindMap(model::Map::Id{std::string(map_id)});
    if (!map) {
        send(MakeError(req, "mapNotFound", "Map not found", http::status::not_found));
        return;
    }

    std::string json = json_builder::BuildMap(*map);
    auto response = MakeResponse(http::status::ok, json, "application/json", req);
    send(std::move(response));
}

// Явная инстанциация шаблонов
template http::response<http::string_body> ApiHandler::MakeResponse(
    http::status, std::string_view, std::string_view, const http::request<http::string_body>&) const;

template http::response<http::string_body> ApiHandler::MakeError(
    const http::request<http::string_body>&, std::string_view, std::string_view, http::status) const;

}  // namespace http_handler
