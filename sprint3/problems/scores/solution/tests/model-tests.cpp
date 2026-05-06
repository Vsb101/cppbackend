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
        loot_gen::LootGenerator loot_gen{TimeInterval{5000}, 0.5};
        
        model::GameSession session(map, std::move(loot_gen));
        
        WHEN("creating dogs") {
            session.CreateDog("dog1"s);
            session.CreateDog("dog2"s);
            
            THEN("dogs are created") {
                REQUIRE(session.GetDogs().size() == 2);
            }
            
            WHEN("updating with time delta") {
                session.Update(5.0); // 5 секунд
                
                THEN("loot may be generated") {
                    // Количество лутов не должно превышать количество собак
                    REQUIRE(session.GetLostObjects().size() <= 2);
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
            
            THEN("count is in valid range") {
                REQUIRE(count >= 0);
                REQUIRE(count == 7);
            }
        }
    }
}
