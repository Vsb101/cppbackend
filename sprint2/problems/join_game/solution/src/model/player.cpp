#include "player.h"

namespace model {

void Player::SetGameSession(std::weak_ptr<GameSession> session) { session_ = session; };

void Player::SetDog(std::weak_ptr<Dog> dog) { dog_ = dog; };

const Player::Id& Player::GetId() const { return id_; };

const std::string& Player::GetName() const { return name_; };

const model::GameSession::Id& Player::GetSessionId() const {
  return session_.lock()->GetId();
};

std::weak_ptr<Dog> Player::GetDog() { return dog_; };

}  // namespace model