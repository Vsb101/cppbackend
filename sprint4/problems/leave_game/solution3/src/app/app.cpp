#include "app.h"
#include "players.h"

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <fstream>

namespace app {

model::Game::Maps Application::ListMaps() {
    return list_maps_.GetMaps();
}

const model::Map& Application::FindMap(std::string map_id) {
    return get_map_.GetMap(model::Map::Id(map_id));
}

JoinGameResult Application::JoinGame(std::string user_name, std::string map_id) {
    return join_game_.JoinGame(model::Map::Id{map_id}, Player::Name{user_name});
}

ListPlayersResult Application::GetPlayers(std::string_view token) {
    return list_players_.GetPlayers(app::Token(std::string(token)));
}

GetStateResult Application::GetState(std::string_view token) {
    return game_state_.GetState(app::Token(std::string(token)));
}

PlayerActionResult Application::ExecutePlayerAction(std::string_view token, PlayerAction action) {
    return player_action_.ExecutePlayerAction(app::Token(std::string(token)), action);
}

TickResult Application::ExecuteTick(Tick tick) {
    // Выполняем один шаг по времени
    auto tick_res = tick_.ExecuteTick(tick);

    // Удаляем неактивных игроков с сохранением их достижений в БД
    SaveRetirementPlayers();
    
    // Уведомляем подписчиков сигнала tick
    tick_signal_(tick.GetTimeDelta());

    return tick_res;
}

AddPlayerResult Application::AddPlayer(const serialization::PlayerRepr& player, const Token& token) {
    return add_player_.AddPlayer(player, token);
}

RecordsResult Application::GetRecords(size_t start, size_t limit) {
    return records_use_case_.GetRecords({start, limit});
}

void Application::SaveRetirementPlayers() {
    // Проверяем игроков на бездействие
    for (auto player : players_.GetPlayers()) {
        auto sleep_time = player->GetSleepTime();
        if (sleep_time >= retirement_time_) {
            // Сохраняем рекорд
            auto name = *player->GetName();
            auto play_time = player->GetPlayTime();
            auto score = player->GetDog().GetScore();
            db_use_cases_.AddPlayer({name, score, play_time});
            // Удаляем игрока
            players_.RemovePlayer(player->GetId());
            // Удаляем токен авторизации
            tokens_.RemovePlayer(player.get());
        }
    }

}


}  // namespace app
