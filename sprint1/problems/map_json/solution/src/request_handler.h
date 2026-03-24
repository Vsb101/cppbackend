#pragma once
#include "http_server.h"
#include "model.h"

#include <string>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

// Функции для преобразования модели в JSON
std::string MapToJson(const model::Map& map);
std::string MapsListToJson(const model::Game& game);

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        std::string target(req.target());
        
        // Убираем начальный слеш если есть
        if (!target.empty() && target[0] == '/') {
            target = target.substr(1);
        }

        http::response<http::string_body> response;

        // Проверяем метод запроса
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            response.result(http::status::method_not_allowed);
            response.set(http::field::content_type, "application/json");
            response.set(http::field::allow, "GET, HEAD");
            response.body() = R"({"code": "badRequest", "message": "Bad request"})";
            response.content_length(response.body().size());
            response.keep_alive(req.keep_alive());
            send(std::move(response));
            return;
        }

        // Обработка запросов к /api/v1/maps
        if (target.rfind("api/v1/maps", 0) == 0) {
            // Убираем префикс api/v1/maps
            std::string path = target.substr(11);
            
            if (path.empty()) {
                // GET /api/v1/maps - возвращаем список карт
                std::string body = MapsListToJson(game_);
                response.result(http::status::ok);
                response.set(http::field::content_type, "application/json");
                response.body() = std::move(body);
                response.content_length(response.body().size());
                response.keep_alive(req.keep_alive());
            } else if (path[0] == '/') {
                // GET /api/v1/maps/{id}
                std::string map_id = path.substr(1);
                
                const model::Map* map = game_.FindMap(model::Map::Id(map_id));
                if (map == nullptr) {
                    // Карта не найдена
                    response.result(http::status::not_found);
                    response.set(http::field::content_type, "application/json");
                    response.body() = R"({"code": "mapNotFound", "message": "Map not found"})";
                    response.content_length(response.body().size());
                    response.keep_alive(req.keep_alive());
                } else {
                    // Возвращаем карту
                    std::string body = MapToJson(*map);
                    response.result(http::status::ok);
                    response.set(http::field::content_type, "application/json");
                    response.body() = std::move(body);
                    response.content_length(response.body().size());
                    response.keep_alive(req.keep_alive());
                }
            } else {
                // Неправильный формат URI после /api/v1/maps
                response.result(http::status::bad_request);
                response.set(http::field::content_type, "application/json");
                response.body() = R"({"code": "badRequest", "message": "Bad request"})";
                response.content_length(response.body().size());
                response.keep_alive(req.keep_alive());
            }
        } else if (target.rfind("api/", 0) == 0) {
            // Запрос к /api/, но не подходит ни под один формат
            response.result(http::status::bad_request);
            response.set(http::field::content_type, "application/json");
            response.body() = R"({"code": "badRequest", "message": "Bad request"})";
            response.content_length(response.body().size());
            response.keep_alive(req.keep_alive());
        } else {
            // Для остальных запросов возвращаем 404
            response.result(http::status::not_found);
            response.set(http::field::content_type, "text/html");
            response.body() = "Not found";
            response.content_length(response.body().size());
            response.keep_alive(req.keep_alive());
        }

        send(std::move(response));
    }

private:
    model::Game& game_;
};

}  // namespace http_handler
