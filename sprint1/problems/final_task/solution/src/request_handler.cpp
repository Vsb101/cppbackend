#include "request_handler.h"

#include <boost/json.hpp>

namespace http_handler {

namespace json = boost::json;

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
json::object SerializeRoad(const model::Road& road) {
    json::object obj;
    obj["x0"] = road.GetStart().x;
    obj["y0"] = road.GetStart().y;
    
    if (road.IsHorizontal()) {
        obj["x1"] = road.GetEnd().x;
    } else {
        obj["y1"] = road.GetEnd().y;
    }
    return obj;
}

/**
 * @brief Сериализует здание в JSON-объект
 * @param building Объект здания
 * @return JSON-объект, представляющий здание
 *
 * Формат:
 * {"x": 5, "y": 5, "w": 30, "h": 20}
 * где x, y - координаты верхнего левого угла, w, h - размеры
 */
json::object SerializeBuilding(const model::Building& building) {
    json::object obj;
    const auto& bounds = building.GetBounds();
    obj["x"] = bounds.position.x;
    obj["y"] = bounds.position.y;
    obj["w"] = bounds.size.width;
    obj["h"] = bounds.size.height;
    return obj;
}

/**
 * @brief Сериализует офис в JSON-объект
 * @param office Объект офиса
 * @return JSON-объект, представляющий офис
 *
 * Формат:
 * {"id": "o1", "x": 100, "y": 200, "offsetX": 5, "offsetY": 0}
 */
json::object SerializeOffice(const model::Office& office) {
    json::object obj;
    obj["id"] = *office.GetId();
    obj["x"] = office.GetPosition().x;
    obj["y"] = office.GetPosition().y;
    obj["offsetX"] = office.GetOffset().dx;
    obj["offsetY"] = office.GetOffset().dy;
    return obj;
}

/**
 * @brief Сериализует карту в JSON-строку
 * @param map Карта для сериализации
 * @return JSON-строка со всеми полями: id, name, roads, buildings, offices
 * 
 * Использует отдельные функции сериализации для каждого типа объекта,
 * что обеспечивает расширяемость и поддерживаемость.
 * 
 * Пример вывода:
 * @code
 * {
 *   "id": "map1",
 *   "name": "First Map",
 *   "roads": [{"x0": 0, "y0": 0, "x1": 10}],
 *   "buildings": [{"x": 5, "y": 5, "w": 30, "h": 20}],
 *   "offices": [{"id": "o1", "x": 100, "y": 200, "offsetX": 5, "offsetY": 0}]
 * }
 * @endcode
 */
std::string MapToJson(const model::Map& map) {
    json::object result;
    
    result["id"] = *map.GetId();
    result["name"] = map.GetName();
    
    // Сериализуем дороги с использованием вспомогательной функции
    json::array roads;
    for (const auto& road : map.GetRoads()) {
        roads.push_back(SerializeRoad(road));
    }
    result["roads"] = roads;
    
    // Сериализуем здания с использованием вспомогательной функции
    json::array buildings;
    for (const auto& building : map.GetBuildings()) {
        buildings.push_back(SerializeBuilding(building));
    }
    result["buildings"] = buildings;
    
    // Сериализуем офисы с использованием вспомогательной функции
    json::array offices;
    for (const auto& office : map.GetOffices()) {
        offices.push_back(SerializeOffice(office));
    }
    result["offices"] = offices;
    
    return json::serialize(result);
}

/**
 * @brief Сериализует список карт в JSON-строку
 * @param game Игра, содержащая карты
 * @return JSON-массив с объектами, содержащими id и name для каждой карты
 * 
 * Используется для эндпоинта GET /api/v1/maps
 * 
 * Пример вывода:
 * @code
 * [{"id": "map1", "name": "First Map"}, {"id": "map2", "name": "Second Map"}]
 * @endcode
 */
std::string MapsListToJson(const model::Game& game) {
    json::array maps_array;
    
    for (const auto& map : game.GetMaps()) {
        json::object map_obj;
        map_obj["id"] = *map.GetId();
        map_obj["name"] = map.GetName();
        maps_array.push_back(map_obj);
    }
    
    return json::serialize(maps_array);
}

/**
 * @brief Создаёт успешный HTTP-ответ с кодом 200 OK
 * @param body Тело ответа (JSON-строка)
 * @param req Исходный запрос (для keep_alive)
 * @return HTTP-ответ с Content-Type: application/json
 */
http::response<http::string_body> RequestHandler::MakeOkResponse(const std::string& body, const http::request<http::string_body>& req) {
    http::response<http::string_body> response;
    response.result(http::status::ok);
    response.set(http::field::content_type, "application/json");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(req.keep_alive());
    return response;
}

/**
 * @brief Создаёт HTTP-ответ с кодом 400 Bad Request
 * @param req Исходный запрос
 * @return HTTP-ответ с кодом ошибки "badRequest"
 * 
 * Используется для неверного формата запроса или некорректного URI.
 */
http::response<http::string_body> RequestHandler::MakeBadRequestResponse(const http::request<http::string_body>& req) {
    http::response<http::string_body> response;
    response.result(http::status::bad_request);
    response.set(http::field::content_type, "application/json");
    response.body() = R"({"code": "badRequest", "message": "Bad request"})";
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    return response;
}

/**
 * @brief Создаёт HTTP-ответ с кодом 404 Not Found
 * @param req Исходный запрос
 * @return HTTP-ответ для несуществующих ресурсов
 * 
 * Используется для запросов к несуществующим ресурсам (не /api/...).
 */
http::response<http::string_body> RequestHandler::MakeNotFoundResponse(const http::request<http::string_body>& req) {
    http::response<http::string_body> response;
    response.result(http::status::not_found);
    response.set(http::field::content_type, "text/html");
    response.body() = "Not found";
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    return response;
}

/**
 * @brief Создаёт HTTP-ответ с кодом 405 Method Not Allowed
 * @param req Исходный запрос
 * @return HTTP-ответ с заголовком Allow, содержащим разрешённые методы
 * 
 * Используется для запросов с неподдерживаемым HTTP-методом (не GET/HEAD).
 */
http::response<http::string_body> RequestHandler::MakeMethodNotAllowedResponse(const http::request<http::string_body>& req) {
    http::response<http::string_body> response;
    response.result(http::status::method_not_allowed);
    response.set(http::field::content_type, "application/json");
    response.set(http::field::allow, "GET, HEAD");
    response.body() = R"({"code": "badRequest", "message": "Bad request"})";
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    return response;
}

/**
 * @brief Обрабатывает запрос GET /api/v1/maps
 * @param req HTTP-запрос
 * @return JSON-массив со списком всех карт (только id и name)
 */
http::response<http::string_body> RequestHandler::HandleMapsList(const http::request<http::string_body>& req) {
    std::string body = MapsListToJson(game_);
    return MakeOkResponse(body, req);
}

/**
 * @brief Обрабатывает запрос GET /api/v1/maps/{id}
 * @param req HTTP-запрос
 * @param map_id Идентификатор карты
 * @return Полное описание карты в JSON или 404 с кодом "mapNotFound", если не найдена
 */
http::response<http::string_body> RequestHandler::HandleMapById(const http::request<http::string_body>& req, const std::string& map_id) {
    const model::Map* map = game_.FindMap(model::Map::Id(map_id));
    if (map == nullptr) {
        http::response<http::string_body> response;
        response.result(http::status::not_found);
        response.set(http::field::content_type, "application/json");
        response.body() = R"({"code": "mapNotFound", "message": "Map not found"})";
        response.content_length(response.body().size());
        response.keep_alive(req.keep_alive());
        return response;
    }
    
    std::string body = MapToJson(*map);
    return MakeOkResponse(body, req);
}

}  // namespace http_handler