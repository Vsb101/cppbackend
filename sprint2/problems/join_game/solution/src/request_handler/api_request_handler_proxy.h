#pragma once

/**
 * @file api_request_handler_proxy.h
 * @brief Прокси для обработки API-запросов
 */

#include <functional>
#include <optional>
#include <vector>

#include "../request_handler/api_request_handler.h"
#include "../request_handler/request_handler_unit.h"

namespace requestHandler {

namespace net = boost::asio;
namespace http = boost::beast::http;
using StringResponse = http::response<http::string_body>;

/**
 * @brief Прокси для обработки API-запросов
 * 
 * @tparam Request Тип HTTP-запроса
 * @tparam Send Тип функции отправки ответа
 */
template <typename Request, typename Send>
class ApiRequestHandlerProxy {
    using activator_type = bool (*)(const Request&);
    using handler_type = std::optional<size_t> (*)(const Request&, app::Application&, Send&);

 public:
    ApiRequestHandlerProxy(const ApiRequestHandlerProxy&) = delete;
    ApiRequestHandlerProxy& operator=(const ApiRequestHandlerProxy&) = delete;
    ApiRequestHandlerProxy(ApiRequestHandlerProxy&&) = delete;
    ApiRequestHandlerProxy& operator=(ApiRequestHandlerProxy&&) = delete;

    /**
     * @brief Получение единственного экземпляра (Singleton)
     */
    static ApiRequestHandlerProxy& GetInstance() {
        static ApiRequestHandlerProxy obj;
        return obj;
    }

    /**
     * @brief Выполнение обработки запроса
     * 
     * @param req HTTP-запрос
     * @param application Приложение
     * @param send Функция отправки ответа
     * @return true если запрос обработан
     * @return false если запрос не обработан
     */
    bool Execute(const Request& req, app::Application& application, Send&& send) {
        for (auto& item : requests_) {
            if (item.GetActivator()(req)) {
                auto& handler = item.GetHandler(req.method());
                if (handler) {
                    try {
                        auto res = handler(req, application, send);
                        while (res.has_value()) {
                            auto* add_handler = item.GetAddHandlerByIndex(res.value());
                            if (!add_handler) {
                                break;
                            }
                            res = add_handler(req, application, send);
                        }
                    } catch (const std::exception& ex) {
                        SendErrorResponse(req.version(), "internalError",
                                          "Internal server error", std::forward<Send>(send));
                    } catch (...) {
                        SendErrorResponse(req.version(), "internalError",
                                          "Internal server error", std::forward<Send>(send));
                    }
                }
                return true;
            }
        }
        return false;
    }

 private:
    /**
     * @brief Отправка ответа об ошибке
     */
    static void SendErrorResponse(unsigned int version,
                                  std::string_view code, std::string_view message,
                                  Send&& send) {
        StringResponse response(http::status::internal_server_error, version);
        response.set(http::field::content_type, "application/json");
        response.body() = R"({"code": "internalError", "message": "Internal server error"})";
        response.content_length(response.body().size());
        response.set(http::field::keep_alive, "true");
        send(std::move(response));
    }

    std::vector<RhUnit<activator_type, handler_type>> requests_ = {
        RhUnit<activator_type, handler_type>(IsGetMapList, {{http::verb::get, GetMapList}},
                                             HandleBadRequest),
        RhUnit<activator_type, handler_type>(IsGetMapById, {{http::verb::get, GetMapById}},
                                             HandleBadRequest, {HandleMapNotFound}),
        RhUnit<activator_type, handler_type>(IsJoinToGameInvalidMethod,
                                             {{http::verb::get, OnlyPostMethodAllowed},
                                              {http::verb::head, OnlyPostMethodAllowed},
                                              {http::verb::put, OnlyPostMethodAllowed},
                                              {http::verb::delete_, OnlyPostMethodAllowed},
                                              {http::verb::patch, OnlyPostMethodAllowed},
                                              {http::verb::options, OnlyPostMethodAllowed}},
                                             HandleBadRequest),
        RhUnit<activator_type, handler_type>(IsJoinToGameInvalidContentType,
                                             {{http::verb::post, JoinToGameInvalidContentType}},
                                             HandleBadRequest),
        RhUnit<activator_type, handler_type>(IsJoinToGameInvalidJson,
                                             {{http::verb::post, JoinToGameInvalidJson}},
                                             HandleBadRequest),
        RhUnit<activator_type, handler_type>(IsJoinToGameEmptyPlayerName,
                                             {{http::verb::post, JoinToGameEmptyPlayerName}},
                                             HandleBadRequest),
        RhUnit<activator_type, handler_type>(IsJoinToGame,
                                             {{http::verb::post, JoinToGame}}, HandleBadRequest,
                                             {JoinToGameMapNotFound}),
        RhUnit<activator_type, handler_type>(
            IsEmptyAuthorization,
            {{http::verb::get, EmptyAuthorization}, {http::verb::head, EmptyAuthorization}},
            HandleInvalidMethod),
        RhUnit<activator_type, handler_type>(
            IsGetPlayersList,
            {{http::verb::get, GetPlayersList}, {http::verb::head, GetPlayersList}},
            HandleInvalidMethod, {UnknownToken}),
        RhUnit<activator_type, handler_type>(
            IsGameState,
            {{http::verb::get, GetGameState}, {http::verb::head, GetGameState}},
            HandleInvalidMethod, {UnknownToken}),
        RhUnit<activator_type, handler_type>(IsBadRequestCheck,
                                             {{http::verb::get, HandleBadRequest}},
                                             HandleBadRequest)};

    ApiRequestHandlerProxy() = default;
};

}  // namespace requestHandler