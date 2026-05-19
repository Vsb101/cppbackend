#include "game.h"

namespace model {
using namespace std::literals;

void Game::AddMap(const Map& map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_map_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_map_index_.erase(it);
            throw;
        }
    }
}

GameSession* Game::CreateSession(const Map::Id& map_id) {
    //  Найдём карту, к которой хотим подключиться
    auto map = FindMap(map_id);
    if( !map ) {
        throw std::invalid_argument("Map with id "s + *map_id + " isn`t exists"s);
    }

    // Карта есть. Пробуем добавить новую сессию
    const size_t index = sessions_.size();
    if (auto [it, inserted] = map_id_to_session_index_.emplace(map_id, index); !inserted) {
        // Сессия есть для карты с таким Id. Заново создавать нельзя!
        throw std::invalid_argument("Session for map with id "s + *map_id + " already exists"s);
    } else {
        // Создаём новую сессию, привязанную к указанной карте
        GameSession* new_session = new GameSession(*map, &loot_generator_);
        try {
            sessions_.emplace_back(new_session);
        } catch (...) {
            // Не получилось. Откатываем изменения в map_id_to_map_index_
            map_id_to_session_index_.erase(it);
            delete new_session;
            throw;
        }
    }
    return sessions_.back();
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_map_index_.find(id); it != map_id_to_map_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

// Возвращает указатель на игровую сесссию с возможностью изменения
GameSession* Game::FindSession(const Map::Id& id) {
    // В данной реализации одна карта (MapId) соответсвует одной сессии
    // При необходимости можно будет переделать, чтобы на одной карте было несколько сессий.
    // Для этого нужно будет добавить критерий "переполнения" сессии и создания новой, например, по количеству 
    // игроков в текущей найденной сессии
    if (auto it = map_id_to_session_index_.find(id); it != map_id_to_session_index_.end()) {
        return sessions_.at(it->second);
    } else {
        // Если не нашли сессию для игрока, пробуем создать новую
        return CreateSession(id);
    }
}

}  // namespace model
