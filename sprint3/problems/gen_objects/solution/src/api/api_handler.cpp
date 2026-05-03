#include "api_handler.h"
#include "json_serialization.h"
#include "endpoints.h"
#include <chrono>

namespace http_handler {

using namespace std::literals;

http::response<http::string_body> ApiHandler::HandleRequest(const http::request<http::string_body>& req) {
    auto target = req.target();
    auto method = req.method();

    if (target == std::string_view(api::Endpoints::GetMaps()) || target == (std::string(api::Endpoints::GetMaps()) + "/")) {
        if (method == http::verb::get || method == http::verb::head) return HandleGetMaps();
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"sv);
    }

    if (target.starts_with(api::Endpoints::GetMap())) {
        std::string map_id = std::string(target.substr(api::Endpoints::GetMap().size()));
        if (method == http::verb::get || method == http::verb::head) return HandleGetMapById(map_id);
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"sv);
    }

    if (target == api::Endpoints::JoinGame()) {
        if (method == http::verb::post) return HandleJoinGame(req.body());
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Only POST is allowed"}}, "POST"sv);
    }

    if (target == api::Endpoints::GetPlayers()) {
        if (method == http::verb::get || method == http::verb::head) 
            return HandleGetPlayers(req[http::field::authorization]);
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"sv);
    }

    if (target == api::Endpoints::GetState()) {
        if (method == http::verb::get || method == http::verb::head) 
            return HandleGetGameState(req[http::field::authorization]);
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"sv);
    }

    if (target == api::Endpoints::PlayerAction()) {
        if (method == http::verb::post) 
            return HandlePlayerAction(req[http::field::authorization], req.body());
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Only POST is allowed"}}, "POST"sv);
    }

    if (target == api::Endpoints::Tick()) {
        if (method == http::verb::post) return HandleTick(req.body());
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Only POST is allowed"}}, "POST"sv);
    }

    return MakeJsonResponse(http::status::bad_request, 
        json::value{{"code", "badRequest"}, {"message", "Unknown endpoint"}});
}

http::response<http::string_body> ApiHandler::HandleGetMaps() {
    const auto& maps = app_.GetGame().GetMaps();
    json::array result;
    for (const auto& map : maps) {
        result.push_back({{"id", *map->GetId()}, {"name", map->GetName()}});
    }
    return MakeJsonResponse(http::status::ok, result);
}

http::response<http::string_body> ApiHandler::HandleGetMapById(std::string_view id) {
    model::Map::Id map_id{std::string(id)};
    auto map = app_.GetGame().FindMap(map_id);
    if (!map) {
        return MakeJsonResponse(http::status::not_found, 
            json::value{{"code", "mapNotFound"}, {"message", "Map not found"}});
    }
    
    auto map_jv = json::value_from(*map);
    map_jv.as_object()["lootTypes"] = app_.GetExtraData().GetLootTypes(map_id);
    
    return MakeJsonResponse(http::status::ok, map_jv);
}

http::response<http::string_body> ApiHandler::HandleJoinGame(std::string_view body_str) {
    try {
        auto body = json::parse(boost::json::string_view{body_str.data(), body_str.size()});
        auto const& obj = body.as_object();
        
        if (!obj.contains("userName") || !obj.contains("mapId")) {
             return MakeJsonResponse(http::status::bad_request, 
                json::value{{"code", "invalidArgument"}, {"message", "Join game request parse error"}});
        }

        std::string name = std::string(obj.at("userName").as_string());
        std::string map_id_str = std::string(obj.at("mapId").as_string());

        if (name.empty()) {
            return MakeJsonResponse(http::status::bad_request, 
                json::value{{"code", "invalidArgument"}, {"message", "Invalid name"}});
        }

        model::Map::Id map_id{map_id_str};
        if (!app_.GetGame().FindMap(map_id)) {
            return MakeJsonResponse(http::status::not_found, 
                json::value{{"code", "mapNotFound"}, {"message", "Map not found"}});
        }

        auto [token, player_id] = app_.JoinGame(name, map_id);
        return MakeJsonResponse(http::status::ok, 
            json::value{{"authToken", *token}, {"playerId", *player_id}});
    } catch (...) {
        return MakeJsonResponse(http::status::bad_request, 
            json::value{{"code", "invalidArgument"}, {"message", "Join game request parse error"}});
    }
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

    json::object lost_obj;
    for (const auto& [loot_id, loot] : session->GetLostObjects()) {
        lost_obj[std::to_string(loot_id)] = json::value_from(loot);
    }

    json::object res;
    res["players"] = players_obj;
    res["lostObjects"] = lost_obj;

    return MakeJsonResponse(http::status::ok, res);
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

http::response<http::string_body> ApiHandler::HandlePlayerAction(std::string_view auth_header, std::string_view body_str) {
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

    try {
        auto body = json::parse(boost::json::string_view{body_str.data(), body_str.size()});
        std::string move_cmd = std::string(body.as_object().at("move").as_string());
        app_.MovePlayer(*token_opt, move_cmd);
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
        auto body = json::parse(boost::json::string_view{body_str.data(), body_str.size()});
        uint64_t delta = body.as_object().at("timeDelta").as_int64();
        app_.Tick(std::chrono::milliseconds(delta));
        return MakeJsonResponse(http::status::ok, json::object{});
    } catch (...) {
        return MakeJsonResponse(http::status::bad_request, 
            json::value{{"code", "invalidArgument"}, {"message", "Failed to parse tick"}});
    }
}

http::response<http::string_body> ApiHandler::MakeJsonResponse(http::status status, json::value body) {
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::cache_control, "no-cache");
    res.body() = json::serialize(body);
    res.prepare_payload();
    return res;
}

http::response<http::string_body> ApiHandler::MakeJsonResponseWithAllow(http::status status, json::value body, std::string_view allow) {
    auto res = MakeJsonResponse(status, std::move(body));
    res.set(http::field::allow, allow);
    return res;
}

std::optional<app::Token> ApiHandler::TryExtractToken(std::string_view auth_header) {
    if (auth_header.empty() || !auth_header.starts_with("Bearer ") || auth_header.size() <= 7) {
        return std::nullopt;
    }
    std::string token_str(auth_header.substr(7));
    if (token_str.size() != 32) return std::nullopt;
    return app::Token{std::move(token_str)};
}

} // namespace http_handler
