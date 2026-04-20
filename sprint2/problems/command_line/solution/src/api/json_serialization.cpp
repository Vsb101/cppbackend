// json_serialization.cpp
#include <boost/json/src.hpp>  // Только здесь! Должен быть включён ровно 1 раз во всём проекте
#include "json_serialization.h"
#include "../model/dog.h"      // Только здесь нужна реализация Dog

namespace model {

namespace json = boost::json;

namespace {
    // JSON keys — лучше не писать вручную каждый раз
    namespace keys {
        inline constexpr auto x0 = "x0"; inline constexpr auto y0 = "y0";
        inline constexpr auto x1 = "x1"; inline constexpr auto y1 = "y1";
        inline constexpr auto x  = "x";  inline constexpr auto y  = "y";
        inline constexpr auto w  = "w";  inline constexpr auto h  = "h";
        inline constexpr auto id = "id"; inline constexpr auto name = "name";
        inline constexpr auto roads = "roads"; 
        inline constexpr auto buildings = "buildings"; 
        inline constexpr auto offices = "offices";
        inline constexpr auto offsetX = "offsetX"; 
        inline constexpr auto offsetY = "offsetY";
        inline constexpr auto pos = "pos";
        inline constexpr auto speed = "speed";
        inline constexpr auto dir = "dir";
    }

    // Вспомогательная функция для направления
    std::string_view DirectionToString(Direction d) {
        switch (d) {
            case Direction::NORTH: return "U";
            case Direction::SOUTH: return "D";
            case Direction::WEST:  return "L";
            case Direction::EAST:  return "R";
            default: return "U";
        }
    }
}

// === Сериализация: value_from ===

// Road
void tag_invoke(json::value_from_tag, json::value& jv, const Road& road) {
    if (road.IsHorizontal()) {
        jv = {{keys::x0, road.GetStart().x}, {keys::y0, road.GetStart().y}, {keys::x1, road.GetEnd().x}};
    } else {
        jv = {{keys::x0, road.GetStart().x}, {keys::y0, road.GetStart().y}, {keys::y1, road.GetEnd().y}};
    }
}

// Building
void tag_invoke(json::value_from_tag, json::value& jv, const Building& building) {
    const auto& bounds = building.GetBounds();
    jv = {
        {keys::x, bounds.position.x},
        {keys::y, bounds.position.y},
        {keys::w, bounds.size.width},
        {keys::h, bounds.size.height}
    };
}

// Office
void tag_invoke(json::value_from_tag, json::value& jv, const Office& office) {
    jv = {
        {keys::id, *office.GetId()},
        {keys::x, office.GetPosition().x},
        {keys::y, office.GetPosition().y},
        {keys::offsetX, office.GetOffset().dx},
        {keys::offsetY, office.GetOffset().dy}
    };
}

// Map
void tag_invoke(json::value_from_tag, json::value& jv, const Map& map) {
    jv = {
        {keys::id, *map.GetId()},
        {keys::name, map.GetName()},
        {keys::roads, map.GetRoads()},
        {keys::buildings, map.GetBuildings()},
        {keys::offices, map.GetOffices()}
    };
}

// Dog
void tag_invoke(json::value_from_tag, json::value& jv, const Dog& dog) {
    jv = {
        {keys::pos,   {dog.GetPosition().x, dog.GetPosition().y}},
        {keys::speed, {dog.GetSpeed().vx, dog.GetSpeed().vy}},
        {keys::dir,   DirectionToString(dog.GetDirection())}
    };
}

// === Десериализация: value_to ===

Road tag_invoke(json::value_to_tag<Road>, const json::value& jv) {
    const auto& obj = jv.as_object();
    Point start{json::value_to<Coord>(obj.at(keys::x0)), json::value_to<Coord>(obj.at(keys::y0))};
    if (obj.contains(keys::x1)) {
        return Road{Road::HORIZONTAL, start, json::value_to<Coord>(obj.at(keys::x1))};
    }
    return Road{Road::VERTICAL, start, json::value_to<Coord>(obj.at(keys::y1))};
}

Building tag_invoke(json::value_to_tag<Building>, const json::value& jv) {
    const auto& obj = jv.as_object();
    return Building{Rectangle{
        Point{json::value_to<Coord>(obj.at(keys::x)), json::value_to<Coord>(obj.at(keys::y))},
        Size{json::value_to<Dimension>(obj.at(keys::w)), json::value_to<Dimension>(obj.at(keys::h))}
    }};
}

Office tag_invoke(json::value_to_tag<Office>, const json::value& jv) {
    const auto& obj = jv.as_object();
    return Office{
        Office::Id{json::value_to<std::string>(obj.at(keys::id))},
        Point{json::value_to<Coord>(obj.at(keys::x)), json::value_to<Coord>(obj.at(keys::y))},
        Offset{json::value_to<Dimension>(obj.at(keys::offsetX)), json::value_to<Dimension>(obj.at(keys::offsetY))}
    };
}

Map tag_invoke(json::value_to_tag<Map>, const json::value& jv) {
    const auto& obj = jv.as_object();
    Map map{Map::Id{json::value_to<std::string>(obj.at(keys::id))}, 
            json::value_to<std::string>(obj.at(keys::name))};

    for (const auto& r : obj.at(keys::roads).as_array()) {
        map.AddRoad(json::value_to<Road>(r));
    }
    for (const auto& b : obj.at(keys::buildings).as_array()) {
        map.AddBuilding(json::value_to<Building>(b));
    }
    for (const auto& o : obj.at(keys::offices).as_array()) {
        map.AddOffice(json::value_to<Office>(o));
    }
    return map;
}

}  // namespace model

// === Player serialization (app) ===

namespace app {

void tag_invoke(json::value_from_tag, json::value& jv, const Player& player) {
    jv = json::object{
        {"id", *player.GetId()},
        {"name", player.GetName()}
    };
}

} // namespace app