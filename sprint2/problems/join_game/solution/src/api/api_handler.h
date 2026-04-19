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

    /**
     * @brief Универсальный хелпер для создания JSON-ответов.
     * Явное использование json::value_from позволяет передавать как json::value, 
     * так и объекты модели, для которых определен tag_invoke.
     */
    template <typename Value>
    static http::response<http::string_body> MakeJsonResponse(http::status status, const Value& body) {
        http::response<http::string_body> response(status, 11);
        response.set(http::field::content_type, "application/json");
        response.set(http::field::cache_control, "no-cache");
        response.body() = json::serialize(json::value_from(body));
        response.prepare_payload();
        return response;
    }

    /**
     * @brief Извлечение токена из заголовка Authorization: Bearer <token>
     */
    static std::optional<app::Token> TryExtractToken(std::string_view auth_header) {
        if (auth_header.empty() || !auth_header.starts_with("Bearer "sv)) {
            return std::nullopt;
        }
        std::string token_str{auth_header.substr(7)};
        return token_str.size() == 32 ? std::make_optional(app::Token{token_str}) : std::nullopt;
    }

    /**
     * @brief Основной диспетчер API-запросов
     */
    template <typename Body, typename Allocator, typename Send>
    void ProcessRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto target = req.target();
        if (target == "/api/v1/game/join"sv) {
            if (req.method() == http::verb::post) {
                HandleJoinGame(std::move(req), std::forward<Send>(send));
            } else {
                send(MakeJsonResponse(http::status::method_not_allowed, 
                    json::value{{"code", "invalidMethod"}, {"message", "Only POST is allowed"}}));
            }
        } else if (target == "/api/v1/game/players"sv) {
            // ТЗ обычно требует GET или HEAD для списка игроков
            if (req.method() == http::verb::get || req.method() == http::verb::head) {
                HandleGetPlayers(std::move(req), std::forward<Send>(send));
            } else {
                send(MakeJsonResponse(http::status::method_not_allowed, 
                    json::value{{"code", "invalidMethod"}, {"message", "Invalid method"}}));
            }
        } else {
            send(MakeJsonResponse(http::status::bad_request, 
                json::value{{"code", "badRequest"}, {"message", "Unknown endpoint"}}));
        }
    }

    /**
     * @brief POST /api/v1/game/join
     */
    template <typename Body, typename Allocator, typename Send>
    void HandleJoinGame(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        try {
            auto body = json::parse(req.body());
            auto name = body.at("userName").as_string();
            auto map_id = body.at("mapId").as_string();
            
            if (name.empty()) {
                return send(MakeJsonResponse(http::status::bad_request, 
                    json::value{{"code", "invalidArgument"}, {"message", "Invalid name"}}));
            }
            
            auto [token, player_id] = app_.JoinGame(std::string(name), model::Map::Id{std::string(map_id)});
            
            send(MakeJsonResponse(http::status::ok, 
                json::value{{"authToken", *token}, {"playerId", *player_id}}));
        } catch (...) {
            send(MakeJsonResponse(http::status::not_found, 
                json::value{{"code", "mapNotFound"}, {"message", "Map not found"}}));
        }
    }

    /**
     * @brief GET /api/v1/game/players
     */
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
            res[std::to_string(*p->GetId())] = json::value_from(*p);
        }
        send(MakeJsonResponse(http::status::ok, res));
    }
};

} // namespace http_handler
