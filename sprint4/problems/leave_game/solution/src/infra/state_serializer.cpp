#include "state_serializer.h"
#include <sstream>
#include <stdexcept>
#include <string>

namespace infra {

SerializingListener::SerializingListener(const std::filesystem::path& state_file_path,
                                          std::chrono::milliseconds save_period,
                                          app::Application* app)
    : state_file_path_(state_file_path)
    , save_period_(save_period)
    , app_(app) {
}

bool SerializingListener::LoadState() {
    if (!std::filesystem::exists(state_file_path_)) {
        return false;
    }
    
    try {
        std::ifstream ifs(state_file_path_, std::ios::binary);
        if (!ifs.is_open()) {
            throw std::runtime_error("Failed to open state file for reading: " + state_file_path_.string());
        }
        
        boost::archive::text_iarchive archive(ifs);
        ApplicationState state;
        archive >> state;
        
        RestoreState(state);
        return true;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load state from file: " + std::string(e.what()));
    }
}

void SerializingListener::SaveState() {
    DoSaveState();
}

void SerializingListener::OnTick(std::chrono::milliseconds delta) {
    // Если период сохранения не задан (ноль или отрицательный), то не делаем автосохранение
    if (save_period_ <= std::chrono::milliseconds::zero()) {
        return;
    }
    
    time_since_save_ += delta;
    
    if (time_since_save_ >= save_period_) {
        DoSaveState();
        time_since_save_ = std::chrono::milliseconds::zero();
    }
}

void SerializingListener::OnShutdown() {
    SaveState();
}

void SerializingListener::DoSaveState() {
    try {
        ApplicationState state = CaptureState();
        
        // Записываем во временный файл
        std::filesystem::path temp_path = state_file_path_.string() + ".tmp";
        std::ofstream ofs(temp_path, std::ios::binary);
        if (!ofs.is_open()) {
            throw std::runtime_error("Failed to open temp state file for writing: " + temp_path.string());
        }
        
        boost::archive::text_oarchive archive(ofs);
        archive << state;
        ofs.close();
        
        // Атомарно переименовываем временный файл в целевой
        std::filesystem::rename(temp_path, state_file_path_);
    } catch (const std::exception& e) {
        // Логируем ошибку, но не бросаем исключение - сервер должен продолжить работу
    }
}

ApplicationState SerializingListener::CaptureState() {
    ApplicationState state;
    
    const auto& game = app_->GetGame();
    
    // Сохраняем все сессии
    for (const auto& [map_id, session] : game.GetSessions()) {
        if (!session) continue;
        
        ApplicationState::SessionData session_data;
        session_data.map_id = *map_id;  // Распаковываем Tagged
        session_data.next_dog_id = session->GetNextDogId();
        session_data.next_loot_id = session->GetNextLootId();
        
        // Сохраняем всех собак в сессии
        for (const auto& dog_ptr : session->GetDogs()) {
            if (dog_ptr) {
                session_data.dogs.push_back(*dog_ptr);
            }
        }
        
        // Сохраняем потерянные объекты
        session_data.lost_objects = session->GetLostObjects();
        
        state.sessions.push_back(std::move(session_data));
    }
    
    // Сохраняем игроков и их токены
    const auto& tokens = app_->GetPlayerTokens();
    for (const auto& [token, player] : tokens.GetAllTokens()) {
        if (!player) continue;
        
        ApplicationState::PlayerData player_data;
        player_data.token = *token;  // Сохраняем сам токен
        player_data.player_id = *player->GetId();  // Распаковываем Tagged
        player_data.player_name = player->GetName();
        
        auto session = player->GetSession();
        if (session) {
            player_data.map_id = *session->GetId();  // Распаковываем Tagged
        }
        
        auto dog = player->GetDog();
        if (dog) {
            player_data.dog_id = *dog->GetId();  // Распаковываем Tagged
        }
        
        state.players.push_back(player_data);
    }
    
    state.next_player_id = app_->GetNextPlayerId();
    state.current_game_time = app_->GetCurrentGameTime();
    
    return state;
}

void SerializingListener::RestoreState(const ApplicationState& state) {
    // Сначала очищаем текущее состояние
    app_->ClearAllPlayers();
    
    // Восстанавливаем сессии
    for (const auto& session_data : state.sessions) {
        // Находим или создаем сессию по map_id
        model::Map::Id map_id{session_data.map_id};  // Конвертируем string в Tagged
        auto& game = const_cast<model::Game&>(app_->GetGame());
        auto game_session = game.FindOrCreateSession(map_id);
        if (!game_session) continue;
        
        // Восстанавливаем собак
        game_session->RestoreDogs(session_data.dogs, session_data.next_dog_id);
        
        // Восстанавливаем потерянные объекты
        game_session->RestoreLostObjects(session_data.lost_objects, session_data.next_loot_id);
    }
    
    // Восстанавливаем игроков и их токены
    for (const auto& player_data : state.players) {
        // Находим сессию
        model::Map::Id map_id{player_data.map_id};  // Конвертируем string в Tagged
        auto& game = const_cast<model::Game&>(app_->GetGame());
        auto game_session = game.FindOrCreateSession(map_id);
        if (!game_session) continue;
        
        // Находим собаку по ID
        std::shared_ptr<model::Dog> restored_dog;
        model::Dog::Id dog_id{player_data.dog_id};  // Конвертируем size_t в Tagged
        for (const auto& dog_ptr : game_session->GetDogs()) {
            if (dog_ptr && dog_ptr->GetId() == dog_id) {
                restored_dog = dog_ptr;
                break;
            }
        }
        
        if (!restored_dog) continue;
        
        // Создаем игрока
        using PlayerId = util::Tagged<size_t, app::Player>;
        auto player = std::make_shared<app::Player>(PlayerId{player_data.player_id}, player_data.player_name);
        player->SetGameSession(game_session);
        player->SetDog(restored_dog);
        
        // Восстанавливаем токен
        app::Token token{player_data.token};
        app_->RestorePlayerToken(token, player);
    }
    
    // Восстанавливаем next_player_id и game time
    app_->SetNextPlayerId(state.next_player_id);
    app_->SetCurrentGameTime(state.current_game_time);
}

} // namespace infra
