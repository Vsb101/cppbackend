#pragma once

#include <filesystem>
#include <string>

#include <boost/json/fwd.hpp>
#include <boost/json.hpp>

#include "model.h"

namespace json_loader {

/**
 * @brief Загружает игру из JSON-файла конфигурации.
 * @param json_path Путь к JSON-файлу с конфигурацией игры
 * @return Объект Game с загруженными картами
 * @throw std::runtime_error если файл не найден, недоступен или произошла ошибка парсинга
 * @throw std::invalid_argument при некорректном формате JSON
 * @throw std::out_of_range если ожидаемое поле отсутствует в JSON
 *
 * Ожидаемый формат JSON:
 * @code
 * {
 *   "maps": [
 *     {
 *       "id": "map1",
 *       "name": "First Map",
 *       "roads": [...],
 *       "buildings": [...],
 *       "offices": [...]
 *     }
 *   ]
 * }
 * @endcode
 */
model::Game LoadGame(const std::filesystem::path& json_path);

/**
 * @brief Читает содержимое файла в строку.
 * @param path Путь к файлу
 * @return Содержимое файла как строка
 * @throw std::runtime_error если файл не удалось открыть
 */
std::string ReadFile(const std::filesystem::path& path);

/**
 * @brief Парсит JSON-объект карты в модель Map.
 * @param map_obj JSON-объект с описанием карты
 * @return Объект Map
 *
 * Ожидаемые поля:
 * - id: строка (идентификатор карты)
 * - name: строка (название карты)
 * - roads: массив дорог (обязательно)
 * - buildings: массив зданий (опционально)
 * - offices: массив офисов (опционально)
 */
model::Map ParseMap(const boost::json::object& map_obj);

/**
 * @brief Парсит JSON-объект дороги в модель Road.
 * @param road_obj JSON-объект с описанием дороги
 * @return Объект Road
 *
 * Горизонтальная дорога:
 * @code
 * { "x0": 0, "y0": 0, "x1": 10 }
 * @endcode
 *
 * Вертикальная дорога:
 * @code
 * { "x0": 0, "y0": 0, "y1": 5 }
 * @endcode
 */
model::Road ParseRoad(const boost::json::object& road_obj);

/**
 * @brief Парсит JSON-объект здания в модель Building.
 * @param building_obj JSON-объект с описанием здания
 * @return Объект Building
 *
 * Ожидаемые поля:
 * @code
 * { "x": 5, "y": 10, "w": 30, "h": 20 }
 * @endcode
 * где x, y - координаты верхнего левого угла, w, h - ширина и высота.
 */
model::Building ParseBuilding(const boost::json::object& building_obj);

/**
 * @brief Парсит JSON-объект офиса в модель Office.
 * @param office_obj JSON-объект с описанием офиса
 * @return Объект Office
 *
 * Ожидаемые поля:
 * @code
 * { "id": "o1", "x": 100, "y": 200, "offsetX": 5, "offsetY": -10 }
 * @endcode
 */
model::Office ParseOffice(const boost::json::object& office_obj);

}  // namespace json_loader
