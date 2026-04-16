#pragma once
#include <functional>
#include <optional>
#include <vector>

#include "../request_handler/api_request_handler.h"
#include "../request_handler/request_handler_unit.h"

namespace requestHandler {

namespace net = boost::asio;
namespace http = boost::beast::http;
using StringResponse = http::response<http::string_body>;

template <typename Request, typename Send>
class ApiRequestHandlerProxy {
  using ActivatorType = bool (*)(const Request&);
  using HandlerType = std::optional<size_t> (*)(const Request&, app::Application&, Send&);

 public:
  /*Всё копирование запрещено*/
  ApiRequestHandlerProxy(const ApiRequestHandlerProxy&) = delete;
  ApiRequestHandlerProxy& operator=(const ApiRequestHandlerProxy&) = delete;
  ApiRequestHandlerProxy(ApiRequestHandlerProxy&&) = delete;
  ApiRequestHandlerProxy& operator=(ApiRequestHandlerProxy&&) = delete;

  static ApiRequestHandlerProxy& GetInstance() {  // Ссыль на объект
    static ApiRequestHandlerProxy obj;
    return obj;
  };

  bool Execute(const Request& req, app::Application& application,
               Send&& send) {  // Сам исполнитель
    for (auto item : requests_) {
      if (item.GetActivator()(req)) {
        auto handler = item.GetHandler(req.method());
        if (handler) {
          try {
            auto res = handler(req, application, send);
            while (res.has_value()) {
              auto add_handler = item.GetAddHandlerByIndex(res.value());
              if (!add_handler) break;
              res = add_handler(req, application, send);
            }
          } catch (const std::exception& ex) {
            StringResponse response(http::status::internal_server_error, req.version());
            response.set(http::field::content_type, "application/json");
            response.body() = R"({"code": "internalError", "message": "Internal server error"})";
            response.content_length(response.body().size());
            response.keep_alive(req.keep_alive());
            send(response);
          } catch (...) {
            StringResponse response(http::status::internal_server_error, req.version());
            response.set(http::field::content_type, "application/json");
            response.body() = R"({"code": "internalError", "message": "Internal server error"})";
            response.content_length(response.body().size());
            response.keep_alive(req.keep_alive());
            send(response);
          }
        }
        return true;
      }
    }
    return false;
  };

 private:
  std::vector<RHUnit<ActivatorType, HandlerType> > requests_ = {
      RHUnit<ActivatorType, HandlerType>(GetMapListСheck, {{http::verb::get, GetMapList}},
                                         BadRequest),
      RHUnit<ActivatorType, HandlerType>(GetMapByIdCheck, {{http::verb::get, GetMapById}},
                                         BadRequest, {MapNotFound}),
      RHUnit<ActivatorType, HandlerType>(JoinToGameInvalidMethodCheck,
                                         {{http::verb::get, OnlyPostMethodAllowed},
                                          {http::verb::head, OnlyPostMethodAllowed},
                                          {http::verb::put, OnlyPostMethodAllowed},
                                          {http::verb::delete_, OnlyPostMethodAllowed},
                                          {http::verb::patch, OnlyPostMethodAllowed},
                                          {http::verb::options, OnlyPostMethodAllowed}},
                                         BadRequest),
      RHUnit<ActivatorType, HandlerType>(JoinToGameInvalidContentTypeCheck,
                                         {{http::verb::post, JoinToGameInvalidContentType}},
                                         BadRequest),
      RHUnit<ActivatorType, HandlerType>(JoinToGameInvalidJsonCheck,
                                         {{http::verb::post, JoinToGameInvalidJson}},
                                         BadRequest),
      RHUnit<ActivatorType, HandlerType>(JoinToGameEmptyPlayerNameCheck,
                                         {{http::verb::post, JoinToGameEmptyPlayerName}},
                                         BadRequest),
      RHUnit<ActivatorType, HandlerType>(JoinToGameCheck,
                                         {{http::verb::post, JoinToGame}},
                                         BadRequest, {JoinToGameMapNotFound}),
      RHUnit<ActivatorType, HandlerType>(
          EmptyAuthorizationCheck,
          {{http::verb::get, EmptyAuthorization}, {http::verb::head, EmptyAuthorization}},
          InvalidMethod),
      RHUnit<ActivatorType, HandlerType>(
          GetPlayersListCheck,
          {{http::verb::get, GetPlayersList}, {http::verb::head, GetPlayersList}},
          InvalidMethod, {UnknownToken}),
      RHUnit<ActivatorType, HandlerType>(
          GameStateCheck,
          {{http::verb::get, GetGameState}, {http::verb::head, GetGameState}},
          InvalidMethod, {UnknownToken}),
      RHUnit<ActivatorType, HandlerType>(BadRequestCheck, {{http::verb::get, BadRequest}},
                                         BadRequest)};

  ApiRequestHandlerProxy() = default;
};

}  // namespace requestHandler