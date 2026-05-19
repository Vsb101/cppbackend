#include <catch2/catch_test_macros.hpp>
//#include <catch2/matchers/catch_matchers_templated.hpp>

#include "../src/model/dog.h"

using namespace model;

// Test cases for the Dog class
SCENARIO("Dog") {
    // Test case 1: Check if the dog's ID is set correctly
    SECTION("ID") {
        Dog::Id id{0};
        Dog dog{id, Dog::Name{"Sharik"}, Position{0.0, 0.0}, Speed{0.0, 0.0}};
        REQUIRE(dog.GetId() == id);
    }

    // Test case 2: Check if the dog's name is set correctly
    SECTION("Name") {
        Dog::Name name{"Sharik"};
        Dog dog{Dog::Id{0}, name, Position{0.0, 0.0}, Speed{0.0, 0.0}};
        REQUIRE(dog.GetName() == name);
    }

    // Test case 3: Check if the dog's position is set correctly
    SECTION("Position") {
        Position position{1.0, 2.0};
        Dog dog{Dog::Id{0}, Dog::Name{"Sharik"}, position, Speed{0.0, 0.0}};
        REQUIRE(dog.GetPosition() == position);
    }

    // Test case 4: Check if the dog's speed is set correctly
    SECTION("Speed") {
        Speed speed{1.0, 2.0};
        Dog dog{Dog::Id{0}, Dog::Name{"Sharik"}, Position{0.0, 0.0}, speed};
        REQUIRE(dog.GetSpeed() == speed);
    }

    // Test case 5: Check if the dog's direction is set correctly
    SECTION("Direction") {
        Direction direction = Direction::NORTH;
        Dog dog{Dog::Id{0}, Dog::Name{"Sharik"}, Position{0.0, 0.0}, Speed{0.0, 0.0}, direction};
        REQUIRE(dog.GetDirection() == direction);
    }
}