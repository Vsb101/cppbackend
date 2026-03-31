#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "tagged.h"

namespace model {

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

/**
 * @class Road
 * @brief Представляет дорогу на игровой карте.
 *
 * Дорога может быть горизонтальной или вертикальной.
 * Горизонтальная дорога: y0 == y1, меняется x
 * Вертикальная дорога: x0 == x1, меняется y
 *
 * Пример создания:
 * @code
 * // Горизонтальная дорога от (0,0) до (10,0)
 * Road horizontal(Road::HORIZONTAL, {0, 0}, 10);
 * // Вертикальная дорога от (0,0) до (0,5)
 * Road vertical(Road::VERTICAL, {0, 0}, 5);
 * @endcode
 */
class Road {
    /// Тег для создания горизонтальной дороги
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    /// Тег для создания вертикальной дороги
    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    /// Константа для создания горизонтальной дороги
    constexpr static HorizontalTag HORIZONTAL{};
    /// Константа для создания вертикальной дороги
    constexpr static VerticalTag VERTICAL{};

    /**
     * @brief Конструктор горизонтальной дороги
     * @param start Начальная точка дороги
     * @param end_x Координата X конечной точки
     */
    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    /**
     * @brief Конструктор вертикальной дороги
     * @param start Начальная точка дороги
     * @param end_y Координата Y конечной точки
     */
    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    /// @return true если дорога горизонтальная (y0 == y1)
    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    /// @return true если дорога вертикальная (x0 == x1)
    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    /// @return Начальная точка дороги
    Point GetStart() const noexcept {
        return start_;
    }

    /// @return Конечная точка дороги
    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;  ///< Начальная точка
    Point end_;    ///< Конечная точка
};

//=============================================================================
// Building - здание на карте
//=============================================================================

/**
 * @class Building
 * @brief Представляет здание на игровой карте.
 *
 * Здание задаётся прямоугольной областью (bounds).
 */
class Building {
public:
    /**
     * @brief Конструктор здания
     * @param bounds Прямоугольная область, занимаемая зданием
     */
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    /// @return Прямоугольная область здания
    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;  ///< Границы здания
};

//=============================================================================
// Office - офис на карте
//=============================================================================

/**
 * @class Office
 * @brief Представляет офис (например, склад) на игровой карте.
 *
 * Офис имеет уникальный идентификатор, позицию на карте
 * и смещение для визуализации.
 */
class Office {
public:
    /// Тип идентификатора офиса (строка с тегом)
    using Id = util::Tagged<std::string, Office>;

    /**
     * @brief Конструктор офиса
     * @param id Уникальный идентификатор
     * @param position Позиция на карте
     * @param offset Смещение для визуализации
     */
    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    /// @return Идентификатор офиса
    const Id& GetId() const noexcept {
        return id_;
    }

    /// @return Позиция офиса на карте
    Point GetPosition() const noexcept {
        return position_;
    }

    /// @return Смещение офиса
    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;          ///< Уникальный идентификатор
    Point position_; ///< Позиция на карте
    Offset offset_;  ///< Смещение для визуализации
};

//=============================================================================
// Map - игровая карта
//=============================================================================

/**
 * @class Map
 * @brief Представляет игровую карту.
 *
 * Карта содержит дороги, здания и офисы.
 * Имеет уникальный идентификатор и название.
 */
class Map {
public:
    /// Тип идентификатора карты (строка с тегом)
    using Id = util::Tagged<std::string, Map>;
    /// Контейнер для хранения дорог
    using Roads = std::vector<Road>;
    /// Контейнер для хранения зданий
    using Buildings = std::vector<Building>;
    /// Контейнер для хранения офисов
    using Offices = std::vector<Office>;

    /**
     * @brief Конструктор карты
     * @param id Уникальный идентификатор карты
     * @param name Название карты
     */
    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    /// @return Идентификатор карты
    const Id& GetId() const noexcept {
        return id_;
    }

    /// @return Название карты
    const std::string& GetName() const noexcept {
        return name_;
    }

    /// @return Контейнер зданий
    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    /// @return Контейнер дорог
    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    /// @return Контейнер офисов
    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    /// @brief Добавляет дорогу на карту
    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    /// @brief Добавляет здание на карту
    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    /// @brief Добавляет офис на карту
    void AddOffice(Office office);

private:
    /// Карта для быстрого поиска офиса по id
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;                      ///< Идентификатор карты
    std::string name_;           ///< Название карты
    Roads roads_;                ///< Дороги на карте
    Buildings buildings_;        ///< Здания на карте
    OfficeIdToIndex warehouse_id_to_index_;  ///< Индекс офисов для быстрого поиска
    Offices offices_;            ///< Офисы на карте
};

//=============================================================================
// Game - игра (коллекция карт)
//=============================================================================

/**
 * @class Game
 * @brief Представляет игру - коллекцию карт.
 *
 * Является фасадом для доступа к картам игры.
 * Позволяет добавлять карты и искать их по идентификатору.
 */
class Game {
public:
    /// Контейнер для хранения карт
    using Maps = std::vector<Map>;

    /**
     * @brief Добавляет карту в игру
     * @param map Карта для добавления
     * @throw std::invalid_argument если карта с таким id уже существует
     */
    void AddMap(Map map);

    /// @return Контейнер со всеми картами
    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    /**
     * @brief Ищет карту по идентификатору
     * @param id Идентификатор карты
     * @return Указатель на карту или nullptr, если не найдена
     */
    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

private:
    /// Хешер для идентификатора карты
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    /// Карта для быстрого поиска карты по id
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;        ///< Все карты игры
    MapIdToIndex map_id_to_index_; ///< Индекс карт для быстрого поиска
};

}  // namespace model
