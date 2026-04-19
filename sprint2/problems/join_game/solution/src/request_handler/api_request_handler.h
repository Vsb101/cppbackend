#pragma once

/**
 * @file api_request_handler.h
 * @brief Обработчики API-запросов
 */

#include <boost/beast/http.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../app/app.h"
#include "../json/json_handler.h"
#include "../model/player_tokens.h"
#include "../other/util.h"

namespace requestHandler {

namespace beast = boost::beast;
namespace http = beast::http;
using StringResponse = http::response<http::string_body>;

/**
 * @brief Константы размеров URL-сегментов
 */
constexpr size_t k_two_segment_size = 2;
constexpr size_t k_three_segment_size = 3;
constexpr size_t k_four_segment_size = 4;

/**
 * @brief Проверка: базовый BAD_REQUEST
 * 
 * @tparam Request Тип запроса
 * @param req HTTP-запрос
 * @return true если это API v1 с неправильной структурой
 * @return false иначе
 */
template <typename Request>
[[nodiscard]] bool IsBadRequest(const Request& req) {
    auto url = util::SplitStr(req.target());
    if (url.empty() || url[0] != "api") {
        return false;
    }
    if (url.size() < k_two_segment_size || url[1] != "v1") {
        return true;
    }
    return false;
}

/**
 * @brief Проверка: базовый BAD_REQUEST (alias для совместимости)
 */
template <typename Request>
[[nodiscard]] bool IsBadRequestCheck(const Request& req) {
    return IsBadRequest(req);
}

/**
 * @brief Обработчик: BAD_REQUEST
 * 
 * @tparam Request Тип запроса
 * @tparam Send Тип функции отправки
 * @param req HTTP-запрос
 * @param application Приложение
 * @param send Функция отправки ответа
 * @return std::nullopt
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> HandleBadRequest(const Request& req,
                                                     app::Application& application,
                                                     Send&& send) {
    StringResponse response(http::status::bad_request, req.version());
    response.set(http::field::content_type, "application/json");
    response.body() = jsonOperation::BadRequest();
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    send(response);
    return std::nullopt;
}

/**
 * @brief Проверка: GET списка карт
 */
template <typename Request>
[[nodiscard]] bool IsGetMapList(const Request& req) {
    return req.target() == "/api/v1/maps" || req.target() == "/api/v1/maps/";
}

/**
 * @brief Обработчик: GET списка карт
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> GetMapList(const Request& req,
                                               app::Application& application,
                                               Send&& send) {
    StringResponse response(http::status::ok, req.version());
    response.set(http::field::content_type, "application/json");
    response.body() = jsonOperation::GameToJson(application.ListMap());
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    send(response);
    return std::nullopt;
}

/**
 * @brief Проверка: GET карты по ID
 */
template <typename Request>
[[nodiscard]] bool IsGetMapById(const Request& req) {
    auto url = util::SplitStr(req.target());
    return url.size() == k_four_segment_size && url[0] == "api" && url[1] == "v1" &&
           url[2] == "maps";
}

/**
 * @brief Обработчик: GET карты по ID
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> GetMapById(const Request& req,
                                               app::Application& application,
                                               Send&& send) {
    auto id = util::SplitStr(req.target())[3];
    auto map = application.FindMap(model::Map::Id(std::string(id)));
    if (map == nullptr) {
        return 0;
    }
    http::response<http::string_body> response(http::status::ok, req.version());
    response.set(http::field::content_type, "application/json");
    response.body() = jsonOperation::MapToJson(*application.FindMap(
        model::Map::Id(std::string(id))));
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    send(response);
    return std::nullopt;
}

/**
 * @brief Обработчик: MAP_NOT_FOUND
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> HandleMapNotFound(const Request& req,
                                                      app::Application& application,
                                                      Send&& send) {
    StringResponse response(http::status::not_found, req.version());
    response.set(http::field::content_type, "application/json");
    response.body() = jsonOperation::MapNotFound();
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    send(response);
    return std::nullopt;
}

/**
 * @brief Проверка: невалидный JSON при join
 */
template <typename Request>
[[nodiscard]] bool IsJoinToGameInvalidJson(const Request& req) {
    return (req.target() == "/api/v1/game/join" || req.target() == "/api/v1/game/join/") &&
           !jsonOperation::ParseJoinToGame(req.body());
}

/**
 * @brief Обработчик: невалидный JSON при join
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> JoinToGameInvalidJson(const Request& req,
                                                          app::Application& application,
                                                          Send&& send) {
    std::string body = jsonOperation::JoinToGameInvalidArgument();
    StringResponse response(http::status::bad_request, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(req.keep_alive());
    send(response);
    return std::nullopt;
}

/**
 * @brief Проверка: пустое имя игрока при join
 */
template <typename Request>
[[nodiscard]] bool IsJoinToGameEmptyPlayerName(const Request& req) {
    if ((req.target() == "/api/v1/game/join" || req.target() == "/api/v1/game/join/")) {
        auto res = jsonOperation::ParseJoinToGame(req.body());
        if (!res) {
            return false;
        }
        auto [player_name, map_id] = res.value();
        return player_name.empty();
    }
    return false;
}

/**
 * @brief Обработчик: пустое имя игрока при join
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> JoinToGameEmptyPlayerName(const Request& req,
                                                              app::Application& application,
                                                              Send&& send) {
    std::string body = jsonOperation::JoinToGameEmptyPlayerName();
    StringResponse response(http::status::bad_request, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(req.keep_alive());
    send(response);
    return std::nullopt;
}

/**
 * @brief Проверка: join в игру
 */
template <typename Request>
[[nodiscard]] bool IsJoinToGame(const Request& req) {
    return (req.target() == "/api/v1/game/join" || req.target() == "/api/v1/game/join/");
}

/**
 * @brief Обработчик: join в игру
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> JoinToGame(const Request& req,
                                               app::Application& application,
                                               Send&& send) {
    auto parsed = jsonOperation::ParseJoinToGame(req.body());
    if (!parsed) {
        return 0;
    }
    auto [player_name, map_id] = parsed.value();
    if (application.FindMap(map_id) == nullptr) {
        return 0;
    }
    auto [token, player_id] = application.JoinGame(player_name, map_id);
    std::string body = jsonOperation::JoinToGame(*token, *player_id);
    StringResponse response(http::status::ok, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(req.keep_alive());
    send(response);
    return std::nullopt;
}

/**
 * @brief Обработчик: карта не найдена при join
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> JoinToGameMapNotFound(const Request& req,
                                                          app::Application& application,
                                                          Send&& send) {
    std::string body = jsonOperation::JoinToGameMapNotFound();
    StringResponse response(http::status::not_found, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(req.keep_alive());
    send(response);
    return std::nullopt;
}

/**
 * @brief Обработчик: только POST метод разрешён
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> OnlyPostMethodAllowed(const Request& req,
                                                          app::Application& application,
                                                          Send&& send) {
    std::string body = jsonOperation::OnlyPostMethodAllowed();
    StringResponse response(http::status::method_not_allowed, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.set(http::field::allow, "POST");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(req.keep_alive());
    send(response);
    return std::nullopt;
}

/**
 * @brief Проверка: пустой токен авторизации
 */
template <typename Request>
[[nodiscard]] bool IsEmptyAuthorization(const Request& req) {
    return ((req.target() == "/api/v1/game/players" ||
             req.target() == "/api/v1/game/players/") ||
            (req.target() == "/api/v1/game/state" ||
             req.target() == "/api/v1/game/state/")) &&
           (req[http::field::authorization].empty() ||
            util::GetBearerToken(req[http::field::authorization]).empty());
}

/**
 * @brief Обработчик: пустой токен авторизации
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> EmptyAuthorization(const Request& req,
                                                       app::Application& application,
                                                       Send&& send) {
    std::string body = jsonOperation::EmptyAuthorization();
    StringResponse response(http::status::unauthorized, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.keep_alive(req.keep_alive());
    if (req.method() == http::verb::head) {
        response.content_length(body.size());
    } else {
        response.body() = body;
        response.content_length(body.size());
    }
    send(response);
    return std::nullopt;
}

/**
 * @brief Проверка: GET списка игроков
 */
template <typename Request>
[[nodiscard]] bool IsGetPlayersList(const Request& req) {
    return (req.target() == "/api/v1/game/players" ||
            req.target() == "/api/v1/game/players/");
}

/**
 * @brief Обработчик: GET списка игроков
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> GetPlayersList(const Request& req,
                                                   app::Application& application,
                                                   Send&& send) {
    model::Token token{util::GetBearerToken(req[http::field::authorization])};
    if (!application.CheckPlayerByToken(token)) {
        return 0;
    }
    auto players_opt = application.GetPlayersFromSession(token);
    if (!players_opt) {
        return 0;
    }
    std::string body = jsonOperation::PlayersListOnMap(players_opt.value());
    StringResponse response(http::status::ok, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.keep_alive(req.keep_alive());
    if (req.method() == http::verb::head) {
        response.content_length(body.size());
    } else {
        response.body() = body;
        response.content_length(body.size());
    }
    send(response);
    return std::nullopt;
}

/**
 * @brief Обработчик: невалидный метод
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> HandleInvalidMethod(const Request& req,
                                                        app::Application& application,
                                                        Send&& send) {
    std::string body = jsonOperation::InvalidMethod();
    StringResponse response(http::status::method_not_allowed, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.set(http::field::allow, "GET, HEAD");
    response.keep_alive(req.keep_alive());
    if (req.method() == http::verb::head) {
        response.content_length(body.size());
    } else {
        response.body() = body;
        response.content_length(body.size());
    }
    send(response);
    return std::nullopt;
}

/**
 * @brief Проверка: GET состояния игры
 */
template <typename Request>
[[nodiscard]] bool IsGameState(const Request& req) {
    return (req.target() == "/api/v1/game/state" || req.target() == "/api/v1/game/state/");
}

/**
 * @brief Обработчик: GET состояния игры
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> GetGameState(const Request& req,
                                                 app::Application& application,
                                                 Send&& send) {
    model::Token token{util::GetBearerToken(req[http::field::authorization])};
    if (!application.CheckPlayerByToken(token)) {
        return 0;
    }
    auto players_opt = application.GetPlayersFromSession(token);
    if (!players_opt) {
        return 0;
    }
    std::string body = jsonOperation::GameState(players_opt.value());
    StringResponse response(http::status::ok, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.keep_alive(req.keep_alive());
    if (req.method() == http::verb::head) {
        response.content_length(body.size());
    } else {
        response.body() = body;
        response.content_length(body.size());
    }
    send(response);
    return std::nullopt;
}

/**
 * @brief Обработчик: неизвестный токен
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> UnknownToken(const Request& req,
                                                 app::Application& application,
                                                 Send&& send) {
    std::string body = jsonOperation::UnknownToken();
    StringResponse response(http::status::unauthorized, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.keep_alive(req.keep_alive());
    if (req.method() == http::verb::head) {
        response.content_length(body.size());
    } else {
        response.body() = body;
        response.content_length(body.size());
    }
    send(response);
    return std::nullopt;
}

/**
 * @brief Обработчик: PAGE_NOT_FOUND
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> PageNotFound(const Request& req,
                                                 app::Application& application,
                                                 Send&& send) {
    std::string body = jsonOperation::PageNotFound();
    StringResponse response(http::status::not_found, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(req.keep_alive());
    send(response);
    return std::nullopt;
}

/**
 * @brief Проверка: невалидный метод для join
 */
template <typename Request>
[[nodiscard]] bool IsJoinToGameInvalidMethod(const Request& req) {
    return (req.target() == "/api/v1/game/join" || req.target() == "/api/v1/game/join/") &&
           req.method() != http::verb::post;
}

/**
 * @brief Проверка: невалидный Content-Type для join
 */
template <typename Request>
[[nodiscard]] bool IsJoinToGameInvalidContentType(const Request& req) {
    if (req.target() == "/api/v1/game/join" || req.target() == "/api/v1/game/join/") {
        if (req.method() != http::verb::post) {
            return false;
        }
        auto content_type = req[http::field::content_type];
        return content_type.empty() ||
               content_type.find("application/json") == std::string::npos;
    }
    return false;
}

/**
 * @brief Обработчик: невалидный Content-Type для join
 */
template <typename Request, typename Send>
[[nodiscard]] std::optional<size_t> JoinToGameInvalidContentType(const Request& req,
                                                                 app::Application& application,
                                                                 Send&& send) {
    std::string body = jsonOperation::JoinToGameInvalidArgument();
    StringResponse response(http::status::bad_request, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(req.keep_alive());
    send(response);
    return std::nullopt;
}

}  // namespace requestHandler