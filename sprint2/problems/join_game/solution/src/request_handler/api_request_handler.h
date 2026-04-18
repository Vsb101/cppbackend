#pragma once
#include <boost/beast/http.hpp>
#include <vector>

#include "../app/app.h"
#include "../model/player_tokens.h"
#include "../json/json_handler.h"
#include "../other/util.h"

namespace requestHandler {

namespace beast = boost::beast;
namespace http = beast::http;
using StringResponse = http::response<http::string_body>;

const size_t TWO_SEGMENT_SIZE = 2;
const size_t THREE_SEGMENT_SIZE = 3;
const size_t FOUR_SEGMENT_SIZE = 4;

template <typename Request>
bool BadRequestCheck(const Request& req) {
  // Проверяем только базовую структуру URL для /api/v1/...
  auto url = util::SplitStr(req.target());
  if (url.empty() || url[0] != "api") {
    return false;  // Не наш API, пусть другие обработчики решают
  }
  if (url.size() < 2 || url[1] != "v1") {
    return true;  // Неправильная версия API
  }
  // Остальные проверки делают специфичные чекеры
  return false;
};

template <typename Request, typename Send>
std::optional<size_t> BadRequest(const Request& req, app::Application& application,
                                 Send&& send) {
  StringResponse response(http::status::bad_request, req.version());
  response.set(http::field::content_type, "application/json");
  response.body() = jsonOperation::BadRequest();
  response.content_length(response.body().size());
  response.keep_alive(req.keep_alive());
  send(response);
  return std::nullopt;
};

template <typename Request>
bool GetMapListСheck(const Request& req) {
  return req.target() == "/api/v1/maps" || req.target() == "/api/v1/maps/";
}

template <typename Request, typename Send>
std::optional<size_t> GetMapList(const Request& req, app::Application& application,
                                 Send&& send) {
  StringResponse response(http::status::ok, req.version());
  response.set(http::field::content_type, "application/json");
  response.body() = jsonOperation::GameToJson(application.ListMap());
  response.content_length(response.body().size());
  response.keep_alive(req.keep_alive());
  send(response);
  return std::nullopt;
}

template <typename Request>
bool GetMapByIdCheck(const Request& req) {
  auto url = util::SplitStr(req.target());
  return url.size() == FOUR_SEGMENT_SIZE && url[0] == "api" && url[1] == "v1" &&
         url[2] == "maps";
}

template <typename Request, typename Send>
std::optional<size_t> GetMapById(const Request& req, app::Application& application,
                                 Send&& send) {
  auto id = util::SplitStr(req.target())[3];
  auto map = application.FindMap(model::Map::Id(std::string(id)));
  if (map == nullptr) {
    return 0;
  }
  http::response<http::string_body> response(http::status::ok, req.version());
  response.set(http::field::content_type, "application/json");
  response.body() =
      jsonOperation::MapToJson(*application.FindMap(model::Map::Id(std::string(id))));
  response.content_length(response.body().size());
  response.keep_alive(req.keep_alive());
  send(response);
  return std::nullopt;
};

template <typename Request, typename Send>
std::optional<size_t> MapNotFound(const Request& req, app::Application& application,
                                  Send&& send) {
  StringResponse response(http::status::not_found, req.version());
  response.set(http::field::content_type, "application/json");
  response.body() = jsonOperation::MapNotFound();
  response.content_length(response.body().size());
  response.keep_alive(req.keep_alive());
  send(response);
  return std::nullopt;
};

template <typename Request>
bool JoinToGameInvalidJsonCheck(const Request& req) {
  return (req.target() == "/api/v1/game/join" || req.target() == "/api/v1/game/join/") &&
         !jsonOperation::ParseJoinToGame(req.body());
}

template <typename Request, typename Send>
std::optional<size_t> JoinToGameInvalidJson(const Request& req,
                                            app::Application& application, Send&& send) {
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

template <typename Request>
bool JoinToGameEmptyPlayerNameCheck(const Request& req) {
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

template <typename Request, typename Send>
std::optional<size_t> JoinToGameEmptyPlayerName(const Request& req,
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

template <typename Request>
bool JoinToGameCheck(const Request& req) {
  return (req.target() == "/api/v1/game/join" || req.target() == "/api/v1/game/join/");
}

template <typename Request, typename Send>
std::optional<size_t> JoinToGame(const Request& req, app::Application& application,
                                 Send&& send) {
  auto parsed = jsonOperation::ParseJoinToGame(req.body());
  if (!parsed) {
    return 0;  // Вызовет JoinToGameInvalidJson
  }
  auto [player_name, map_id] = parsed.value();
  if (application.FindMap(map_id) == nullptr) {
    return 0;  // Вызовет JoinToGameMapNotFound
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

template <typename Request, typename Send>
std::optional<size_t> JoinToGameMapNotFound(const Request& req,
                                            app::Application& application, Send&& send) {
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

template <typename Request, typename Send>
std::optional<size_t> OnlyPostMethodAllowed(const Request& req,
                                            app::Application& application, Send&& send) {
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

template <typename Request>
bool EmptyAuthorizationCheck(const Request& req) {
  return ((req.target() == "/api/v1/game/players" ||
           req.target() == "/api/v1/game/players/") ||
          (req.target() == "/api/v1/game/state" ||
           req.target() == "/api/v1/game/state/")) &&
         (req[http::field::authorization].empty() ||
          util::GetBearerToken(req[http::field::authorization]).empty());
};

template <typename Request, typename Send>
std::optional<size_t> EmptyAuthorization(const Request& req,
                                         app::Application& application, Send&& send) {
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
};

template <typename Request>
bool GetPlayersListCheck(const Request& req) {
  return (req.target() == "/api/v1/game/players" ||
          req.target() == "/api/v1/game/players/");
};

template <typename Request, typename Send>
std::optional<size_t> GetPlayersList(const Request& req, app::Application& application,
                                     Send&& send) {
  model::Token token{util::GetBearerToken(req[http::field::authorization])};
  if (!application.CheckPlayerByToken(token)) {
    return 0;
  }
  auto players = application.GetPlayersFromSession(token);
  std::string body = jsonOperation::PlayersListOnMap(players);
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
};

template <typename Request, typename Send>
std::optional<size_t> InvalidMethod(const Request& req, app::Application& application,
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
};

template <typename Request>
bool GameStateCheck(const Request& req) {
  return (req.target() == "/api/v1/game/state" || req.target() == "/api/v1/game/state/");
}

template <typename Request, typename Send>
std::optional<size_t> GetGameState(const Request& req, app::Application& application,
                                   Send&& send) {
  model::Token token{util::GetBearerToken(req[http::field::authorization])};
  if (!application.CheckPlayerByToken(token)) {
    return 0;
  }
  auto players = application.GetPlayersFromSession(token);
  std::string body = jsonOperation::GameState(players);
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

template <typename Request, typename Send>
std::optional<size_t> UnknownToken(const Request& req, app::Application& application,
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

template <typename Request, typename Send>
std::optional<size_t> PageNotFound(const Request& req, app::Application& application,
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
};

template <typename Request>
bool JoinToGameInvalidMethodCheck(const Request& req) {
  return (req.target() == "/api/v1/game/join" || req.target() == "/api/v1/game/join/") &&
         req.method() != http::verb::post;
}

template <typename Request>
bool JoinToGameInvalidContentTypeCheck(const Request& req) {
  if (req.target() == "/api/v1/game/join" || req.target() == "/api/v1/game/join/") {
    if (req.method() != http::verb::post) {
      return false;  // Метод обрабатывается отдельно
    }
    auto content_type = req[http::field::content_type];
    return content_type.empty() || 
           content_type.find("application/json") == std::string::npos;
  }
  return false;
}

template <typename Request, typename Send>
std::optional<size_t> JoinToGameInvalidContentType(const Request& req,
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