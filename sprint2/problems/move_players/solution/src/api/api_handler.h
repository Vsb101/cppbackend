#pragma once
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <optional>
#include <string_view>
#include <unordered_set>

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
        return token_str.size() == 32 ? std::make_optional(app::Token{std::move(token_str)}) : std::nullopt;
    }

    template <typename Body, typename Allocator, typename Send>
    void ProcessRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto target = req.target();
        auto method = req.method();

        // --- /api/v1/maps ---
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

        // --- /api/v1/game/join ---
        if (target == "/api/v1/game/join"sv) {
            if (method == http::verb::post) return HandleJoinGame(std::move(req), std::forward<Send>(send));
            return send(MakeJsonResponseWithAllow(http::status::method_not_allowed, 
                json::value{{"code", "invalidMethod"}, {"message", "Only POST is allowed"}}, "POST"));
        }

        // --- /api/v1/game/players ---
        if (target == "/api/v1/game/players"sv) {
            if (method == http::verb::get || method == http::verb::head) 
                return HandleGetPlayers(std::move(req), std::forward<Send>(send));
            return send(MakeJsonResponseWithAllow(http::status::method_not_allowed, 
                json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"));
        }

        // --- /api/v1/game/state ---
        if (target == "/api/v1/game/state"sv) {
            if (method == http::verb::get || method == http::verb::head)
                return HandleGetGameState(std::move(req), std::forward<Send>(send));
            return send(MakeJsonResponseWithAllow(http::status::method_not_allowed, 
                json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "GET, HEAD"));
        }

        // --- /api/v1/game/player/action ---
        if (target == "/api/v1/game/player/action"sv) {
            if (method == http::verb::post)
                return HandlePlayerAction(std::move(req), std::forward<Send>(send));
            return send(MakeJsonResponseWithAllow(http::status::method_not_allowed, 
                json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}, "POST"));
        }

        // Default 400
        send(MakeJsonResponse(http::status::bad_request, 
            json::value{{"code", "badRequest"}, {"message", "Unknown endpoint"}}));
    }

    template <typename Body, typename Allocator, typename Send>
    void HandlePlayerAction(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Проверка Content-Type
        if (req[http::field::content_type] != "application/json") {
            return send(MakeJsonResponse(http::status::bad_request, 
                json::value{{"code", "invalidArgument"}, {"message", "Invalid content type"}}));
        }

        auto token_opt = TryExtractToken(req[http::field::authorization]);
        if (!token_opt) {
            return send(MakeJsonResponse(http::status::unauthorized, 
                json::value{{"code", "invalidToken"}, {"message", "Authorization header is required"}}));
        }

        std::string move_cmd;
        try {
            auto body = json::parse(req.body());
            move_cmd = json::value_to<std::string>(body.as_object().at("move"));
        } catch (...) {
            return send(MakeJsonResponse(http::status::bad_request, 
                json::value{{"code", "invalidArgument"}, {"message", "Failed to parse action"}}));
        }

        // Валидация значений move (L, R, U, D, "")
        static const std::unordered_set<std::string_view> valid_moves = {"L", "R", "U", "D", ""};
        if (!valid_moves.contains(move_cmd)) {
             return send(MakeJsonResponse(http::status::bad_request, 
                json::value{{"code", "invalidArgument"}, {"message", "Invalid move value"}}));
        }

        try {
            app_.MovePlayer(*token_opt, move_cmd);
            return send(MakeJsonResponse(http::status::ok, json::object{}));
        } catch (const std::invalid_argument& e) {
            if (std::string_view(e.what()) == "UnknownToken") {
                return send(MakeJsonResponse(http::status::unauthorized, 
                    json::value{{"code", "unknownToken"}, {"message", "Player token has not been found"}}));
            }
            return send(MakeJsonResponse(http::status::bad_request, 
                json::value{{"code", "invalidArgument"}, {"message", e.what()}}));
        }
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleGetGameState(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto token_opt = TryExtractToken(req[http::field::authorization]);
        if (!token_opt) {
            return send(MakeJsonResponse(http::status::unauthorized, 
                json::value{{"code", "invalidToken"}, {"message", "Authorization header is required"}}));
        }

        auto player = app_.FindPlayerByToken(*token_opt);
        if (!player) {
            return send(MakeJsonResponse(http::status::unauthorized, 
                json::value{{"code", "unknownToken"}, {"message", "Player token has not been found"}}));
        }

        auto session = player->GetSession();
        json::object players_obj;
        // Состояние всех собак в текущей сессии
        for (const auto& dog_ptr : session->GetDogs()) {
            players_obj[std::to_string(*dog_ptr->GetId())] = json::value_from(*dog_ptr);
        }

        send(MakeJsonResponse(http::status::ok, json::value{{"players", players_obj}}));
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleJoinGame(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        try {
            auto body = json::parse(req.body());
            std::string name = std::string(body.as_object().at("userName").as_string());
            std::string map_id = std::string(body.as_object().at("mapId").as_string());

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
        auto token_opt = TryExtractToken(req[http::field::authorization]);
        if (!token_opt) {
            return send(MakeJsonResponse(http::status::unauthorized, 
                json::value{{"code", "invalidToken"}, {"message", "Authorization header is required"}}));
        }
        
        auto players = app_.GetPlayersInSession(*token_opt);
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
    void HandleGetMapById(const std::string& id, Send&& send) {
        auto map = app_.FindMap(model::Map::Id{id});
        if (!map) {
            return send(MakeJsonResponse(http::status::not_found, 
                json::value{{"code", "mapNotFound"}, {"message", "Map not found"}}));
        }
        send(MakeJsonResponse(http::status::ok, *map));
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
