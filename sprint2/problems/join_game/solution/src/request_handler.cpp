#include "request_handler.h"

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
namespace json = boost::json;
using namespace std::literals;

// === СЕРИАЛИЗАЦИЯ В JSON ===

std::string MapToJson(const model::Map& map) {
    json::object result;

    // Основные поля карты
    result["id"] = *map.GetId();      // Указатель на Id → разыменовываем
    result["name"] = map.GetName();   // Просто строка

    // === Дороги ===
    // Дорога может быть горизонтальной (x меняется) или вертикальной (y меняется)
    // Поэтому в JSON:
    // - Горизонтальная: {x0, y0, x1}
    // - Вертикальная:   {x0, y0, y1}
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
        roads.push_back(std::move(road_obj));
    }
    result["roads"] = std::move(roads);

    // === Здания ===
    // Каждое здание имеет прямоугольную область (Bounds): позиция и размер
    json::array buildings;
    for (const auto& building : map.GetBuildings()) {
        json::object building_obj;
        const auto& bounds = building.GetBounds();
        building_obj["x"] = bounds.position.x;
        building_obj["y"] = bounds.position.y;
        building_obj["w"] = bounds.size.width;
        building_obj["h"] = bounds.size.height;
        buildings.push_back(std::move(building_obj));
    }
    result["buildings"] = std::move(buildings);

    // === Офисы ===
    // Офисы используются для размещения игроков.
    // offsetX/Y — смещение метки на карте относительно точки (x,y)
    json::array offices;
    for (const auto& office : map.GetOffices()) {
        json::object office_obj;
        office_obj["id"] = *office.GetId();
        office_obj["x"] = office.GetPosition().x;
        office_obj["y"] = office.GetPosition().y;
        office_obj["offsetX"] = office.GetOffset().dx;
        office_obj["offsetY"] = office.GetOffset().dy;
        offices.push_back(std::move(office_obj));
    }
    result["offices"] = std::move(offices);

    // Сериализуем весь объект в строку
    return json::serialize(result);
}

std::string MapsListToJson(const model::Game& game) {
    json::array maps_array;

    // Для каждого элемента в списке карт создаём объект {id, name}
    for (const auto& map : game.GetMaps()) {
        json::object map_obj;
        map_obj["id"] = *map.GetId();
        map_obj["name"] = map.GetName();
        maps_array.push_back(std::move(map_obj));
    }

    return json::serialize(maps_array);
}

RequestHandler::RequestHandler(model::Game& game, const std::filesystem::path& static_files_root, Strand api_strand)
    : game_(game)
    , static_files_root_(static_files_root)
    , api_strand_(std::move(api_strand))
    , api_handler_(std::make_unique<api_handler::ApiHandler>(game, sessions_)) {
}

// === ОБРАБОТКА API ===

bool RequestHandler::IsApiRequest(std::string_view target) {
    return target.rfind("api/", 0) == 0;
}

http::response<http::string_body> RequestHandler::HandleApiMaps(
    const http::request<http::string_body>& req,
    std::string_view path_suffix) {
    // Пустой суффикс или "/" → список всех карт
    if (path_suffix.empty() || path_suffix == "/") {
        return HandleMapsList(req);
    }
    // Начинается с '/' → /{id}
    else if (path_suffix[0] == '/') {
        return HandleMapById(req, path_suffix.substr(1));
    }
    // Некорректный путь
    return MakeBadRequestResponse(req);
}

void RequestHandler::InvalidateCache() {
    map_json_cache_.clear();
    maps_list_cache_valid_ = false;
}

http::response<http::string_body> RequestHandler::HandleMapsList(
    const http::request<http::string_body>& req) {
    if (!maps_list_cache_valid_) {
        maps_list_json_ = MapsListToJson(game_);
        maps_list_cache_valid_ = true;
    }
    return MakeResponse(http::status::ok, maps_list_json_, "application/json", req);
}

http::response<http::string_body> RequestHandler::HandleMapById(
    const http::request<http::string_body>& req,
    std::string_view map_id) {
    // Преобразуем string_view в строку для Id
    const model::Map* map = game_.FindMap(model::Map::Id{std::string(map_id)});
    if (!map) {
        return MakeResponse(
            http::status::not_found,
            R"({"code": "mapNotFound", "message": "Map not found"})"sv,
            "application/json",
            req);
    }

    // Проверяем кэш
    std::string map_id_str(map_id);
    auto it = map_json_cache_.find(map_id_str);
    if (it == map_json_cache_.end()) {
        // Кэшируем результат
        it = map_json_cache_.emplace(map_id_str, MapToJson(*map)).first;
    }
    
    return MakeResponse(http::status::ok, it->second, "application/json", req);
}

// === ОБРАБОТКА СТАТИКИ ===

std::string RequestHandler::GetMimeType(std::string_view path) {
    std::string extension;
    auto pos = path.find_last_of('.');
    if (pos != std::string_view::npos) {
        extension = std::string(path.substr(pos));
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char c) { return std::tolower(c); });
    }

    static const std::unordered_map<std::string_view, std::string_view> mime_types = {
        {".htm", "text/html"},       {".html", "text/html"},
        {".css", "text/css"},        {".txt", "text/plain"},
        {".js", "text/javascript"},  {".json", "application/json"},
        {".xml", "application/xml"}, {".png", "image/png"},
        {".jpg", "image/jpeg"},      {".jpe", "image/jpeg"},
        {".jpeg", "image/jpeg"},     {".gif", "image/gif"},
        {".bmp", "image/bmp"},       {".ico", "image/vnd.microsoft.icon"},
        {".tiff", "image/tiff"},     {".tif", "image/tiff"},
        {".svg", "image/svg+xml"},   {".svgz", "image/svg+xml"},
        {".mp3", "audio/mpeg"}
    };

    auto it = mime_types.find(extension);
    if (it != mime_types.end()) {
        return std::string(it->second);
    }
    return "application/octet-stream";  // бинарные данные по умолчанию
}

std::string RequestHandler::UrlDecode(std::string_view str) {
    std::string decoded;
    decoded.reserve(str.size());  // оптимизация: избегаем реаллокаций

    auto from_hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1; // невалидный символ
    };

    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];
        if (c == '+') {
            decoded += ' ';
        } else if (c == '%' && i + 2 < str.size()) {
            // Читаем два следующих символа
            int hi = from_hex(str[i + 1]);
            int lo = from_hex(str[i + 2]);
            if (hi != -1 && lo != -1) {
                decoded += static_cast<char>((hi << 4) | lo);
                i += 2; // пропускаем два символа после '%'
                continue;
            }
            // Если hex-код некорректен — трактуем буквально
            decoded += c;
        } else {
            decoded += c;
        }
    }

    return decoded;
}

http::response<http::string_body> RequestHandler::HandleStaticFile(
    const http::request<http::string_body>& req,
    std::string_view target) {
    
    std::string decoded_target = UrlDecode(target);
    std::filesystem::path requested_path(decoded_target);

    // Если путь пустой или заканчивается на '/', считаем, что нужен index.html
    if (decoded_target.empty() || decoded_target.back() == '/' || !requested_path.has_filename()) {
        requested_path /= "index.html";
    }

    std::filesystem::path full_path = static_files_root_ / requested_path;

    try {
        // Приводим путь к каноничному виду (разрешаем ., ..)
        full_path = std::filesystem::weakly_canonical(full_path);

        // Защита от path traversal: проверяем, что путь внутри корня
        if (!IsSubPath(full_path)) {
            return MakeBadRequestResponse(req);
        }

        // Если это директория — добавляем index.html
        if (std::filesystem::is_directory(full_path)) {
            full_path /= "index.html";
        }

        // Только обычные файлы можно отдавать
        if (!std::filesystem::is_regular_file(full_path)) {
            return MakeNotFoundResponse(req);
        }

        // Читаем содержимое
        if (auto content = ReadFileContent(full_path)) {
            return MakeFileResponse(*content, full_path, req);
        } else {
            return MakeNotFoundResponse(req);
        }

    } catch (const std::filesystem::filesystem_error&) {
        return MakeNotFoundResponse(req);
    }
}

bool RequestHandler::IsSubPath(const std::filesystem::path& path) const {
    std::error_code ec;
    std::filesystem::path full = std::filesystem::weakly_canonical(path, ec);
    if (ec) return false;  // ошибка при нормализации

    std::filesystem::path root = std::filesystem::weakly_canonical(static_files_root_, ec);
    if (ec) return false;

    std::string full_str = full.string();
    std::string root_str = root.string();

    // Проверяем, что full начинается с root
    return full_str.size() >= root_str.size() &&
           full_str.compare(0, root_str.size(), root_str) == 0;
}

std::optional<std::string> RequestHandler::ReadFileContent(const std::filesystem::path& path) const {
    std::ifstream file(path.string(), std::ios::binary);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();  // читаем весь файл
    return buffer.str();
}

http::response<http::string_body> RequestHandler::MakeFileResponse(
    const std::string& content,
    const std::filesystem::path& path,
    const http::request<http::string_body>& req) {
    return MakeResponse(http::status::ok, content, GetMimeType(path.string()), req);
}

// === ОТВЕТЫ НА ОШИБКИ ===

http::response<http::string_body> RequestHandler::MakeBadRequestResponse(
    const http::request<http::string_body>& req) {
    return MakeResponse(
        http::status::bad_request,
        R"({"code": "badRequest", "message": "Bad request"})"sv,
        "application/json",
        req);
}

http::response<http::string_body> RequestHandler::MakeNotFoundResponse(
    const http::request<http::string_body>& req) {
    return MakeResponse(
        http::status::not_found,
        "File not found"sv,
        "text/plain",
        req);
}

http::response<http::string_body> RequestHandler::MakeMethodNotAllowedResponse(
    const http::request<http::string_body>& req) {
    auto response = MakeResponse(
        http::status::method_not_allowed,
        R"({"code": "badRequest", "message": "Bad request"})"sv,
        "application/json",
        req);
    response.set(http::field::allow, "GET, HEAD");  // обязательный заголовок
    return response;
}

}  // namespace http_handler