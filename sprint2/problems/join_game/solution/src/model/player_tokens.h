#pragma once
#include <memory>
#include <random>
#include <string>
#include <unordered_map>

#include "player.h"
#include "../other/tagged.h"

namespace model {

struct TokenTag {};
using Token = util::Tagged<std::string, TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;

class PlayerTokens {
 public:
  PlayerTokens() = default;
  PlayerTokens(const PlayerTokens&) = default;
  PlayerTokens(PlayerTokens&&) = default;
  PlayerTokens& operator=(const PlayerTokens&) = default;
  PlayerTokens& operator=(PlayerTokens&&) = default;
  virtual ~PlayerTokens() = default;

  Token AddPlayer(std::weak_ptr<Player> player);
  std::weak_ptr<Player> FindPlayerByToken(Token token);

 private:
  std::unordered_map<Token, std::weak_ptr<Player>, TokenHasher> tokenToPlayer_;

  std::random_device random_device_;
  std::mt19937_64 generator1_{[this] {
    std::uniform_int_distribution<std::mt19937_64::result_type> dist;
    return dist(random_device_);
  }()};
  std::mt19937_64 generator2_{[this] {
    std::uniform_int_distribution<std::mt19937_64::result_type> dist;
    return dist(random_device_);
  }()};
};

}  // namespace model
