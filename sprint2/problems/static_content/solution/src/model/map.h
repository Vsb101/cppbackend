#pragma once

#include "../other/tagged.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <deque>
#include <algorithm>

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point { Coord x, y; };
struct Size { Dimension width, height; };
struct Rectangle { Point position; Size size; };
struct Offset { Dimension dx, dy; };

// --- КЛАССЫ ДОЛЖНЫ БЫТЬ ОБЪЯВЛЕНЫ ДО КЛАССА MAP ---

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
    using Roads = std::deque<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id)), name_(std::move(name)) {}

    const Id& GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return name_; }
    const Buildings& GetBuildings() const noexcept { return buildings_; }
    const Roads& GetRoads() const noexcept { return roads_; }
    const Offices& GetOffices() const noexcept { return offices_; }

    void AddRoad(const Road& road);
    void AddBuilding(const Building& building) { buildings_.push_back(building); }
    void AddOffice(Office office);

    const std::vector<const Road*>& GetHorizontalRoadsAt(int y) const;

    const std::vector<const Road*>& GetVerticalRoadsAt(int x) const;

    void SetDogSpeed(double speed) { dog_speed_ = speed; }
    double GetDogSpeed() const noexcept { return dog_speed_; }

private:
    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    Offices offices_;
    double dog_speed_ = 0.0;

    std::unordered_map<int, std::vector<const Road*>> horizontal_roads_;
    std::unordered_map<int, std::vector<const Road*>> vertical_roads_;
    
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
    OfficeIdToIndex office_id_to_index_;
};

}  // namespace model
