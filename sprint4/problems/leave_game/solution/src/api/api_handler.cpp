#include "api_handler.h"
#include "json_serialization.h"
#include "endpoints.h"
#include <chrono>
#include <charconv>
#include <string>
#include <algorithm>

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

    // 8. Таблица рекордов (с пагинацией и поддержкой HEAD)
    if (target.starts_with(api::Endpoints::GetRecords())) {
        if (method == http::verb::get || method == http::verb::head) {
            std::string_view full_target = target;
            std::string_view query = ""sv;
            
            // Выделяем строго query-параметры, отсекая путь
            if (auto pos = full_target.find('?'); pos != std::string_view::npos) {
                query = full_target.substr(pos + 1);
            }
            
            auto response = HandleGetRecords(query);
            
            // Спецификация HTTP: для HEAD конструируем те же заголовки, но очищаем тело
            if (method == http::verb::head) {
                response.body().clear();
                response.prepare_payload();
            }
            return response;
        }
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
        res[{buf, static_cast<size_t>(ptr - buf)}] = json::value{{"name", p->GetName()}};
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
        players_obj[{buf, static_cast<size_t>(ptr - buf)}] = json::value_from(*dog_ptr);
    }

    json::object lost_obj;
    for (const auto& [loot_id, loot] : session->GetLostObjects()) {
        char buf[20];
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), loot_id);
        lost_obj[{buf, static_cast<size_t>(ptr - buf)}] = json::value_from(loot);
    }

    json::object state_res;
    state_res["players"] = std::move(players_obj);
    state_res["lostObjects"] = std::move(lost_obj);

    return MakeJsonResponse(http::status::ok, state_res);
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
        auto const& obj = body.as_object();
        if (auto it = obj.find("move"); it != obj.end()) {
            std::string_view move_cmd = json::value_to<std::string_view>(it->value());
            app_.MovePlayer(*token_opt, move_cmd);
            return MakeJsonResponse(http::status::ok, json::object{});
        }
    } catch (...) {}

    return MakeJsonResponse(http::status::bad_request, 
        json::value{{"code", "invalidArgument"}, {"message", "Failed to parse action"}});
}

http::response<http::string_body> ApiHandler::HandleTick(std::string_view body_str) {
    if (app_.IsTickAuto()) {
        return MakeJsonResponse(http::status::bad_request, 
            json::value{{"code", "badRequest"}, {"message", "Invalid endpoint"}});
    }

    try {
        auto body = json::parse({body_str.data(), body_str.size()});
        auto const& obj = body.as_object();
        if (auto it = obj.find("timeDelta"); it != obj.end()) {
            uint64_t delta_ms = it->value().as_int64();
            app_.Tick(std::chrono::milliseconds(delta_ms));
            return MakeJsonResponse(http::status::ok, json::object{});
        }
    } catch (...) {}

    return MakeJsonResponse(http::status::bad_request, 
        json::value{{"code", "invalidArgument"}, {"message", "Failed to parse tick"}});
}

http::response<http::string_body> ApiHandler::HandleGetRecords(std::string_view query) {
    size_t start = 0;
    size_t max_items = 100;

    // Универсальный парсер параметров
    auto parse_query_param = [](std::string_view q, std::string_view key) -> std::optional<size_t> {
        size_t pos = 0;
        while ((pos = q.find(key, pos)) != std::string_view::npos) {
            bool is_start = (pos == 0 || q[pos - 1] == '&');
            size_t next_pos = pos + key.size();
            
            if (is_start && next_pos < q.size() && q[next_pos] == '=') {
                size_t val_start = next_pos + 1;
                size_t val_end = q.find('&', val_start);
                std::string_view val_str = q.substr(val_start, val_end - val_start);
                
                size_t result = 0;
                // std::from_chars корректно работает строго в границах val_str
                auto [ptr, ec] = std::from_chars(val_str.data(), val_str.data() + val_str.size(), result);
                if (ec == std::errc{}) {
                    return result;
                }
            }
            pos += key.size();
        }
        return std::nullopt;
    };

    if (auto start_opt = parse_query_param(query, "start")) start = *start_opt;
    if (auto max_opt = parse_query_param(query, "maxItems")) max_items = *max_opt;

    // Проверка лимита maxItems по ТЗ
    if (max_items > 100) {
        http::response<http::string_body> response(http::status::bad_request, 11);
        response.set(http::field::content_type, "application/json");
        response.body() = R"({"code": "badRequest", "message": "maxItems cannot exceed 100"})";
        response.prepare_payload();
        return response;
    }

    // Получаем записи из базы данных
    auto records = app_.GetRecords(start, max_items);

    // ВРУЧНУЮ собираем JSON-массив в виде строки.
    std::string json_str = "[";
    for (size_t i = 0; i < records.size(); ++i) { // ИСПРАВЛЕНО: ++i вместо ++1
        const auto& record = records[i];
        
        // Превращаем имя в безопасную JSON-строку
        std::string escaped_name;
        escaped_name.reserve(record.name.size()); // Оптимизация аллокаций
        for (char c : record.name) {
            if (c == '"' || c == '\\') escaped_name += '\\';
            escaped_name += c;
        }

        // Форматируем play_time, гарантируя точку (float)
        char time_buf[64];
        int written = std::snprintf(time_buf, sizeof(time_buf), "%.3f", record.play_time);

        json_str += "{\"name\":\"" + escaped_name + "\",";
        json_str += "\"score\":" + std::to_string(record.score) + ",";
        json_str += "\"playTime\":" + std::string(time_buf, static_cast<size_t>(written)) + "}";
        
        if (i + 1 < records.size()) {
            json_str += ",";
        }
    }
    json_str += "]";

    // Формируем чистый HTTP-ответ
    http::response<http::string_body> response(http::status::ok, 11);
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.body() = std::move(json_str);
    response.prepare_payload();
    
    return response;
}


http::response<http::string_body> ApiHandler::MakeJsonResponse(http::status status, json::value body) {
    http::response<http::string_body> response(status, 11);
    response.set(http::field::content_type, "application/json");
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
    static constexpr std::string_view BEARER = "Bearer "sv;
    if (auth_header.starts_with(BEARER)) {
        std::string token_str(auth_header.substr(BEARER.size()));
        if (token_str.length() == 32) {
            return app::Token(token_str);
        }
    }
    return std::nullopt;
}

} // namespace http_handler
