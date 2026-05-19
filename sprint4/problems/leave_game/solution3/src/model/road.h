#pragma once

#include "model_geom.h"
#include <algorithm>    // std::min,max

namespace model {

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    const Point& GetStart() const noexcept {
        return start_;
    }

    const Point& GetEnd() const noexcept {
        return end_;
    }

    StartEndCoord GetOxProjection() const noexcept {
        return {std::min(start_.x, end_.x)-d_width, std::max(start_.x, end_.x)+d_width};
    }

    StartEndCoord GetOyProjection() const noexcept {
        return {std::min(start_.y, end_.y)-d_width, std::max(start_.y, end_.y)+d_width};
    }

public:
    // Половина ширины дороги или расстояние, на которое можно отойти от оси дороги
    static constexpr DynamicDimension d_width = 0.4;

private:
    Point start_;
    Point end_;
};

bool operator==(Road a, Road b);
bool operator!=(Road a, Road b);

}  // namespace model
