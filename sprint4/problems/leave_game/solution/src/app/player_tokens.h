#pragma once

#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "player.h"
#include "../other/tagged.h"

namespace app {

struct TokenTag {};
using Token = util::Tagged<std::string, TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;

class PlayerTokens {
 public:
    PlayerTokens() = default;

    // Генерация токена и добавление игрока
    Token AddPlayer(std::shared_ptr<Player> player) {
        auto generate_hex = [this](uint64_t val) {
            std::stringstream ss;
            ss << std::setw(16) << std::setfill('0') << std::hex << val;
            return ss.str();
        };

        std::string t = generate_hex(generator_()) + generate_hex(generator_());
        Token token{t};
        
        token_to_player_[token] = std::move(player);
        return token;
    }

    // Поиск игрока по токену
    std::shared_ptr<Player> FindPlayerByToken(const Token& token) const {
        if (auto it = token_to_player_.find(token); it != token_to_player_.end()) {
            return it->second;
        }
        return nullptr;
    }

    // НОВЫЙ МЕТОД: Удаление игрока и аннулирование его токена при выходе на пенсию
    void RemovePlayer(const std::shared_ptr<Player>& player) {
        if (!player) return;
        
        auto it = std::find_if(token_to_player_.begin(), token_to_player_.end(),
            [&player](const auto& pair) {
                return pair.second == player;
            });
            
        if (it != token_to_player_.end()) {
            token_to_player_.erase(it);
        }
    }

    // Восстановление состояния
    void RestorePlayer(const Token& token, std::shared_ptr<Player> player) {
        token_to_player_[token] = std::move(player);
    }

    // Получение всех токенов для сериализации
    [[nodiscard]] const std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher>& GetAllTokens() const {
        return token_to_player_;
    }

    // Полная очистка
    void ClearAllTokens() {
        token_to_player_.clear();
    }

 private:
    std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> token_to_player_;

    std::random_device random_device_;
    std::mt19937_64 generator_{random_device_()}; 
};

}  // namespace app
