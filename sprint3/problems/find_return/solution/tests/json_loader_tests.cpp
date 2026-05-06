#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "../src/infra/json_loader.h"
#include "../src/model/loot_generator.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <random>

using namespace std::literals;

// Вспомогательная функция для создания временного JSON файла
std::filesystem::path CreateTempJsonFile(const std::string& content) {
    auto temp_dir = std::filesystem::temp_directory_path();
    // Создаем уникальный имя файла
    static unsigned counter = 0;
    auto temp_file = temp_dir / ("test_config_" + std::to_string(++counter) + ".json");
    
    std::ofstream ofs(temp_file);
    ofs << content;
    ofs.close();
    
    return temp_file;
}

SCENARIO("Load game configuration with loot generator settings") {
    GIVEN("a valid config file with lootGeneratorConfig") {
        std::string config_json = R"({
            "lootGeneratorConfig": {
                "period": 5.0,
                "probability": 0.5
            },
            "defaultDogSpeed": 3.0,
            "maps": [
                {
                    "id": "map1",
                    "name": "Map 1",
                    "lootTypes": [
                        {"name": "key", "file": "assets/key.obj"},
                        {"name": "wallet", "file": "assets/wallet.obj"}
                    ],
                    "roads": [
                        {"x0": 0, "y0": 0, "x1": 40}
                    ],
                    "buildings": [],
                    "offices": []
                }
            ]
        })";
        
        auto temp_path = CreateTempJsonFile(config_json);
        
        WHEN("loading the configuration") {
            auto config = json_loader::LoadGame(temp_path);
            
            THEN("loot generator config is loaded correctly") {
                // Проверяем, что сессия может быть создана с правильным генератором
                auto session = config.game.FindOrCreateSession(model::Map::Id{"map1"s});
                REQUIRE(session != nullptr);
            }
            
            THEN("map has correct loot types count") {
                auto map = config.game.FindMap(model::Map::Id{"map1"s});
                REQUIRE(map != nullptr);
                REQUIRE(map->GetLootTypesCount() == 2);
            }
            
            THEN("extra data contains loot types") {
                auto& loot_types = config.extra_data.GetLootTypes(model::Map::Id{"map1"s});
                REQUIRE(loot_types.size() == 2);
            }
        }
        
        std::filesystem::remove(temp_path);
    }
}

SCENARIO("Load game configuration without lootGeneratorConfig") {
    GIVEN("a config file without lootGeneratorConfig") {
        std::string config_json = R"({
            "defaultDogSpeed": 4.0,
            "maps": [
                {
                    "id": "map1",
                    "name": "Map 1",
                    "lootTypes": [
                        {"name": "key"}
                    ],
                    "roads": [
                        {"x0": 0, "y0": 0, "x1": 40}
                    ],
                    "buildings": [],
                    "offices": []
                }
            ]
        })";
        
        auto temp_path = CreateTempJsonFile(config_json);
        
        WHEN("loading the configuration") {
            auto config = json_loader::LoadGame(temp_path);
            
            THEN("default loot generator config is used") {
                // Должны быть использованы значения по умолчанию
                auto session = config.game.FindOrCreateSession(model::Map::Id{"map1"s});
                REQUIRE(session != nullptr);
            }
        }
        
        std::filesystem::remove(temp_path);
    }
}

SCENARIO("Load game configuration with missing lootTypes") {
    GIVEN("a config file with map missing lootTypes") {
        std::string config_json = R"({
            "maps": [
                {
                    "id": "map1",
                    "name": "Map 1",
                    "roads": [
                        {"x0": 0, "y0": 0, "x1": 40}
                    ],
                    "buildings": [],
                    "offices": []
                }
            ]
        })";
        
        auto temp_path = CreateTempJsonFile(config_json);
        
        WHEN("loading the configuration") {
            THEN("an exception is thrown") {
                REQUIRE_THROWS_WITH(json_loader::LoadGame(temp_path), 
                    Catch::Matchers::ContainsSubstring("must have lootTypes"));
            }
        }
        
        std::filesystem::remove(temp_path);
    }
}

SCENARIO("Load game configuration with empty lootTypes") {
    GIVEN("a config file with empty lootTypes array") {
        std::string config_json = R"({
            "maps": [
                {
                    "id": "map1",
                    "name": "Map 1",
                    "lootTypes": [],
                    "roads": [
                        {"x0": 0, "y0": 0, "x1": 40}
                    ],
                    "buildings": [],
                    "offices": []
                }
            ]
        })";
        
        auto temp_path = CreateTempJsonFile(config_json);
        
        WHEN("loading the configuration") {
            THEN("an exception is thrown") {
                REQUIRE_THROWS_WITH(json_loader::LoadGame(temp_path), 
                    Catch::Matchers::ContainsSubstring("must have at least one loot type"));
            }
        }
        
        std::filesystem::remove(temp_path);
    }
}

SCENARIO("Load game configuration with multiple maps") {
    GIVEN("a config file with multiple maps") {
        std::string config_json = R"({
            "maps": [
                {
                    "id": "map1",
                    "name": "Map 1",
                    "lootTypes": [
                        {"name": "key"},
                        {"name": "wallet"}
                    ],
                    "roads": [{"x0": 0, "y0": 0, "x1": 40}],
                    "buildings": [],
                    "offices": []
                },
                {
                    "id": "map2",
                    "name": "Map 2",
                    "lootTypes": [
                        {"name": "coin"}
                    ],
                    "roads": [{"x0": 0, "y0": 0, "x1": 50}],
                    "buildings": [],
                    "offices": []
                }
            ]
        })";
        
        auto temp_path = CreateTempJsonFile(config_json);
        
        WHEN("loading the configuration") {
            auto config = json_loader::LoadGame(temp_path);
            
            THEN("both maps are loaded") {
                REQUIRE(config.game.FindMap(model::Map::Id{"map1"s}) != nullptr);
                REQUIRE(config.game.FindMap(model::Map::Id{"map2"s}) != nullptr);
            }
            
            THEN("each map has correct loot types count") {
                auto map1 = config.game.FindMap(model::Map::Id{"map1"s});
                auto map2 = config.game.FindMap(model::Map::Id{"map2"s});
                REQUIRE(map1->GetLootTypesCount() == 2);
                REQUIRE(map2->GetLootTypesCount() == 1);
            }
        }
        
        std::filesystem::remove(temp_path);
    }
}
