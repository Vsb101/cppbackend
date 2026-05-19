#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

#include "../src/model/game.h"
#include "../src/app/app.h"
#include "../src/infra/state_serializer.h"
#include "../src/infra/json_loader.h"

using namespace std::literals;

namespace {

std::filesystem::path CreateTestConfig() {
    auto temp_dir = std::filesystem::temp_directory_path();
    auto config_path = temp_dir / "test_config.json";
    
    std::ofstream ofs(config_path);
    ofs << R"({
        "defaultDogSpeed": 3.0,
        "lootGeneratorConfig": {
            "period": 5.0,
            "probability": 0.5
        },
        "maps": [
            {
                "id": "test_map",
                "name": "Test Map",
                "dogSpeed": 1.0,
                "roads": [
                    {"x0": 0, "y0": 0, "x1": 40},
                    {"x0": 40, "y0": 0, "y1": 30},
                    {"x0": 40, "y0": 30, "x1": 0},
                    {"x0": 0, "y0": 0, "y1": 30}
                ],
                "buildings": [],
                "offices": [],
                "lootTypes": [
                    {"name": "item1", "file": "assets/item1.obj", "type": "obj", "rotation": 0, "color": "#FF0000", "scale": 0.01, "value": 10},
                    {"name": "item2", "file": "assets/item2.obj", "type": "obj", "rotation": 0, "color": "#00FF00", "scale": 0.01, "value": 20}
                ]
            }
        ]
    })";
    ofs.close();
    
    return config_path;
}

} // namespace

SCENARIO("Application state serialization - basic") {
    GIVEN("an application with game state") {
        auto config_path = CreateTestConfig();
        auto config = json_loader::LoadGame(config_path.string());
        
        auto app = std::make_shared<app::Application>(
            std::move(config.game),
            std::move(config.extra_data),
            false,
            false
        );
        
        // Создаем сессию и собаку
        auto& game = const_cast<model::Game&>(app->GetGame());
        auto session = game.FindOrCreateSession(model::Map::Id{"test_map"s});
        REQUIRE(session);
        
        auto dog = session->CreateDog("test_dog"s);
        REQUIRE(dog);
        
        // Двигаем собаку
        dog->SetPosition({10.0, 10.0});
        dog->SetDirection(model::Direction::EAST);
        dog->AddToBag(1, 0);
        dog->AddScore(100);
        
        std::filesystem::path state_file = std::filesystem::temp_directory_path() / "test_state.txt";
        
        WHEN("serializing state") {
            auto listener = std::make_unique<infra::SerializingListener>(
                state_file,
                std::chrono::milliseconds::zero(),
                app.get()
            );
            
            listener->SaveState();
            
            THEN("state file is created") {
                REQUIRE(std::filesystem::exists(state_file));
            }
            
            WHEN("deserializing state to new application") {
                auto config2 = json_loader::LoadGame(config_path.string());
                auto app2 = std::make_shared<app::Application>(
                    std::move(config2.game),
                    std::move(config2.extra_data),
                    false,
                    false
                );
                
                auto listener2 = std::make_unique<infra::SerializingListener>(
                    state_file,
                    std::chrono::milliseconds::zero(),
                    app2.get()
                );
                
                REQUIRE_NOTHROW(listener2->LoadState());
                
                THEN("state is restored") {
                    const auto& sessions = app2->GetGame().GetSessions();
                    REQUIRE(!sessions.empty());
                    
                    auto restored_session = sessions.begin()->second;
                    REQUIRE(restored_session);
                    
                    const auto& dogs = restored_session->GetDogs();
                    REQUIRE(dogs.size() == 1);
                    
                    const auto& restored_dog = dogs[0];
                    REQUIRE(restored_dog->GetName() == "test_dog"s);
                    REQUIRE(restored_dog->GetPosition().x == 10.0);
                    REQUIRE(restored_dog->GetPosition().y == 10.0);
                    REQUIRE(restored_dog->GetDirection() == model::Direction::EAST);
                    REQUIRE(restored_dog->GetBag().size() == 1);
                    REQUIRE(restored_dog->GetScore() == 100);
                }
                
                std::filesystem::remove(state_file);
            }
        }
        
        std::filesystem::remove(config_path);
    }
}

SCENARIO("State listener tick notification") {
    GIVEN("a serializing listener with auto-save period") {
        std::filesystem::path state_file = std::filesystem::temp_directory_path() / "test_autosave.txt";
        
        auto config_path = CreateTestConfig();
        auto config = json_loader::LoadGame(config_path.string());
        auto app = std::make_shared<app::Application>(
            std::move(config.game),
            std::move(config.extra_data),
            false,
            false
        );
        
        auto listener = std::make_unique<infra::SerializingListener>(
            state_file,
            std::chrono::milliseconds(1000), // 1 секунда
            app.get()
        );
        
        WHEN("ticks accumulate to save period") {
            // 2 тика по 500мс = 1000мс == 1000мс (должно сохранить, т.к. >=)
            listener->OnTick(std::chrono::milliseconds(500));
            listener->OnTick(std::chrono::milliseconds(500));
            
            THEN("state is saved after 1000ms total") {
                REQUIRE(std::filesystem::exists(state_file));
            }
            
            std::filesystem::remove(state_file);
        }
        
        std::filesystem::remove(config_path);
    }
}

SCENARIO("State file does not exist") {
    GIVEN("a state file path that doesn't exist") {
        std::filesystem::path state_file = std::filesystem::temp_directory_path() / "nonexistent_state.txt";
        
        if (std::filesystem::exists(state_file)) {
            std::filesystem::remove(state_file);
        }
        
        auto config_path = CreateTestConfig();
        auto config = json_loader::LoadGame(config_path.string());
        auto app = std::make_shared<app::Application>(
            std::move(config.game),
            std::move(config.extra_data),
            false,
            false
        );
        
        auto listener = std::make_unique<infra::SerializingListener>(
            state_file,
            std::chrono::milliseconds::zero(),
            app.get()
        );
        
        WHEN("trying to load state") {
            bool loaded = listener->LoadState();
            
            THEN("LoadState returns false") {
                REQUIRE_FALSE(loaded);
            }
        }
        
        std::filesystem::remove(config_path);
    }
}

SCENARIO("Invalid state file") {
    GIVEN("a corrupted state file") {
        std::filesystem::path state_file = std::filesystem::temp_directory_path() / "corrupted_state.txt";
        
        {
            std::ofstream ofs(state_file);
            ofs << "not a valid serialized state";
        }
        
        auto config_path = CreateTestConfig();
        auto config = json_loader::LoadGame(config_path.string());
        auto app = std::make_shared<app::Application>(
            std::move(config.game),
            std::move(config.extra_data),
            false,
            false
        );
        
        auto listener = std::make_unique<infra::SerializingListener>(
            state_file,
            std::chrono::milliseconds::zero(),
            app.get()
        );
        
        WHEN("trying to load corrupted state") {
            THEN("LoadState throws exception") {
                REQUIRE_THROWS(listener->LoadState());
            }
        }
        
        std::filesystem::remove(state_file);
        std::filesystem::remove(config_path);
    }
}

SCENARIO("Atomic save with temp file") {
    GIVEN("a state that will be saved") {
        std::filesystem::path state_file = std::filesystem::temp_directory_path() / "atomic_state.txt";
        
        auto config_path = CreateTestConfig();
        auto config = json_loader::LoadGame(config_path.string());
        
        auto app = std::make_shared<app::Application>(
            std::move(config.game),
            std::move(config.extra_data),
            false,
            false
        );
        
        // Создаем состояние
        auto& game = const_cast<model::Game&>(app->GetGame());
        auto session = game.FindOrCreateSession(model::Map::Id{"test_map"s});
        auto dog = session->CreateDog("dog1"s);
        (void)dog; // suppress unused warning
        
        auto listener = std::make_unique<infra::SerializingListener>(
            state_file,
            std::chrono::milliseconds::zero(),
            app.get()
        );
        
        WHEN("saving state") {
            listener->SaveState();
            
            THEN("only the final file exists, no temp file") {
                REQUIRE(std::filesystem::exists(state_file));
                REQUIRE_FALSE(std::filesystem::exists(state_file.string() + ".tmp"));
            }
            
            std::filesystem::remove(state_file);
        }
        
        std::filesystem::remove(config_path);
    }
}
