#pragma once

#include <memory>
#include <random>
#include <string>
#include <unordered_map>

#include "player.h"
#include "../other/tagged.h"

namespace app {

struct TokenTag {};
using Token = util::Tagged<std::string, TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;

class PlayerTokens {
 public:
    PlayerTokens() = default;

    // используем shared_ptr и const ссылки
    Token AddPlayer(std::shared_ptr<Player> player);
    std::shared_ptr<Player> FindPlayerByToken(const Token& token) const;

    // Метод для восстановления состояния
    void RestorePlayer(const Token& token, std::shared_ptr<Player> player);

    void RemovePlayer(std::shared_ptr<Player> player);

    // Методы для сериализации
    [[nodiscard]] const std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher>& GetAllTokens() const;
    void ClearAllTokens();

 private:
    std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> token_to_player_;

    //упрощенная инициализация рандома
    std::random_device random_device_;
    std::mt19937_64 generator_{random_device_()}; 
};

}  // namespace app
