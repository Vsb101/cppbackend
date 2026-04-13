#pragma once

#include "model.h"
#include <string>
#include <random>

namespace model {

class Player {
public:
    Player(PlayerId id, std::string name, const Map* map)
        : id_(id), name_(std::move(name)), map_(map) {}

    PlayerId GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    const Map* GetMap() const { return map_; }

private:
    PlayerId id_;
    std::string name_;
    const Map* map_; // Игрок привязан к карте
};

} // namespace model