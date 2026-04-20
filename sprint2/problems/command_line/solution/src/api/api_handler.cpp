#include "api_handler.h"
#include "json_serialization.h"
#include <chrono>

namespace http_handler {

using namespace std::literals;

http::response<http::string_body> ApiHandler::HandleRequest(const http::request<http::string_body>& req) {
    auto target = req.target();
    auto method = req.method();

    if (target == "/api/v1/maps"sv || target == "/api/v1/maps/"sv) {
        if (method == http::verb::get || method == http::verb::head) return HandleGetMaps();
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"sv);
    }

    if (target.starts_with("/api/v1/maps/"sv)) {
        std::string map_id = std::string(target.substr("/api/v1/maps/"sv.size()));
        if (method == http::verb::get || method == http::verb::head) return HandleGetMapById(map_id);
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"sv);
    }

    if (target == "/api/v1/game/join"sv) {
        if (method == http::verb::post) return HandleJoinGame(req.body());
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Only POST is allowed"}}, "POST"sv);
    }

    if (target == "/api/v1/game/players"sv) {
        if (method == http::verb::get || method == http::verb::head) 
            return HandleGetPlayers(req[http::field::authorization]);
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"sv);
    }

    if (target == "/api/v1/game/state"sv) {
        if (method == http::verb::get || method == http::verb::head) 
            return HandleGetGameState(req[http::field::authorization]);
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"sv);
    }

    if (target == "/api/v1/game/player/action"sv) {
        if (method == http::verb::post) 
            return HandlePlayerAction(req[http::field::authorization], req.body());
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Only POST is allowed"}}, "POST"sv);
    }

    if (target == "/api/v1/game/tick"sv) {
        if (method == http::verb::post) return HandleTick(req.body());
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Only POST is allowed"}}, "POST"sv);
    }

    return MakeJsonResponse(http::status::bad_request, 
        json::value{{"code", "badRequest"}, {"message", "Unknown endpoint"}});
}

http::response<http::string_body> ApiHandler::HandleGetMaps() {
    const auto& maps = app_.ListMaps();
    json::array result;
    for (const auto& map : maps) {
        result.push_back({{"id", *map->GetId()}, {"name", map->GetName()}});
    }
    return MakeJsonResponse(http::status::ok, result);
}

http::response<http::string_body> ApiHandler::HandleGetMapById(std::string_view id) {
    auto map = app_.FindMap(model::Map::Id{std::string(id)});
    if (!map) {
        return MakeJsonResponse(http::status::not_found, 
            json::value{{"code", "mapNotFound"}, {"message", "Map not found"}});
    }
    // ВАЖНО: используем value_from
    return MakeJsonResponse(http::status::ok, json::value_from(*map));
}

http::response<http::string_body> ApiHandler::HandleJoinGame(std::string_view body_str) {
    try {
        // Конвертируем std::string_view в boost::json::string_view
        auto body = json::parse(json::string_view{body_str.data(), body_str.size()});
        std::string name = std::string(body.as_object().at("userName").as_string());
        std::string map_id = std::string(body.at("mapId").as_string());

        if (name.empty()) {
            return MakeJsonResponse(http::status::bad_request, 
                json::value{{"code", "invalidArgument"}, {"message", "Invalid name"}});
        }

        auto [token, player_id] = app_.JoinGame(name, model::Map::Id{map_id});
        return MakeJsonResponse(http::status::ok, 
            json::value{{"authToken", *token}, {"playerId", *player_id}});
    } catch (...) {
        return MakeJsonResponse(http::status::bad_request, 
            json::value{{"code", "invalidArgument"}, {"message", "Join game request parse error"}});
    }
}

http::response<http::string_body> ApiHandler::HandleGetPlayers(std::string_view auth_header) {
    auto token_opt = TryExtractToken(auth_header);
    if (!token_opt) {
        return MakeJsonResponse(http::status::unauthorized, 
            json::value{{"code", "invalidToken"}, {"message", "Authorization header is required"}});
    }
    
    auto players = app_.GetPlayersInSession(*token_opt);
    if (players.empty()) {
        return MakeJsonResponse(http::status::unauthorized, 
            json::value{{"code", "unknownToken"}, {"message", "Player token has not been found"}});
    }
    
    json::object res;
    for (const auto& p : players) {
        res[std::to_string(*p->GetId())] = json::value{{"name", p->GetName()}};
    }
    return MakeJsonResponse(http::status::ok, res);
}

http::response<http::string_body> ApiHandler::HandleGetGameState(std::string_view auth_header) {
    auto token_opt = TryExtractToken(auth_header);
    if (!token_opt) {
        return MakeJsonResponse(http::status::unauthorized, 
            json::value{{"code", "invalidToken"}, {"message", "Authorization header is required"}});
    }

    auto player = app_.FindPlayerByToken(*token_opt);
    if (!player) {
        return MakeJsonResponse(http::status::unauthorized, 
            json::value{{"code", "unknownToken"}, {"message", "Player token has not been found"}});
    }

    auto session = player->GetSession();
    json::object players_obj;
    for (const auto& dog_ptr : session->GetDogs()) {
        players_obj[std::to_string(*dog_ptr->GetId())] = json::value_from(*dog_ptr);
    }
    return MakeJsonResponse(http::status::ok, json::value{{"players", players_obj}});
}

http::response<http::string_body> ApiHandler::HandlePlayerAction(std::string_view auth_header, std::string_view body_str) {
    auto token_opt = TryExtractToken(auth_header);
    if (!token_opt) {
        return MakeJsonResponse(http::status::unauthorized, 
            json::value{{"code", "invalidToken"}, {"message", "Authorization header is required"}});
    }

    try {
        auto body = json::parse(json::string_view{body_str.data(), body_str.size()});
        std::string move = std::string(body.as_object().at("move").as_string());
        app_.MovePlayer(*token_opt, move);
        return MakeJsonResponse(http::status::ok, json::object{});
    } catch (...) {
        return MakeJsonResponse(http::status::bad_request, 
            json::value{{"code", "invalidArgument"}, {"message", "Failed to parse action"}});
    }
}

http::response<http::string_body> ApiHandler::HandleTick(std::string_view body_str) {
    if (app_.IsTickAuto()) {
        return MakeJsonResponse(http::status::bad_request, 
            json::value{{"code", "badRequest"}, {"message", "Invalid endpoint"}});
    }

    try {
        auto body = json::parse(json::string_view{body_str.data(), body_str.size()});
        auto delta_ms = body.as_object().at("timeDelta").as_int64();
        app_.Tick(std::chrono::milliseconds(delta_ms));
        return MakeJsonResponse(http::status::ok, json::object{});
    } catch (...) {
        return MakeJsonResponse(http::status::bad_request, 
            json::value{{"code", "invalidArgument"}, {"message", "Failed to parse tick request"}});
    }
}

http::response<http::string_body> ApiHandler::MakeJsonResponse(http::status status, json::value body) {
    http::response<http::string_body> response(status, 11);
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.body() = json::serialize(body);
    response.prepare_payload();
    return response;
}

http::response<http::string_body> ApiHandler::MakeJsonResponseWithAllow(http::status status, json::value body, std::string_view allow) {
    auto response = MakeJsonResponse(status, std::move(body));
    response.set(http::field::allow, allow);
    return response;
}

std::optional<app::Token> ApiHandler::TryExtractToken(std::string_view auth_header) {
    if (auth_header.empty() || !auth_header.starts_with("Bearer "sv)) {
        return std::nullopt;
    }
    std::string token_str{auth_header.substr(7)};
    if (token_str.size() != 32) return std::nullopt;
    return app::Token{std::move(token_str)};
}

} // namespace http_handler
