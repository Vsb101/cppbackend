#include "player_tokens.h"

#include <iomanip>
#include <sstream>

namespace model {

constexpr size_t kHalfTokenSize = 16;

Token PlayerTokens::AddPlayer(std::weak_ptr<Player> player) {
    std::stringstream ss;
    ss << std::setw(kHalfTokenSize) << std::setfill('0') << std::hex << generator1_();
    ss << std::setw(kHalfTokenSize) << std::setfill('0') << std::hex << generator2_();
    Token token{ss.str()};
    token_to_player_[token] = player;
    return token;
}

std::weak_ptr<Player> PlayerTokens::FindPlayerByToken(Token token) {
    auto it = token_to_player_.find(token);
    if (it == token_to_player_.end()) {
        return std::weak_ptr<Player>();
    }
    return it->second;
}

}  // namespace model