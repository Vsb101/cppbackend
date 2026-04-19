#include "player.h"

namespace model {

Player::Player(std::string name)
    : id_(Id{cnt_max_id_++})
    , name_(std::move(name)) {}

Player::Player(Id id, std::string name)
    : id_(id)
    , name_(std::move(name)) {}

void Player::SetGameSession(std::weak_ptr<GameSession> session) {
    session_ = session;
}

void Player::SetDog(std::weak_ptr<Dog> dog) {
    dog_ = dog;
}

const Player::Id& Player::GetId() const {
    return id_;
}

const std::string& Player::GetName() const {
    return name_;
}

const model::GameSession::Id& Player::GetSessionId() const {
    return session_.lock()->GetId();
}

std::weak_ptr<Dog> Player::GetDog() {
    return dog_;
}

}  // namespace model