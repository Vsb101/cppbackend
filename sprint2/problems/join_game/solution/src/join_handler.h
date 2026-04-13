#pragma once

#include "http_server.h"
#include "model.h"

#include <string>

namespace http_handler {

namespace http = boost::beast::http;

// Обработчик запроса на вход в игру
http::response<http::string_body> HandleJoinGame(const http::request<http::string_body>& req, model::Game& game);

// Обработчик получения списка игроков
http::response<http::string_body> HandleGetPlayers(const http::request<http::string_body>& req, const model::Game& game);

} // namespace http_handler