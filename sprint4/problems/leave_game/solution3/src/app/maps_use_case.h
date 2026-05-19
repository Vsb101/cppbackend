#pragma once

#include "game.h"

namespace app {

class ListMapsUseCase {
public:
    // Связываем модель использования с игрой
    ListMapsUseCase(model::Game& game) 
        : game_{&game} {
    }

    // Получаем список карт
    const model::Game::Maps& GetMaps() {
        return game_->GetMaps();
    }

private:
    model::Game* game_;
};

class GetMapUseCase {
public:
    // Связываем модель использования с игрой
    GetMapUseCase(model::Game& game) 
        : game_{&game} {
    }

    // Получаем карту по её Id
    const model::Map& GetMap(const model::Map::Id &id) {
        return *game_->FindMap(id);
    }

private:
    model::Game* game_;
};

}  // namespace app
