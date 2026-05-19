#pragma once

#include "model_geom.h"

#include <cstdint>
#include <string>
#include <vector>

namespace serializer {

struct SerItem {
    std::uint32_t id;
    int type;
    model::Position position;
    int value;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & id;
        ar & type;
        ar & position;
        ar & value;
    }
};

struct SerDog {
    std::uint32_t id;
    std::string name;
    model::Position position;
    model::Speed speed;
    model::Direction direction;
    std::vector<SerItem> bag;
    int score;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & id;
        ar & name;
        ar & position;
        ar & speed;
        ar & direction;
        ar & bag;
        ar & score;
    }
};

struct SerPlayer {
    std::uint32_t id;
    std::string name;
    
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & id;
        ar & name;
    }
};

struct SerPlayers {
    std::vector<SerPlayer> players;
    std::vector<std::string> names;
    std::vector<size_t> indexes;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & players;
        ar & names;
        ar & indexes;
    }
};

struct SerPlayerTokens {
    std::vector<std::string> tokens;
};

}   // namespace serializer