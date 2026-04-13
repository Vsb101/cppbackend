#pragma once

#include "model.h"
#include <random>
#include <unordered_map>
#include <string>

namespace model {

class PlayerTokens {
public:
    Token GenerateToken();

    // Находит игрока по токену
    Player* FindPlayerByToken(const Token& token);
    const Player* FindPlayerByToken(const Token& token) const;

    // Связывает токен с игроком
    void AddPlayer(Token token, Player* player);

    // Генератор случайных чисел для токенов
    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};

private:
    std::unordered_map<Token, Player*, util::TaggedHasher<Token>> tokens_to_players_;
};

} // namespace model