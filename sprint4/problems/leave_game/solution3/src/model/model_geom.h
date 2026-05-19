#pragma once

#include <chrono>   // std::chrono
#include <utility>  // std::pair

namespace model {

// Единицы измерения расстояния
using DynamicDimension = double;    // Расстояние игровых единицах
using DynamicCoord = DynamicDimension;
using StartEndCoord = std::pair<DynamicDimension,DynamicDimension>;

// Единицы измерения времени
using TimeType = std::chrono::milliseconds;  // Время в миллисекундах

// Позиция в двумерном пространстве
struct Position {
    DynamicCoord x, y;
};

bool operator==(Position a, Position b);
bool operator!=(Position a, Position b);
Position operator+(Position a, Position b);

// Скорость в двумерном пространстве
struct Speed {
    DynamicDimension ux, uy;
};

bool operator==(Speed a, Speed b);
bool operator!=(Speed a, Speed b);

Position operator*(Speed speed, TimeType dt);

// Единицы измерения для статических игровых объектов
using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

Coord radius_squared(const Point& point);

bool operator==(Point a, Point b);
bool operator!=(Point a, Point b);

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

enum class Direction {
    NORTH = 'U',
    SOUTH = 'D',
    WEST = 'L',
    EAST = 'R'
};

}  // namespace model
