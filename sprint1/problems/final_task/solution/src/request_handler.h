#pragma once

#include "http_server.h"
#include "model.h"

#include <boost/json.hpp>

#include <string>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

/**
 * @brief Сериализует карту в JSON-строку (полное описание)
 * @param map Карта для сериализации
 * @return JSON-строка со всеми полями: id, name, roads, buildings, offices
 */
std::string MapToJson(const model::Map& map);

/**
 * @brief Сериализует список карт в JSON-строку (только id и name)
 * @param game Игра, содержащая карты
 * @return JSON-массив с объектами, содержащими id и name для каждой карты
 */
std::string MapsListToJson(const model::Game& game);

/**
 * @brief Сериализует дорогу в JSON-объект
 * @param road Объект дороги
 * @return JSON-объект, представляющий дорогу
 *
 * Горизонтальная дорога:
 * {"x0": 0, "y0": 0, "x1": 10}
 *
 * Вертикальная дорога:
 * {"x0": 0, "y0": 0, "y1": 20}
 */
boost::json::object SerializeRoad(const model::Road& road);

/**
 * @brief Сериализует здание в JSON-объект
 * @param building Объект здания
 * @return JSON-объект, представляющий здание
 *
 * Формат:
 * {"x": 5, "y": 5, "w": 30, "h": 20}
 * где x, y - координаты верхнего левого угла, w, h - размеры
 */
boost::json::object SerializeBuilding(const model::Building& building);

/**
 * @brief Сериализует офис в JSON-объект
 * @param office Объект офиса
 * @return JSON-объект, представляющий офис
 *
 * Формат:
 * {"id": "o1", "x": 100, "y": 200, "offsetX": 5, "offsetY": 0}
 */
boost::json::object SerializeOffice(const model::Office& office);

/**
 * @class RequestHandler
 * @brief Обработчик HTTP-запросов.
 * 
 * Делегирует запросы соответствующим методам Handle* в зависимости от URI.
 * Поддерживает методы GET и HEAD.
 * 
 * @note По сути, RequestHandler — это владелец ссылки на Game,
 * и он должен быть единичным (singleton).
 */
class RequestHandler {
public:
    /**
     * @brief Конструктор
     * @param game Ссылка на модель игры
     */
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    // Удаляем копирование — подчёркиваем singleton-природу
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    /**
     * @brief Точка входа для обработки запроса
     * @tparam Body Тип тела запроса
     * @tparam Allocator Тип аллокатора
     * @tparam Send Тип callback-а для отправки ответа
     * @param req HTTP-запрос
     * @param send Callback для отправки ответа
     * 
     * Выполняет роутинг: определяет какой метод Handle* вызвать по URI.
     */
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
            if (path.empty() || path == "/") {
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
    /**
     * @brief Создаёт успешный HTTP-ответ с кодом 200 OK
     * @param body Тело ответа (JSON-строка)
     * @param req Исходный запрос (для keep_alive)
     * @return HTTP-ответ с Content-Type: application/json
     */
    http::response<http::string_body> MakeOkResponse(const std::string& body, const http::request<http::string_body>& req);
    
    /**
     * @brief Создаёт HTTP-ответ с кодом 400 Bad Request
     * @param req Исходный запрос
     * @return HTTP-ответ с кодом ошибки "badRequest"
     */
    http::response<http::string_body> MakeBadRequestResponse(const http::request<http::string_body>& req);
    
    /**
     * @brief Создаёт HTTP-ответ с кодом 404 Not Found
     * @param req Исходный запрос
     * @return HTTP-ответ для несуществующих ресурсов
     */
    http::response<http::string_body> MakeNotFoundResponse(const http::request<http::string_body>& req);
    
    /**
     * @brief Создаёт HTTP-ответ с кодом 405 Method Not Allowed
     * @param req Исходный запрос
     * @return HTTP-ответ с заголовком Allow
     */
    http::response<http::string_body> MakeMethodNotAllowedResponse(const http::request<http::string_body>& req);

    // Обработчики эндпоинтов
    /**
     * @brief Обрабатывает GET /api/v1/maps
     * @param req HTTP-запрос
     * @return JSON-массив со списком всех карт (только id и name)
     */
    http::response<http::string_body> HandleMapsList(const http::request<http::string_body>& req);
    
    /**
     * @brief Обрабатывает GET /api/v1/maps/{id}
     * @param req HTTP-запрос
     * @param map_id Идентификатор карты
     * @return Полное описание карты в JSON или 404, если не найдена
     */
    http::response<http::string_body> HandleMapById(const http::request<http::string_body>& req, const std::string& map_id);
};

}  // namespace http_handler