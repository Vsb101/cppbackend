#include "player_tokens.h"

#include <iomanip>
#include <sstream>

namespace model {

    const size_t HALF_TOKEN_SIZE = 16;

    Token PlayerTokens::AddPlayer(std::weak_ptr<model::Player> player) {
        std::stringstream ss;
        ss << std::setw(HALF_TOKEN_SIZE) << std::setfill('0') << std::hex << generator1_();
        ss << std::setw(HALF_TOKEN_SIZE) << std::setfill('0') << std::hex << generator2_();
        Token token{ ss.str() };
        tokenToPlayer_[token] = player;
        return token;
    };

    std::weak_ptr<model::Player> PlayerTokens::FindPlayerByToken(Token token) {
        if (!tokenToPlayer_.contains(token)) {
            return std::weak_ptr<model::Player>();
        }
        return tokenToPlayer_[token];
    };

}