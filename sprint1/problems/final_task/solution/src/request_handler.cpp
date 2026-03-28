#include "request_handler.h"

#include <boost/json.hpp>

namespace http_handler {

namespace json = boost::json;

/**
 * @brief Сериализует карту в JSON-строку
 * @param map Карта для сериализации
 * @return JSON-строка со всеми полями: id, name, roads, buildings, offices
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
    
    // Сериализуем дороги
    // Пример:
    // В памяти дорога — это объект с полями:
    // Road road{{x0, y0}, {x1, y1}}  // начало и конец
    // После сериализации в JSON:
    // {"x0": 0, "y0": 0, "x1": 10}
    json::array roads;
    for (const auto& road : map.GetRoads()) {
        json::object road_obj;
        road_obj["x0"] = road.GetStart().x;
        road_obj["y0"] = road.GetStart().y;
        
        if (road.IsHorizontal()) {
            road_obj["x1"] = road.GetEnd().x;
        } else {
            road_obj["y1"] = road.GetEnd().y;
        }
        roads.push_back(road_obj);
    }
    result["roads"] = roads;
    
    // Сериализуем здания
    json::array buildings;
    for (const auto& building : map.GetBuildings()) {
        json::object building_obj;
        const auto& bounds = building.GetBounds();
        building_obj["x"] = bounds.position.x;
        building_obj["y"] = bounds.position.y;
        building_obj["w"] = bounds.size.width;
        building_obj["h"] = bounds.size.height;
        buildings.push_back(building_obj);
    }
    result["buildings"] = buildings;
    
    // Сериализуем офисы
    json::array offices;
    for (const auto& office : map.GetOffices()) {
        json::object office_obj;
        office_obj["id"] = *office.GetId();
        office_obj["x"] = office.GetPosition().x;
        office_obj["y"] = office.GetPosition().y;
        office_obj["offsetX"] = office.GetOffset().dx;
        office_obj["offsetY"] = office.GetOffset().dy;
        offices.push_back(office_obj);
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
