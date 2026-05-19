#include "player_tokens.h"
#include <iomanip>
#include <sstream>

namespace app {

Token PlayerTokens::AddPlayer(std::shared_ptr<Player> player) {
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

std::shared_ptr<Player> PlayerTokens::FindPlayerByToken(const Token& token) const {
    if (auto it = token_to_player_.find(token); it != token_to_player_.end()) {
        return it->second;
    }
    return nullptr;
}

void PlayerTokens::RestorePlayer(const Token& token, std::shared_ptr<Player> player) {
    token_to_player_[token] = std::move(player);
}

void PlayerTokens::RemovePlayer(std::shared_ptr<Player> player) {
    for (auto it = token_to_player_.begin(); it != token_to_player_.end(); ) {
        if (it->second == player) {
            it = token_to_player_.erase(it);
        } else {
            ++it;
        }
    }
}

const std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher>& PlayerTokens::GetAllTokens() const {
    return token_to_player_;
}

void PlayerTokens::ClearAllTokens() {
    token_to_player_.clear();
}

}  // namespace app
