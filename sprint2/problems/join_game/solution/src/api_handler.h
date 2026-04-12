#pragma once

#include "game_model.h"
#include "model.h"

#include <boost/beast/http.hpp>
#include <boost/utility/string_view.hpp>
#include <functional>
#include <string>

namespace api_handler {

namespace http = boost::beast::http;
namespace beast = boost::beast;

/**
 * @class ApiHandler
 * @brief Обработчик API запросов игрового сервера.
 * 
 * Обрабатывает запросы:
 * - POST /api/v1/game/join - вход в игру
 * - GET /api/v1/game/players - получение списка игроков
 */
class ApiHandler {
public:
    using StringResponse = http::response<http::string_body>;
    using StringRequest = http::request<http::string_body>;

    /**
     * @brief Конструктор обработчика API
     * @param game Ссылка на модель игры
     * @param sessions Менеджер игровых сеансов
     */
    ApiHandler(model::Game& game, game_model::GameSessions& sessions);

    /// Обработка API запроса
    std::optional<StringResponse> HandleRequest(const StringRequest& req);

private:
    model::Game& game_;
    game_model::GameSessions& sessions_;

    // === POST /api/v1/game/join ===
    std::optional<StringResponse> HandleJoinGame(const StringRequest& req);
    StringResponse HandleJoinGamePost(const StringRequest& req);
    StringResponse MakeJoinErrorResponse(http::status status, const std::string& code, 
                                         const std::string& message, const StringRequest& req);

    // === GET /api/v1/game/players ===
    std::optional<StringResponse> HandleGetPlayers(const StringRequest& req);
    StringResponse MakePlayersResponse(const game_model::GameSession& session, const StringRequest& req);
    StringResponse MakeUnauthorizedResponse(const std::string& code, const std::string& message, 
                                           const StringRequest& req);
    StringResponse MakeMethodNotAllowedResponse(const StringRequest& req, beast::string_view allowed_methods);

    // === Вспомогательные методы ===
    static beast::string_view ExtractBearerToken(beast::string_view auth_header);
    static StringResponse MakeResponse(http::status status, beast::string_view body, 
                                       beast::string_view content_type, const StringRequest& req);
};

}  // namespace api_handler
