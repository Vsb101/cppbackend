#include "dog.h"

namespace model {

void Dog::SetSpeed(DynamicDimension speed, std::optional<Direction> direction) noexcept {
    if (!direction.has_value()) {
        speed_.ux = speed_.uy = 0.0;
        return;
    }

    direction_ = direction.value();
    switch (direction_) {
        case Direction::NORTH: {
            speed_.ux = 0.0;
            speed_.uy = -speed;
            break;
        }
        case Direction::SOUTH: {
            speed_.ux = 0.0;
            speed_.uy = speed;
            break;
        }
        case Direction::WEST: {
            speed_.ux = -speed;
            speed_.uy = 0.0;
            break;
        }
        case Direction::EAST: {
            speed_.ux = speed;
            speed_.uy = 0.0;
            break;
        }
    }
}

}  // namespace model
