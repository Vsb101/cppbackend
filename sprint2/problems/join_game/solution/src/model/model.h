#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "tagged.h"

namespace model {

// Предварительное объявление классов для разрешения циклических зависимостей
class Map;

/// Тип для координат и размеров (целочисленные значения)
using Dimension = int;
/// Тип для координатной точки (alias для Dimension)
using Coord = Dimension;

//=============================================================================
// Базовые структуры
//=============================================================================

/// Точка на игровой карте (система координат X, Y)
struct Point {
    Coord x;  ///< Координата по горизонтали
    Coord y;  ///< Координата по вертикали
};

/// Размер объекта на карте (ширина и высота)
struct Size {
    Dimension width;   ///< Ширина
    Dimension height;  ///< Высота
};

/// Прямоугольная область на карте (положение + размер)
struct Rectangle {
    Point position;  ///< Координаты верхнего левого угла
    Size size;       ///< Размеры (ширина и высота)
};

/// Смещение относительно какой-то точки
struct Offset {
    Dimension dx;  ///< Смещение по оси X
    Dimension dy;  ///< Смещение по оси Y
};

//=============================================================================
// Road - дорога на карте
//=============================================================================

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

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

//=============================================================================
// Building - здание на карте
//=============================================================================

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

//=============================================================================
// Office - офис на карте
//=============================================================================

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

//=============================================================================
// Dog - пёс на карте (доменная модель)
//=============================================================================

class Dog {
public:
    using Id = util::Tagged<std::string, Dog>;

    Dog(std::string name, const Map* map) noexcept
        : name_(std::move(name)), map_(map) {}

    const std::string& GetName() const noexcept { return name_; }

    const Map* GetMap() const noexcept { return map_; }

private:
    std::string name_;
    const Map* map_ = nullptr;
};

//=============================================================================
// GameSession - игровая сессия на одной карте
//=============================================================================

class GameSession {
public:
    explicit GameSession(const Map* map) noexcept : map_(map) {}

    const Map* GetMap() const noexcept { return map_; }

private:
    const Map* map_ = nullptr;
};

//=============================================================================
// Map - игровая карта
//=============================================================================

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

//=============================================================================
// Game - игра (коллекция карт) - доменная модель
//=============================================================================

class Game {
public:
    using Maps = std::vector<Map>;
    using Sessions = std::vector<GameSession>;
    using Dogs = std::vector<Dog>;

    Game() = default;

    // Запрещаем копирование и перемещение
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
    Game(Game&&) = delete;
    Game& operator=(Game&&) = delete;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    GameSession& CreateSession(const Map* map) {
        sessions_.emplace_back(map);
        return sessions_.back();
    }

    GameSession* FindSession(const Map* map) noexcept {
        auto it = std::find_if(sessions_.begin(), sessions_.end(),
                              [map](const GameSession& session) {
                                  return session.GetMap() == map;
                              });
        return it != sessions_.end() ? &(*it) : nullptr;
    }

    size_t CreateDog(std::string name, const Map* map) {
        dogs_.emplace_back(std::move(name), map);
        return dogs_.size() - 1;
    }

    const Dog* GetDog(size_t index) const noexcept {
        return &dogs_.at(index);
    }

    size_t GetDogsCount() const noexcept {
        return dogs_.size();
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    std::vector<GameSession> sessions_;
    std::vector<Dog> dogs_;
};

}  // namespace model
