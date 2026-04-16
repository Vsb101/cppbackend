#pragma once
#include "../other/tagged.h"

#include <string>
#include <unordered_map>

namespace model {

    enum class Direction {
        NORTH,
        SOUTH,
        WEST,
        EAST
    };

    const std::unordered_map<Direction, std::string> DIRECTION_TO_JSON = {
    {Direction::NORTH, "U"},
    {Direction::SOUTH, "D"},
    {Direction::WEST,  "L"},
    {Direction::EAST,  "R"}
    };

    const std::unordered_map<std::string, Direction> JSON_TO_DIRECTION = {
    {"U", Direction::NORTH},
    {"D", Direction::SOUTH},
    {"L", Direction::WEST},
    {"R", Direction::EAST}
    };

    struct Position {
        double x, y;
    };

    struct Speed {
        double vx, vy;
    };

    class Dog {
        inline static size_t cntMaxId = 0;

    public:
        using Id = util::Tagged<size_t, Dog>;

        Dog(std::string name) :
            id_(Id{ Dog::cntMaxId++ }),
            name_(name) {};

        Dog(Id id, std::string name) :
            id_(id),
            name_(name) {};

        /*Кострукторы копирования все дефолтные*/
        Dog(const Dog& other) = default;
        Dog(Dog&& other) = default;
        Dog& operator = (const Dog& other) = default;
        Dog& operator = (Dog&& other) = default;
        virtual ~Dog() = default;

        const Id& GetId() const;                    //Геттер на айди
        const std::string& GetName() const;         //Геттер на имя

        const Direction GetDirection() const;       //Геттер на направление
        void SetDirection(Direction direction);     //Сеттер на направление
        
        const Position& GetPosition() const;        //Геттер на позицию
        void SetPosition(Position position);        //Сеттер на позицию
        
        const Speed& GetSpeed() const;              //Геттер на скорость
        void SetSpeed(Speed velocity);              //Сеттер на скорость
        

    private:
        Id id_;                                     //айди
        std::string name_;                          //имя
        Direction direction_{ Direction::NORTH };   //направление
        Position position_{ 0.0, 0.0 };             //позиция
        Speed speed_{ 0.0, 0.0 };                   //скорость
    };

}