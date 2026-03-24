#pragma once
#include "http_server.h"
#include "model.h"

#include <string>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

// Сериализует карту в JSON-строку (полное описание)
std::string MapToJson(const model::Map& map);
// Сериализует список карт в JSON-строку (только id и name)
std::string MapsListToJson(const model::Game& game);

// Обработчик HTTP-запросов.
// Делегирует запросы соответствующим методам Handle* в зависимости от URI.
class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    // Примечание
    // По сути, RequestHandler — это владелец ссылки на Game,
    // и он должен быть единичным (singleton).
    // Удаляя копирование, это подчёркиваем.
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    // Точка входа для обработки запроса.
    // Выполняет роутинг: определяет какой метод Handle* вызвать по URI.
    // Поддерживает GET и HEAD методы.
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        std::string target(req.target());
        if (!target.empty() && target[0] == '/') {
            target = target.substr(1);
        }

        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            send(MakeMethodNotAllowedResponse(req));
            return;
        }

        if (target.rfind("api/v1/maps", 0) == 0) {
            std::string path = target.substr(11);
            if (path.empty()) {
                send(HandleMapsList(req));
            } else if (path[0] == '/') {
                send(HandleMapById(req, path.substr(1)));
            } else {
                send(MakeBadRequestResponse(req));
            }
        } else if (target.rfind("api/", 0) == 0) {
            send(MakeBadRequestResponse(req));
        } else {
            send(MakeNotFoundResponse(req));
        }
    }

private:
    model::Game& game_;

    // Фабрики ответов
    http::response<http::string_body> MakeOkResponse(const std::string& body, const http::request<http::string_body>& req);
    http::response<http::string_body> MakeBadRequestResponse(const http::request<http::string_body>& req);
    http::response<http::string_body> MakeNotFoundResponse(const http::request<http::string_body>& req);
    http::response<http::string_body> MakeMethodNotAllowedResponse(const http::request<http::string_body>& req);

    // Обработчики конкретных эндпоинтов
    http::response<http::string_body> HandleMapsList(const http::request<http::string_body>& req);
    http::response<http::string_body> HandleMapById(const http::request<http::string_body>& req, const std::string& map_id);
};

}  // namespace http_handler
