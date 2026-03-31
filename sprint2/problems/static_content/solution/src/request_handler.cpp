#include "request_handler.h"

#include <boost/asio/dispatch.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_map>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace sys = boost::system;
namespace json = boost::json;
using namespace std::literals;

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
    response.set(http::field::content_type, "text/plain");
    response.body() = "File not found";
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

// Обработка статических файлов

std::string RequestHandler::GetMimeType(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    static const std::unordered_map<std::string, std::string> mime_types = {
        {".htm", "text/html"},
        {".html", "text/html"},
        {".css", "text/css"},
        {".txt", "text/plain"},
        {".js", "text/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpe", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".ico", "image/vnd.microsoft.icon"},
        {".tiff", "image/tiff"},
        {".tif", "image/tiff"},
        {".svg", "image/svg+xml"},
        {".svgz", "image/svg+xml"},
        {".mp3", "audio/mpeg"}
    };

    auto it = mime_types.find(extension);
    if (it != mime_types.end()) {
        return it->second;
    }
    return "application/octet-stream";
}

std::string RequestHandler::UrlDecode(const std::string& str) {
    std::string decoded;
    decoded.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && (i + 2) < str.size()) {
            int high = std::tolower(str[++i]);
            int low = std::tolower(str[++i]);
            high = (high >= 'a') ? high - 'a' + 10 : high - '0';
            low = (low >= 'a') ? low - 'a' + 10 : low - '0';
            decoded += static_cast<char>((high << 4) | low);
        } else if (str[i] == '+') {
            decoded += ' ';
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}

void RequestHandler::HandleStaticFile(const http::request<http::string_body>& req,
                                       std::function<void(http::response<http::string_body>&&)>&& send,
                                       const std::string& target) {
    // URL-декодируем target
    std::string decoded_target = UrlDecode(target);

    // Если путь пустой или заканчивается на /, считаем что запрашивается index.html
    std::filesystem::path requested_path(decoded_target);
    if (decoded_target.empty() || decoded_target.back() == '/' || !requested_path.has_filename()) {
        requested_path /= "index.html";
    }

    // Формируем полный путь к файлу
    std::filesystem::path full_path = static_files_root_ / requested_path;
    
    try {
        // Приводим путь к каноничному виду
        full_path = std::filesystem::weakly_canonical(full_path);
        
        // Проверяем, что путь находится внутри корневого каталога
        if (!IsSubPath(full_path)) {
            send(MakeBadRequestResponse(req));
            return;
        }
        
        // Проверяем, что это файл (не каталог)
        if (std::filesystem::is_directory(full_path)) {
            full_path /= "index.html";
        }
        
        // Проверяем, что это обычный файл
        if (!std::filesystem::is_regular_file(full_path)) {
            send(MakeNotFoundResponse(req));
            return;
        }
        
        // Читаем содержимое файла
        auto content = ReadFileContent(full_path);
        if (!content) {
            send(MakeNotFoundResponse(req));
            return;
        }
        
        // Создаём ответ с содержимым файла
        send(MakeFileResponse(*content, full_path, req));
        
    } catch (const std::filesystem::filesystem_error&) {
        send(MakeNotFoundResponse(req));
    }
}

bool RequestHandler::IsSubPath(const std::filesystem::path& path) const {
    std::filesystem::path full_path = std::filesystem::weakly_canonical(path);
    std::filesystem::path root = std::filesystem::weakly_canonical(static_files_root_);
    
    auto full_str = full_path.string();
    auto root_str = root.string();
    
    return full_str.size() >= root_str.size() && 
           full_str.compare(0, root_str.size(), root_str) == 0;
}

std::optional<std::string> RequestHandler::ReadFileContent(const std::filesystem::path& path) const {
    std::ifstream file(path.string(), std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

http::response<http::string_body> RequestHandler::MakeFileResponse(const std::string& content,
                                                                     const std::filesystem::path& path,
                                                                     const http::request<http::string_body>& req) {
    http::response<http::string_body> response;
    response.result(http::status::ok);
    response.set(http::field::content_type, GetMimeType(path));
    response.body() = content;
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    return response;
}

}  // namespace http_handler
