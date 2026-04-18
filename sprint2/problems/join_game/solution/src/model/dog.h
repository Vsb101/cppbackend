#pragma once

/**
 * @file dog.h
 * @brief Модель собаки игрока
 */

#include "../other/tagged.h"

#include <string>
#include <unordered_map>

namespace model {

/**
 * @brief Направление движения собаки
 */
enum class Direction {
    NORTH,  ///< Вверх
    SOUTH,  ///< Вниз
    WEST,   ///< Влево
    EAST    ///< Вправо
};

/**
 * @brief Преобразование направления в JSON-строку
 */
[[nodiscard]] constexpr std::string_view DirectionToJson(Direction direction) {
    switch (direction) {
        case Direction::NORTH: return "U";
        case Direction::SOUTH: return "D";
        case Direction::WEST:  return "L";
        case Direction::EAST:  return "R";
    }
    return "U";
}

/**
 * @brief Позиция на карте
 */
struct Position {
    double x;
    double y;
};

/**
 * @brief Скорость движения
 */
struct Speed {
    double vx;
    double vy;
};

/**
 * @brief Игровая собака
 */
class Dog {
 public:
    using Id = util::Tagged<size_t, Dog>;

    explicit Dog(std::string name);
    Dog(Id id, std::string name);

    Dog(const Dog& other) = default;
    Dog(Dog&& other) = default;
    Dog& operator=(const Dog& other) = default;
    Dog& operator=(Dog&& other) = default;
    virtual ~Dog() = default;

    [[nodiscard]] const Id& GetId() const;
    [[nodiscard]] const std::string& GetName() const;

    [[nodiscard]] Direction GetDirection() const;
    void SetDirection(Direction direction);

    [[nodiscard]] const Position& GetPosition() const;
    void SetPosition(Position position);

    [[nodiscard]] const Speed& GetSpeed() const;
    void SetSpeed(Speed velocity);

 private:
    Id id_;
    std::string name_;
    Direction direction_{Direction::NORTH};
    Position position_{0.0, 0.0};
    Speed speed_{0.0, 0.0};

    inline static size_t cnt_max_id_ = 0;
};

}  // namespace model