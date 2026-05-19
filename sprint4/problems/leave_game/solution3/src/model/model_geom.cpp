#include "model_geom.h"

#include <limits>

namespace model {

bool operator==(Position a, Position b) {
    const DynamicDimension eps = std::numeric_limits<DynamicDimension>::epsilon();
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) < eps;
}

bool operator!=(Position a, Position b) {
    return !(a == b);
}

Position operator+(Position a, Position b) {
    return Position{a.x + b.x, a.y + b.y};
}

bool operator==(Speed a, Speed b) {
    const DynamicDimension eps = std::numeric_limits<DynamicDimension>::epsilon();
    return (a.ux - b.ux) * (a.ux - b.ux) + (a.uy - b.uy) * (a.uy - b.uy) < eps;
}

bool operator!=(Speed a, Speed b) {
    return !(a == b);
}

Position operator*(Speed speed, TimeType dt) {
    // Физическое время в секундах, а TimeType в милисекундах
    return Position{speed.ux * dt.count() / 1000, speed.uy * dt.count() / 1000};
}

Coord radius_squared(const Point& point) {
    return point.x * point.x + point.y * point.y;
}

bool operator==(Point a, Point b) {
    return a.x == b.x && a.y == b.y;
}

bool operator!=(Point a, Point b) {
    return !(a == b);
}

}  // namespace model
