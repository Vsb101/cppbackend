#pragma once

#include <string>
#include <filesystem>
#include <chrono>
#include <memory>
#include <fstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/string.hpp>

#include "../model/game_session.h"
#include "../app/app.h"
#include "state_listener.h"

namespace infra {

// Полное состояние приложения для сериализации
struct ApplicationState {
    // Состояние сессий: map_id -> данные сессии
    struct SessionData {
        std::string map_id;
        std::vector<model::Dog> dogs;  // Сериализуем собак напрямую
        std::unordered_map<uint32_t, model::LostObject> lost_objects;
        uint32_t next_dog_id;
        uint32_t next_loot_id;
        
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar& map_id;
            ar& dogs;
            ar& lost_objects;
            ar& next_dog_id;
            ar& next_loot_id;
        }
    };
    
    std::vector<SessionData> sessions;
    
    // Состояние игроков: token -> (player_id, player_name)
    struct PlayerData {
        std::string token;
        size_t player_id;
        std::string player_name;
        std::string map_id;  // ID сессии игрока
        size_t dog_id;       // ID собаки игрока
        
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar& token;
            ar& player_id;
            ar& player_name;
            ar& map_id;
            ar& dog_id;
        }
    };
    
    std::vector<PlayerData> players;
    size_t next_player_id;
    
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar& sessions;
        ar& players;
        ar& next_player_id;
    }
};

// Слушатель, который сохраняет состояние приложения в файл
class SerializingListener : public StateListener {
public:
    SerializingListener(const std::filesystem::path& state_file_path, 
                        std::chrono::milliseconds save_period,
                        app::Application* app);
    
    // Загрузить состояние из файла при старте
    // Возвращает true при успехе, false если файл не существует
    // Бросает исключение при ошибке чтения/парсинга
    bool LoadState();
    
    // Сохранить состояние в файл
    void SaveState();
    
    // Реализация интерфейса StateListener
    void OnTick(std::chrono::milliseconds delta) override;
    void OnShutdown() override;
    
private:
    void DoSaveState();
    ApplicationState CaptureState();
    void RestoreState(const ApplicationState& state);
    
    std::filesystem::path state_file_path_;
    std::chrono::milliseconds save_period_;
    app::Application* app_;
    std::chrono::milliseconds time_since_save_{0};
};

} // namespace infra
