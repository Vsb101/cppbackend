#include "api_handler.h"

#include <boost/json.hpp>
#include <boost/utility/string_view.hpp>

#include <algorithm>
#include <cctype>
#include <sstream>

namespace api_handler {

namespace json = boost::json;
namespace beast = boost::beast;

ApiHandler::ApiHandler(model::Game& game, game_model::GameSessions& sessions)
    : game_(game)
    , sessions_(sessions) {
}

std::optional<ApiHandler::StringResponse> ApiHandler::HandleRequest(const StringRequest& req) {
    beast::string_view target = req.target();
    
    // Убираем начальный '/'
    if (!target.empty() && target[0] == '/') {
        target = target.substr(1);
    }

    // POST /api/v1/game/join
    if (target == "api/v1/game/join" || target.rfind("api/v1/game/join", 0) == 0) {
        return HandleJoinGame(req);
    }

    // GET /api/v1/game/players
    if (target == "api/v1/game/players" || target.rfind("api/v1/game/players", 0) == 0) {
        return HandleGetPlayers(req);
    }

    // Не наш обработчик
    return std::nullopt;
}

std::optional<ApiHandler::StringResponse> ApiHandler::HandleJoinGame(const StringRequest& req) {
    // Проверяем метод
    if (req.method() != http::verb::post) {
        return MakeMethodNotAllowedResponse(req, "POST");
    }

    return HandleJoinGamePost(req);
}

ApiHandler::StringResponse ApiHandler::HandleJoinGamePost(const StringRequest& req) {
    // Проверяем Content-Type
    auto content_type = req.find(http::field::content_type);
    if (content_type == req.end() || content_type->value() != "application/json") {
        return MakeJoinErrorResponse(http::status::bad_request, "invalidArgument",
                                     "Invalid Content-Type", req);
    }

    // Парсим JSON
    json::error_code ec;
    json::value value = json::parse(req.body(), ec);
    if (ec) {
        return MakeJoinErrorResponse(http::status::bad_request, "invalidArgument",
                                     "Join game request parse error", req);
    }

    if (!value.is_object()) {
        return MakeJoinErrorResponse(http::status::bad_request, "invalidArgument",
                                     "Join game request parse error", req);
    }

    const json::object& obj = value.as_object();

    // Получаем userName
    auto name_it = obj.find("userName");
    if (name_it == obj.end() || !name_it->value().is_string()) {
        return MakeJoinErrorResponse(http::status::bad_request, "invalidArgument",
                                     "Join game request parse error", req);
    }
    std::string user_name = name_it->value().as_string().c_str();

    // Проверяем, что имя не пустое
    if (user_name.empty()) {
        return MakeJoinErrorResponse(http::status::bad_request, "invalidArgument",
                                     "Invalid name", req);
    }

    // Получаем mapId
    auto map_id_it = obj.find("mapId");
    if (map_id_it == obj.end() || !map_id_it->value().is_string()) {
        return MakeJoinErrorResponse(http::status::bad_request, "invalidArgument",
                                     "Join game request parse error", req);
    }
    std::string map_id = map_id_it->value().as_string().c_str();

    // Ищем карту
    const model::Map* map = game_.FindMap(model::Map::Id{map_id});
    if (!map) {
        return MakeJoinErrorResponse(http::status::not_found, "mapNotFound",
                                     "Map not found", req);
    }

    // Создаём нового игрока
    game_model::PlayerId player_id = static_cast<game_model::PlayerId>(sessions_.GetOrCreateSession(map)->GetPlayers().size());
    game_model::AuthToken auth_token = sessions_.GetTokenGenerator().GenerateToken();

    game_model::Player player{player_id, std::move(user_name), auth_token, map};

    // Добавляем игрока в сеанс
    auto* session = sessions_.GetOrCreateSession(map);
    session->AddPlayer(player);
    sessions_.GetTokenGenerator().AddToken(player.token, &session->GetPlayers().back());

    // Формируем ответ
    json::object response_obj;
    response_obj["authToken"] = *player.token;
    response_obj["playerId"] = player.id;

    std::string body = json::serialize(response_obj);

    auto response = MakeResponse(http::status::ok, body, "application/json", req);
    response.set(http::field::cache_control, "no-cache");
    return response;
}

ApiHandler::StringResponse ApiHandler::MakeJoinErrorResponse(http::status status, 
                                                              const std::string& code,
                                                              const std::string& message,
                                                              const StringRequest& req) {
    json::object error_obj;
    error_obj["code"] = code;
    error_obj["message"] = message;

    std::string body = json::serialize(error_obj);

    auto response = MakeResponse(status, body, "application/json", req);
    response.set(http::field::cache_control, "no-cache");
    return response;
}

std::optional<ApiHandler::StringResponse> ApiHandler::HandleGetPlayers(const StringRequest& req) {
    // Проверяем метод
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        return MakeMethodNotAllowedResponse(req, "GET, HEAD");
    }

    // Проверяем заголовок Authorization
    auto auth_it = req.find(http::field::authorization);
    if (auth_it == req.end()) {
        return MakeUnauthorizedResponse("invalidToken", "Authorization header is missing", req);
    }

    beast::string_view auth_value = auth_it->value();
    beast::string_view token_str = ExtractBearerToken(auth_value);

    if (token_str.empty()) {
        return MakeUnauthorizedResponse("invalidToken", "Invalid authorization header format", req);
    }

    // Ищем игрока по токену
    game_model::AuthToken auth_token{std::string(token_str)};
    const game_model::Player* player = sessions_.FindPlayerByToken(auth_token);

    if (!player) {
        return MakeUnauthorizedResponse("unknownToken", "Player token has not been found", req);
    }

    // Находим сеанс игрока
    auto* session = sessions_.FindSession(player->map);
    if (!session) {
        return MakeUnauthorizedResponse("unknownToken", "Player token has not been found", req);
    }

    return MakePlayersResponse(*session, req);
}

ApiHandler::StringResponse ApiHandler::MakePlayersResponse(const game_model::GameSession& session, 
                                                            const StringRequest& req) {
    json::object players_obj;

    for (const auto& player : session.GetPlayers()) {
        json::object player_info;
        player_info["name"] = player.name;
        players_obj[std::to_string(player.id)] = std::move(player_info);
    }

    std::string body = json::serialize(players_obj);

    auto response = MakeResponse(http::status::ok, body, "application/json", req);
    response.set(http::field::cache_control, "no-cache");
    return response;
}

ApiHandler::StringResponse ApiHandler::MakeUnauthorizedResponse(const std::string& code, 
                                                                 const std::string& message,
                                                                 const StringRequest& req) {
    json::object error_obj;
    error_obj["code"] = code;
    error_obj["message"] = message;

    std::string body = json::serialize(error_obj);

    auto response = MakeResponse(http::status::unauthorized, body, "application/json", req);
    response.set(http::field::cache_control, "no-cache");
    return response;
}

ApiHandler::StringResponse ApiHandler::MakeMethodNotAllowedResponse(const StringRequest& req, 
                                                                     beast::string_view allowed_methods) {
    json::object error_obj;
    error_obj["code"] = "invalidMethod";
    error_obj["message"] = "Invalid method";

    std::string body = json::serialize(error_obj);

    auto response = MakeResponse(http::status::method_not_allowed, body, "application/json", req);
    response.set(http::field::allow, std::string(allowed_methods));
    response.set(http::field::cache_control, "no-cache");
    return response;
}

beast::string_view ApiHandler::ExtractBearerToken(beast::string_view auth_header) {
    // Ожидаем формат "Bearer <token>"
    beast::string_view prefix = "Bearer ";
    
    if (auth_header.size() <= prefix.size()) {
        return {};
    }

    if (auth_header.substr(0, prefix.size()) != prefix) {
        return {};
    }

    return auth_header.substr(prefix.size());
}

ApiHandler::StringResponse ApiHandler::MakeResponse(http::status status, 
                                                     beast::string_view body,
                                                     beast::string_view content_type,
                                                     const StringRequest& req) {
    StringResponse response;
    response.result(status);
    response.set(http::field::content_type, std::string(content_type));
    response.content_length(body.size());
    response.keep_alive(req.keep_alive());

    // Для HEAD запросов не добавляем тело
    if (req.method() != http::verb::head) {
        response.body() = std::string(body);
    }

    return response;
}

}  // namespace api_handler
