#pragma once

#include "model_geom.h"
#include "tagged.h"

#include <string>

namespace model {

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{std::move(position)}
        , offset_{std::move(offset)} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const Point& GetPosition() const noexcept {
        return position_;
    }

    const Offset& GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

}  // namespace model
