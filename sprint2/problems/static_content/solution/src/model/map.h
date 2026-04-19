#pragma once

#include "../other/tagged.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <cstddef>

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
public:
    struct HorizontalTag { explicit HorizontalTag() = default; };
    struct VerticalTag { explicit VerticalTag() = default; };

    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}, end_{end_x, start.y} {}

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}, end_{start.x, end_y} {}

    [[nodiscard]] bool IsHorizontal() const noexcept { return start_.y == end_.y; }
    [[nodiscard]] bool IsVertical() const noexcept { return start_.x == end_.x; }
    [[nodiscard]] Point GetStart() const noexcept { return start_; }
    [[nodiscard]] Point GetEnd() const noexcept { return end_; }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept : bounds_{bounds} {}
    [[nodiscard]] const Rectangle& GetBounds() const noexcept { return bounds_; }
private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}, position_{position}, offset_{offset} {}

    [[nodiscard]] const Id& GetId() const noexcept { return id_; }
    [[nodiscard]] Point GetPosition() const noexcept { return position_; }
    [[nodiscard]] Offset GetOffset() const noexcept { return offset_; }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id)), name_(std::move(name)) {}

    [[nodiscard]] const Id& GetId() const noexcept { return id_; }
    [[nodiscard]] const std::string& GetName() const noexcept { return name_; }
    [[nodiscard]] const Buildings& GetBuildings() const noexcept { return buildings_; }
    [[nodiscard]] const Roads& GetRoads() const noexcept { return roads_; }
    [[nodiscard]] const Offices& GetOffices() const noexcept { return offices_; }

    void AddRoad(const Road& road) { roads_.push_back(road); }
    void AddBuilding(const Building& building) { buildings_.push_back(building); }
    void AddOffice(Office office);

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    Offices offices_;
    OfficeIdToIndex office_id_to_index_;
};

}  // namespace model
