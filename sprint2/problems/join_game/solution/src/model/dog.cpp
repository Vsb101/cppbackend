#include "dog.h"

namespace model {

    const Dog::Id& Dog::GetId() const {
        return id_;
    };

    const std::string& Dog::GetName() const {
        return name_;
    };

    void Dog::SetDirection(Direction direction) {
        direction_ = std::move(direction);
    };

    const Direction Dog::GetDirection() const {
        return direction_;
    };

    void Dog::SetPosition(Position position) {
        position_ = std::move(position);
    };

    const Position& Dog::GetPosition() const {
        return position_;
    };

    void Dog::SetSpeed(Speed velocity) {
        speed_ = velocity;
    };

    const Speed& Dog::GetSpeed() const {
        return speed_;
    };

}