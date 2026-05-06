#include "json_loader.h"
#include <boost/json.hpp>
#include "../api/json_serialization.h"
#include <fstream>
#include <string>

namespace json_loader {
namespace json = boost::json;

using namespace std::literals;

ProjectConfig LoadGame(const std::filesystem::path& json_path) {
    // Валидация файла
    std::ifstream ifs(json_path);
    if (!ifs) {
        throw std::runtime_error("Can't open " + json_path.string());
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    
    boost::json::error_code ec;
    auto value = json::parse(content, ec);
    if (ec) {
        throw std::runtime_error("JSON parse error: " + ec.message());
    }
    
    if (!value.is_object()) {
        throw std::runtime_error("JSON root must be an object");
    }
    
    auto& root_obj = value.as_object();
    
    if (!root_obj.contains("maps")) {
        throw std::runtime_error("Missing required field 'maps' in JSON");
    }

    ProjectConfig config;

    // Читаем настройки генератора (с дефолтными значениями для защиты от 503)
    if (root_obj.contains("lootGeneratorConfig")) {
        auto& lgc = root_obj.at("lootGeneratorConfig").as_object();
        config.game.SetLootGeneratorConfig({
            lgc.at("period").as_double(),
            lgc.at("probability").as_double()
        });
    } else {
        // Если в конфиге нет настроек генератора, ставим безопасные значения
        config.game.SetLootGeneratorConfig({1.0, 0.1});
    }

    // Скорость по умолчанию
    double default_speed = 1.0;
    if (root_obj.contains("defaultDogSpeed")) {
        default_speed = root_obj.at("defaultDogSpeed").as_double();
    }
    config.game.SetDefaultDogSpeed(default_speed);

    // Вместимость рюкзака по умолчанию
    size_t default_bag_capacity = 3;
    if (root_obj.contains("defaultBagCapacity")) {
        default_bag_capacity = static_cast<size_t>(root_obj.at("defaultBagCapacity").as_int64());
    }
    
    // Читаем карты
    auto maps_it = root_obj.find("maps");
    if (!maps_it->value().is_array()) {
        throw std::runtime_error("Field 'maps' must be an array");
    }
    
    // Устанавливаем дефолтную вместимость для всех карт
    // (будет переопделена для конкретных карт, если указано bagCapacity)
    
    auto& maps_array = maps_it->value().as_array();
    for (const auto& map_jv : maps_array) {
        auto& map_obj = map_jv.as_object();
        
        // Используем value_to для дорог, зданий и офисов
        auto map = json::value_to<model::Map>(map_jv);

        // Установка скорости (карта или дефолт)
        if (map_obj.contains("dogSpeed")) {
            map.SetDogSpeed(map_obj.at("dogSpeed").as_double());
        } else {
            map.SetDogSpeed(default_speed);
        }

        // Установка вместимости рюкзака (карта или дефолт)
        if (map_obj.contains("bagCapacity")) {
            map.SetBagCapacity(static_cast<size_t>(map_obj.at("bagCapacity").as_int64()));
        } else {
            map.SetBagCapacity(default_bag_capacity);
        }

        // Работа с LootTypes (Экстра-данные)
        if (map_obj.contains("lootTypes")) {
            auto& loot_types = map_obj.at("lootTypes").as_array();
            if (loot_types.empty()) {
                throw std::runtime_error("Map '"s + *map.GetId() + "' must have at least one loot type");
            }
            map.SetLootTypesCount(loot_types.size());
            for (size_t i = 0; i < loot_types.size(); ++i) {
                int value = 0;
                if (loot_types[i].as_object().contains("value")) {
                    value = static_cast<int>(loot_types[i].as_object().at("value").as_int64());
                }
                map.SetLootTypeValue(i, value);
            }
            config.extra_data.SaveLootTypes(map.GetId(), loot_types);
        } else {
            throw std::runtime_error("Map '"s + *map.GetId() + "' must have lootTypes field with at least one element");
        }

        config.game.AddMap(std::move(map));
    }

    return config;
}
}  // namespace json_loader
