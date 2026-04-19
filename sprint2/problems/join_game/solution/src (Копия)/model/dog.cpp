#include "dog.h"

namespace model {

Dog::Dog(std::string name)
    : id_(Id{cnt_max_id_++})
    , name_(std::move(name)) {}

Dog::Dog(Id id, std::string name)
    : id_(id)
    , name_(std::move(name)) {}

const Dog::Id& Dog::GetId() const {
    return id_;
}

const std::string& Dog::GetName() const {
    return name_;
}

Direction Dog::GetDirection() const {
    return direction_;
}

void Dog::SetDirection(Direction direction) {
    direction_ = direction;
}

const Position& Dog::GetPosition() const {
    return position_;
}

void Dog::SetPosition(Position position) {
    position_ = position;
}

const Speed& Dog::GetSpeed() const {
    return speed_;
}

void Dog::SetSpeed(Speed velocity) {
    speed_ = velocity;
}

}  // namespace model