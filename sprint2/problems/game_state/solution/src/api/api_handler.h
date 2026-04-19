#pragma once
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <optional>
#include <string_view>

#include "../app/app.h"
#include "json_serialization.h"

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
using namespace std::literals;

class ApiHandler {
public:
    explicit ApiHandler(app::Application& app) : app_(app) {}

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        ProcessRequest(std::forward<decltype(req)>(req), std::forward<Send>(send));
    }

private:
    app::Application& app_;

    template <typename Value>
    static http::response<http::string_body> MakeJsonResponse(http::status status, const Value& body) {
        http::response<http::string_body> response(status, 11);
        response.set(http::field::content_type, "application/json");
        response.set(http::field::cache_control, "no-cache");
        response.body() = json::serialize(json::value_from(body));
        response.prepare_payload();
        return response;
    }

    template <typename Value>
    static http::response<http::string_body> MakeJsonResponseWithAllow(
        http::status status, const Value& body, std::string_view allow_header) {
        auto response = MakeJsonResponse(status, body);
        response.set(http::field::allow, allow_header);
        return response;
    }

    static std::optional<app::Token> TryExtractToken(std::string_view auth_header) {
        if (auth_header.empty() || !auth_header.starts_with("Bearer "sv)) {
            return std::nullopt;
        }
        std::string token_str{auth_header.substr(7)};
        // Ожидаем токен длиной 32 символа (hex)
        return token_str.size() == 32 ? std::make_optional(app::Token{token_str}) : std::nullopt;
    }

    template <typename Body, typename Allocator, typename Send>
    void ProcessRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto target = req.target();
        auto method = req.method();

        // --- MAPS ---
        if (target == "/api/v1/maps"sv || target == "/api/v1/maps/"sv) {
            if (method == http::verb::get) return HandleGetMaps(std::forward<Send>(send));
            return send(MakeJsonResponseWithAllow(http::status::method_not_allowed, 
                json::value{{"code", "invalidMethod"}, {"message", "GET method is expected"}}, "GET"));
        }
        
        if (target.starts_with("/api/v1/maps/"sv)) {
            if (method == http::verb::get) {
                auto map_id = target.substr("/api/v1/maps/"sv.size());
                return HandleGetMapById(std::string(map_id), std::forward<Send>(send));
            }
            return send(MakeJsonResponseWithAllow(http::status::method_not_allowed, 
                json::value{{"code", "invalidMethod"}, {"message", "GET method is expected"}}, "GET"));
        }

        // --- GAME JOIN ---
        if (target == "/api/v1/game/join"sv) {
            if (method == http::verb::post) return HandleJoinGame(std::move(req), std::forward<Send>(send));
            return send(MakeJsonResponseWithAllow(http::status::method_not_allowed, 
                json::value{{"code", "invalidMethod"}, {"message", "Only POST is allowed"}}, "POST"));
        }

        // --- GAME PLAYERS ---
        if (target == "/api/v1/game/players"sv) {
            if (method == http::verb::get || method == http::verb::head) 
                return HandleGetPlayers(std::move(req), std::forward<Send>(send));
            return send(MakeJsonResponseWithAllow(http::status::method_not_allowed, 
                json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"));
        }

        // --- GAME STATE (Новое!) ---
        if (target == "/api/v1/game/state"sv) {
            if (method == http::verb::get || method == http::verb::head)
                return HandleGetGameState(std::move(req), std::forward<Send>(send));
            return send(MakeJsonResponseWithAllow(http::status::method_not_allowed, 
                json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"));
        }

        // Default 400
        send(MakeJsonResponse(http::status::bad_request, 
            json::value{{"code", "badRequest"}, {"message", "Unknown endpoint"}}));
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleGetGameState(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto token = TryExtractToken(req[http::field::authorization]);
        if (!token) {
            return send(MakeJsonResponse(http::status::unauthorized, 
                json::value{{"code", "invalidToken"}, {"message", "Authorization header is required"}}));
        }

        auto player = app_.FindPlayerByToken(*token);
        if (!player) {
            return send(MakeJsonResponse(http::status::unauthorized, 
                json::value{{"code", "unknownToken"}, {"message", "Player token has not been found"}}));
        }

        auto session = player->GetSession();
        if (!session) {
             return send(MakeJsonResponse(http::status::internal_server_error, "Session error"));
        }

        json::object players_obj;
        // В ТЗ в /game/state ключами являются ID собак (или игроков)
        for (const auto& dog_ptr : session->GetDogs()) {
            players_obj[std::to_string(*dog_ptr->GetId())] = json::value_from(*dog_ptr);
        }

        send(MakeJsonResponse(http::status::ok, json::value{{"players", players_obj}}));
    }

    template <typename Send>
    void HandleGetMapById(const std::string& id, Send&& send) {
        auto map = app_.FindMap(model::Map::Id{id});
        if (!map) {
            return send(MakeJsonResponse(http::status::not_found, 
                json::value{{"code", "mapNotFound"}, {"message", "Map not found"}}));
        }
        send(MakeJsonResponse(http::status::ok, *map));
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleJoinGame(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        try {
            auto body = json::parse(req.body());
            std::string name = std::string(body.at("userName").as_string());
            std::string map_id = std::string(body.at("mapId").as_string());

            if (name.empty()) {
                return send(MakeJsonResponse(http::status::bad_request, 
                    json::value{{"code", "invalidArgument"}, {"message", "Invalid name"}}));
            }

            auto [token, player_id] = app_.JoinGame(name, model::Map::Id{map_id});
            send(MakeJsonResponse(http::status::ok, 
                json::value{{"authToken", *token}, {"playerId", *player_id}}));
        } catch (...) {
            send(MakeJsonResponse(http::status::bad_request, 
                json::value{{"code", "invalidArgument"}, {"message", "Join game request parse error"}}));
        }
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleGetPlayers(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto token = TryExtractToken(req[http::field::authorization]);
        if (!token) {
            return send(MakeJsonResponse(http::status::unauthorized, 
                json::value{{"code", "invalidToken"}, {"message", "Authorization header is required"}}));
        }
        
        auto players = app_.GetPlayersInSession(*token);
        if (players.empty()) {
            return send(MakeJsonResponse(http::status::unauthorized, 
                json::value{{"code", "unknownToken"}, {"message", "Player token has not been found"}}));
        }
        
        json::object res;
        for (const auto& p : players) {
            res[std::to_string(*p->GetId())] = json::value{{"name", p->GetName()}};
        }
        send(MakeJsonResponse(http::status::ok, res));
    }

    template <typename Send>
    void HandleGetMaps(Send&& send) {
        const auto& maps = app_.ListMaps();
        json::array result;
        for (const auto& map : maps) {
            result.push_back(json::value{{"id", *map->GetId()}, {"name", map->GetName()}});
        }
        send(MakeJsonResponse(http::status::ok, result));
    }
};

} // namespace http_handler
