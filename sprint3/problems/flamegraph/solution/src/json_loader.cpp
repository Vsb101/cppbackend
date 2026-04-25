#include "json_loader.h"

#include <fstream>
#include <sstream>

namespace json_loader {

namespace json = boost::json;

/**
 * @brief Читает содержимое файла и возвращает его как строку.
 * @param path Путь к файлу
 * @return Содержимое файла
 * @throw std::runtime_error если файл не удалось открыть
 */
std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }
    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

/**
 * @brief Парсит JSON-объект дороги в модель Road.
 * @param road_obj JSON-объект дороги
 * @return Объект Road
 *
 * Горизонтальная дорога (есть x1):
 * { "x0": 0, "y0": 0, "x1": 40 }  -> от (0,0) до (40,0)
 *
 * Вертикальная дорога (есть y1):
 * { "x0": 0, "y0": 0, "y1": 20 }  -> от (0,0) до (0,20)
 */
model::Road ParseRoad(const json::object& road_obj) {
    model::Point start{
        static_cast<model::Coord>(road_obj.at("x0").as_int64()),
        static_cast<model::Coord>(road_obj.at("y0").as_int64())
    };
    if (road_obj.contains("x1")) {
        model::Coord end_x = static_cast<model::Coord>(road_obj.at("x1").as_int64());
        return model::Road(model::Road::HORIZONTAL, start, end_x);
    } else {
        model::Coord end_y = static_cast<model::Coord>(road_obj.at("y1").as_int64());
        return model::Road(model::Road::VERTICAL, start, end_y);
    }
}

/**
 * @brief Парсит JSON-объект здания в модель Building.
 * @param building_obj JSON-объект здания
 * @return Объект Building
 *
 * Формат:
 * { "x": 5, "y": 5, "w": 30, "h": 20 }
 * где x, y - координаты верхнего левого угла, w, h - размеры
 */
model::Building ParseBuilding(const json::object& building_obj) {
    model::Rectangle bounds{
        model::Point{
            static_cast<model::Coord>(building_obj.at("x").as_int64()),
            static_cast<model::Coord>(building_obj.at("y").as_int64())
        },
        model::Size{
            static_cast<model::Dimension>(building_obj.at("w").as_int64()),
            static_cast<model::Dimension>(building_obj.at("h").as_int64())
        }
    };
    return model::Building(bounds);
}

/**
 * @brief Парсит JSON-объект офиса в модель Office.
 * @param office_obj JSON-объект офиса
 * @return Объект Office
 *
 * Формат:
 * { "id": "o0", "x": 40, "y": 30, "offsetX": 5, "offsetY": 0 }
 */
model::Office ParseOffice(const json::object& office_obj) {
    model::Office::Id office_id(std::string(office_obj.at("id").as_string()));
    model::Point position{
        static_cast<model::Coord>(office_obj.at("x").as_int64()),
        static_cast<model::Coord>(office_obj.at("y").as_int64())
    };
    model::Offset offset{
        static_cast<model::Dimension>(office_obj.at("offsetX").as_int64()),
        static_cast<model::Dimension>(office_obj.at("offsetY").as_int64())
    };
    return model::Office(office_id, position, offset);
}

/**
 * @brief Парсит JSON-объект карты в модель Map.
 * @param map_obj JSON-объект карты
 * @return Объект Map
 *
 * Обязательные поля: id, name, roads
 * Опциональные поля: buildings, offices
 */
model::Map ParseMap(const json::object& map_obj) {
    std::string map_id = std::string(map_obj.at("id").as_string());
    std::string map_name = std::string(map_obj.at("name").as_string());
    model::Map map(model::Map::Id(map_id), map_name);

    // Парсим дороги (обязательное поле)
    for (const auto& road_value : map_obj.at("roads").as_array()) {
        map.AddRoad(ParseRoad(road_value.as_object()));
    }

    // Парсим здания (опциональное поле)
    if (map_obj.contains("buildings")) {
        for (const auto& building_value : map_obj.at("buildings").as_array()) {
            map.AddBuilding(ParseBuilding(building_value.as_object()));
        }
    }

    // Парсим офисы (опциональное поле)
    if (map_obj.contains("offices")) {
        for (const auto& office_value : map_obj.at("offices").as_array()) {
            map.AddOffice(ParseOffice(office_value.as_object()));
        }
    }

    return map;
}

/**
 * @brief Главная функция загрузки игры из файла.
 * @param json_path Путь к JSON-файлу конфигурации
 * @return Объект Game с загруженными картами
 *
 * Ожидаемый формат: массив объектов в поле "maps"
 *
 * @throw std::runtime_error при ошибках чтения/парсинга/формата
 */
model::Game LoadGame(const std::filesystem::path& json_path) {
    try {
        std::string json_str = ReadFile(json_path);
        auto value = json::parse(json_str);
        const json::object& obj = value.as_object();
        
        // Проверка наличия обязательного поля "maps"
        auto it = obj.find("maps");
        if (it == obj.end()) {
            throw std::runtime_error("Field 'maps' is missing in JSON config file");
        }

        const json::array& maps_array = it->value().as_array();
        model::Game game;
        
        // Парсим каждую карту
        for (const auto& map_value : maps_array) {
            if (!map_value.is_object()) {
                throw std::runtime_error("Each element in 'maps' array must be a JSON object");
            }
            game.AddMap(ParseMap(map_value.as_object()));
        }

        return game;

    } catch (const std::out_of_range& e) {
        throw std::runtime_error("Missing required JSON field in \"" + json_path.string() + "\": " + e.what());
    } catch (const std::invalid_argument& e) {
        throw std::runtime_error("Invalid JSON value type in \"" + json_path.string() + "\": " + e.what());
    } catch (const std::exception& e) {
        throw std::runtime_error("Error loading game config from \"" + json_path.string() + "\": " + e.what());
    }
}

}  // namespace json_loader
