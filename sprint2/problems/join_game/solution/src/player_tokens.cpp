#include "player_tokens.h"
#include <iomanip>
#include <sstream>

namespace model {

Token PlayerTokens::GenerateToken() {
    // Генерируем два 64-битных случайных числа
    uint64_t part1 = generator1_();
    uint64_t part2 = generator2_();
    
    // Преобразуем в hex-строки и объединяем
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(16) << part1
        << std::setw(16) << part2;
    return Token(oss.str());
}

Player* PlayerTokens::FindPlayerByToken(const Token& token) {
    auto it = tokens_to_players_.find(token);
    return it != tokens_to_players_.end() ? it->second : nullptr;
}

const Player* PlayerTokens::FindPlayerByToken(const Token& token) const {
    auto it = tokens_to_players_.find(token);
    return it != tokens_to_players_.end() ? it->second : nullptr;
}

void PlayerTokens::AddPlayer(Token token, Player* player) {
    tokens_to_players_.emplace(std::move(token), player);
}

} // namespace model