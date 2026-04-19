#pragma once

#include "../other/tagged.h"
#include <string>

namespace model {

enum class Direction { NORTH, SOUTH, WEST, EAST };

struct Position { double x, y; };
struct Speed { double vx, vy; };

class Dog {
public:
    using Id = util::Tagged<size_t, Dog>;

    // Убираем статический счетчик из класса. 
    // ID должен приходить извне (например, от хранилища или БД)
    Dog(Id id, std::string name)
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const { return id_; }
    const std::string& GetName() const { return name_; }

    Direction GetDirection() const { return direction_; }
    void SetDirection(Direction direction) { direction_ = direction; }

    const Position& GetPosition() const { return position_; }
    void SetPosition(Position position) { position_ = position; }

    const Speed& GetSpeed() const { return speed_; }
    void SetSpeed(Speed speed) { speed_ = speed; }

private:
    Id id_;
    std::string name_;
    Direction direction_{Direction::NORTH};
    Position position_{0.0, 0.0};
    Speed speed_{0.0, 0.0};
};

}  // namespace model
