#pragma once

/**
 * @file map.h
 * @brief Модель игровой карты
 */

#include "../other/tagged.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <boost/json.hpp>
#include <utility>
#include <cstddef>

namespace model {

namespace json = boost::json;

// Ключи JSON для мапы
constexpr std::string_view kMaps = "maps";
constexpr std::string_view kMapId = "id";
constexpr std::string_view kMapName = "name";
constexpr std::string_view kRoads = "roads";
constexpr std::string_view kRoadX0 = "x0";
constexpr std::string_view kRoadY0 = "y0";
constexpr std::string_view kRoadX1 = "x1";
constexpr std::string_view kRoadY1 = "y1";
constexpr std::string_view kBuildings = "buildings";
constexpr std::string_view kBuildingX = "x";
constexpr std::string_view kBuildingY = "y";
constexpr std::string_view kBuildingWidth = "w";
constexpr std::string_view kBuildingHeight = "h";
constexpr std::string_view kOffices = "offices";
constexpr std::string_view kOfficeId = "id";
constexpr std::string_view kOfficeX = "x";
constexpr std::string_view kOfficeY = "y";
constexpr std::string_view kOfficeOffsetX = "offsetX";
constexpr std::string_view kOfficeOffsetY = "offsetY";

using Dimension = int;
using Coord = Dimension;

/**
 * @brief Точка на карте
 */
struct Point {
    Coord x;
    Coord y;
};

/**
 * @brief Размер объекта
 */
struct Size {
    Dimension width;
    Dimension height;
};

/**
 * @brief Прямоугольник
 */
struct Rectangle {
    Point position;
    Size size;
};

/**
 * @brief Смещение
 */
struct Offset {
    Dimension dx;
    Dimension dy;
};

/**
 * @brief Дорога на карте (горизонтальная или вертикальная)
 */
class Road {
 public:
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {}

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {}

    [[nodiscard]] bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    [[nodiscard]] bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    [[nodiscard]] Point GetStart() const noexcept {
        return start_;
    }

    [[nodiscard]] Point GetEnd() const noexcept {
        return end_;
    }

 private:
    Point start_;
    Point end_;
};

void tag_invoke(json::value_from_tag, json::value& json_value, const Road& road);
Road tag_invoke(json::value_to_tag<Road>, const json::value& json_value);

/**
 * @brief Здание на карте
 */
class Building {
 public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {}

    [[nodiscard]] const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

 private:
    Rectangle bounds_;
};

void tag_invoke(json::value_from_tag, json::value& json_value, const Building& building);
Building tag_invoke(json::value_to_tag<Building>, const json::value& json_value);

/**
 * @brief Офис на карте
 */
class Office {
 public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {}

    [[nodiscard]] const Id& GetId() const noexcept {
        return id_;
    }

    [[nodiscard]] Point GetPosition() const noexcept {
        return position_;
    }

    [[nodiscard]] Offset GetOffset() const noexcept {
        return offset_;
    }

 private:
    Id id_;
    Point position_;
    Offset offset_;
};

void tag_invoke(json::value_from_tag, json::value& json_value, const Office& office);
Office tag_invoke(json::value_to_tag<Office>, const json::value& json_value);

/**
 * @brief Игровая карта
 */
class Map {
 public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {}

    [[nodiscard]] const Id& GetId() const noexcept {
        return id_;
    }

    [[nodiscard]] const std::string& GetName() const noexcept {
        return name_;
    }

    [[nodiscard]] const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    [[nodiscard]] const Roads& GetRoads() const noexcept {
        return roads_;
    }

    [[nodiscard]] const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddRoads(Roads& roads) {
        for (const auto& item : roads) {
            AddRoad(item);
        }
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddBuildings(Buildings& buildings) {
        for (const auto& item : buildings) {
            AddBuilding(item);
        }
    }

    void AddOffice(Office office);

    void AddOffices(Offices& offices) {
        for (auto& item : offices) {
            AddOffice(item);
        }
    }

 private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

void tag_invoke(json::value_from_tag, json::value& json_value, const Map& map);
Map tag_invoke(json::value_to_tag<Map>, const json::value& json_value);

}  // namespace model