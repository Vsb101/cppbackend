#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "../src/model/map.h"
#include "../src/model/game.h"
#include "../src/model/game_session.h"
#include "../src/model/loot_generator.h"

using namespace std::literals;

SCENARIO("Map creation and management") {
    GIVEN("a new map") {
        model::Map map{model::Map::Id{"map1"s}, "Test Map"s};
        
        WHEN("adding roads") {
            map.AddRoad(model::Road{model::Road::HORIZONTAL, {0, 0}, 40});
            map.AddRoad(model::Road{model::Road::VERTICAL, {40, 0}, 30});
            
            THEN("roads are stored correctly") {
                REQUIRE(map.GetRoads().size() == 2);
            }
        }

        WHEN("adding buildings") {
            map.AddBuilding(model::Building{{{5, 5}, {30, 20}}});
            
            THEN("buildings are stored correctly") {
                REQUIRE(map.GetBuildings().size() == 1);
            }
        }

        WHEN("setting loot types count") {
            map.SetLootTypesCount(5);
            
            THEN("loot types count is returned correctly") {
                REQUIRE(map.GetLootTypesCount() == 5);
            }
        }
    }
}

SCENARIO("Game session with loot generation") {
    GIVEN("a game session with loot generator") {
        auto map = std::make_shared<model::Map>(model::Map::Id{"test_map"s}, "Test Map"s);
        map->AddRoad(model::Road{model::Road::HORIZONTAL, {0, 0}, 40});
        map->AddRoad(model::Road{model::Road::VERTICAL, {40, 0}, 30});
        map->AddRoad(model::Road{model::Road::HORIZONTAL, {40, 30}, 0});
        map->AddRoad(model::Road{model::Road::VERTICAL, {0, 0}, 30});
        map->SetLootTypesCount(2);
        
        using TimeInterval = loot_gen::LootGenerator::TimeInterval;
        // Устанавливаем вероятность 1.0 для детерминированности теста
        loot_gen::LootGenerator loot_gen{TimeInterval{5000}, 1.0};
        
        model::GameSession session(map, std::move(loot_gen));
        
        WHEN("creating dogs") {
            auto dog1 = session.CreateDog("dog1"s);
            auto dog2 = session.CreateDog("dog2"s);
            
            THEN("dogs are created") {
                REQUIRE(session.GetDogs().size() == 2);
            }
            
            WHEN("updating with time delta") {
                // Передаем 3 аргумента (dt, дефолтный лимит 60.0 секунд и пустой callback-заглушку)
                session.Update(5.0, 60.0, [](const std::shared_ptr<model::Dog>&) {});
                
                THEN("loot is generated and does not exceed dog count") {
                    auto lost_objects_count = session.GetLostObjects().size();
                    REQUIRE(lost_objects_count > 0);
                    REQUIRE(lost_objects_count <= 2);
                }
            }
        }
    }
}

SCENARIO("Loot types count validation") {
    GIVEN("a map with loot types") {
        model::Map map{model::Map::Id{"map1"s}, "Map 1"s};
        map.SetLootTypesCount(7);
        
        WHEN("getting loot types count") {
            size_t count = map.GetLootTypesCount();
            
            THEN("count is correct") {
                // REQUIRE(count >= 0) убран, чтобы избежать warning-ов компилятора на беззнаковом size_t
                REQUIRE(count == 7);
            }
        }
    }
}
