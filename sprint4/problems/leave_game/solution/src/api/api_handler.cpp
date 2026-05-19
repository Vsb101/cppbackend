#include "api_handler.h"
#include "json_serialization.h"
#include "endpoints.h"
#include <chrono>
#include <charconv>

namespace http_handler {

using namespace std::literals;

http::response<http::string_body> ApiHandler::HandleRequest(const http::request<http::string_body>& req) {
    auto target = req.target();
    auto method = req.method();

    static constexpr std::string_view MAPS_PATH = api::Endpoints::GetMaps();

    // 1. Список всех карт
    if (target == MAPS_PATH || (target.length() == MAPS_PATH.length() + 1 && target.compare(0, MAPS_PATH.length(), MAPS_PATH) == 0 && target.back() == '/')) {
        if (method == http::verb::get || method == http::verb::head) return HandleGetMaps();
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"sv);
    }

    // 2. Получение конкретной карты
    if (target.starts_with(api::Endpoints::GetMap())) {
        std::string_view map_id_part = target.substr(api::Endpoints::GetMap().size());
        if (!map_id_part.empty() && map_id_part[0] == '/') map_id_part.remove_prefix(1);
        
        if (method == http::verb::get || method == http::verb::head) return HandleGetMapById(map_id_part);
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"sv);
    }

    // 3. Вход в игру
    if (target == api::Endpoints::JoinGame()) {
        if (method == http::verb::post) return HandleJoinGame(req.body());
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Only POST is allowed"}}, "POST"sv);
    }

    // 4. Список игроков
    if (target == api::Endpoints::GetPlayers()) {
        if (method == http::verb::get || method == http::verb::head) 
            return HandleGetPlayers(req[http::field::authorization]);
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"sv);
    }

    // 5. Состояние игры
    if (target == api::Endpoints::GetState()) {
        if (method == http::verb::get || method == http::verb::head) 
            return HandleGetGameState(req[http::field::authorization]);
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"sv);
    }

    // 6. Действие игрока
    if (target == api::Endpoints::PlayerAction()) {
        if (method == http::verb::post) 
            return HandlePlayerAction(req[http::field::authorization], req.body());
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Only POST is allowed"}}, "POST"sv);
    }

    // 7. Ручной тик времени
    if (target == api::Endpoints::Tick()) {
        if (method == http::verb::post) return HandleTick(req.body());
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Only POST is allowed"}}, "POST"sv);
    }

    // 8. Таблица рекордов
    if (target.starts_with(api::Endpoints::GetRecords())) {
        if (method == http::verb::get || method == http::verb::head) return HandleGetRecords(target);
        return MakeJsonResponseWithAllow(http::status::method_not_allowed, 
            json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"sv);
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
        auto body = json::parse({body_str.data(), body_str.size()});
        auto const& obj = body.as_object();
        
        auto name_it = obj.find("userName");
        auto map_id_it = obj.find("mapId");
        if (name_it == obj.end() || map_id_it == obj.end()) {
             throw std::invalid_argument("missing fields");
        }

        std::string_view name = json::value_to<std::string_view>(name_it->value());
        std::string_view map_id_str = json::value_to<std::string_view>(map_id_it->value());

        if (name.empty()) {
            return MakeJsonResponse(http::status::bad_request, 
                json::value{{"code", "invalidArgument"}, {"message", "Invalid name"}});
        }

        model::Map::Id map_id{std::string(map_id_str)};
        if (!app_.GetGame().FindMap(map_id)) {
            return MakeJsonResponse(http::status::not_found, 
                json::value{{"code", "mapNotFound"}, {"message", "Map not found"}});
        }

        auto [token, player_id] = app_.JoinGame(std::string(name), map_id);
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
        char buf[20];
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), *p->GetId());
        res[std::string(buf, static_cast<size_t>(ptr - buf))] = json::value{{"name", p->GetName()}};
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
        char buf[20];
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), *dog_ptr->GetId());
        players_obj[std::string(buf, static_cast<size_t>(ptr - buf))] = json::value_from(*dog_ptr);
    }

    json::object lost_obj;
    for (const auto& [loot_id, loot] : session->GetLostObjects()) {
        char buf[20];
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), loot_id);
        lost_obj[std::string(buf, static_cast<size_t>(ptr - buf))] = json::value_from(loot);
    }

    json::object res;
    res["players"] = players_obj;
    res["lostObjects"] = lost_obj;

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
        auto body = json::parse({body_str.data(), body_str.size()});
        auto move_it = body.as_object().find("move");
        if (!body.is_object() || move_it == body.as_object().end()) throw std::runtime_error("bad move");
        std::string_view move_cmd = json::value_to<std::string_view>(move_it->value());
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
        auto body = json::parse({body_str.data(), body_str.size()});
        auto delta_it = body.as_object().find("timeDelta");
        if (delta_it == body.as_object().end()) throw std::runtime_error("not a number");
        auto delta_val = delta_it->value();
        if (!delta_val.is_number()) throw std::runtime_error("not a number");
        
        uint64_t delta = delta_val.as_int64();
        app_.Tick(std::chrono::milliseconds(delta));
        return MakeJsonResponse(http::status::ok, json::object{});
    } catch (...) {
        return MakeJsonResponse(http::status::bad_request, 
            json::value{{"code", "invalidArgument"}, {"message", "Failed to parse tick"}});
    }
}

http::response<http::string_body> ApiHandler::HandleGetRecords(std::string_view target) {
    int start = 0;
    int max_items = 100;

    if (size_t query_pos = target.find('?'); query_pos != std::string_view::npos) {
        std::string_view query = target.substr(query_pos + 1);
        
        while (!query.empty()) {
            size_t pair_end = query.find('&');
            std::string_view pair = (pair_end == std::string_view::npos) ? query : query.substr(0, pair_end);
            
            if (size_t eq_pos = pair.find('='); eq_pos != std::string_view::npos) {
                std::string_view key = pair.substr(0, eq_pos);
                std::string_view val = pair.substr(eq_pos + 1);
                
                if (key == "start") {
                    auto [ptr, ec] = std::from_chars(val.data(), val.data() + val.size(), start);
                    if (ec != std::errc{} || start < 0) {
                        return MakeJsonResponse(http::status::bad_request, 
                            json::value{{"code", "badRequest"}, {"message", "Invalid start parameter"}});
                    }
                } else if (key == "maxItems") {
                    auto [ptr, ec] = std::from_chars(val.data(), val.data() + val.size(), max_items);
                    if (ec != std::errc{} || max_items < 0 || max_items > 100) {
                        return MakeJsonResponse(http::status::bad_request, 
                            json::value{{"code", "badRequest"}, {"message", "Invalid maxItems parameter"}});
                    }
                }
            }
            query = (pair_end == std::string_view::npos) ? ""sv : query.substr(pair_end + 1);
        }
    }

    auto records = app_.GetRecords(start, max_items);

    json::array res_arr;
    for (const auto& item : records) {
        res_arr.push_back({
            {"name", item.name},
            {"score", item.score},
            {"playTime", item.play_time}
        });
    }

    return MakeJsonResponse(http::status::ok, res_arr);
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
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::cache_control, "no-cache");
    res.set(http::field::allow, allow);
    res.body() = json::serialize(body);
    res.prepare_payload();
    return res;
}

std::optional<app::Token> ApiHandler::TryExtractToken(std::string_view auth_header) {
    constexpr std::string_view AUTH_PREFIX = "Bearer ";
    constexpr size_t EXPECTED_TOKEN_LENGTH = 32;

    if (auth_header.empty() || 
        !auth_header.starts_with(AUTH_PREFIX) || 
        auth_header.size() <= AUTH_PREFIX.size()) {
        return std::nullopt;
    }

    std::string token_str(auth_header.substr(AUTH_PREFIX.size()));
    if (token_str.size() != EXPECTED_TOKEN_LENGTH) {
        return std::nullopt;
    }

    return app::Token{std::move(token_str)};
}

} // namespace http_handler
