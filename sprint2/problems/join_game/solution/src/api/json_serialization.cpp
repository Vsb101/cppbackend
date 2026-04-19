#include <boost/json/src.hpp> // МЕГА-ВАЖНО
#include "json_serialization.h"

namespace model {

namespace json = boost::json;

namespace {
    namespace keys {
        const auto x0 = "x0"; const auto y0 = "y0"; const auto x1 = "x1"; const auto y1 = "y1";
        const auto x = "x";   const auto y = "y";   const auto w = "w";   const auto h = "h";
        const auto id = "id"; const auto name = "name";
        const auto roads = "roads"; const auto buildings = "buildings"; const auto offices = "offices";
        const auto offsetX = "offsetX"; const auto offsetY = "offsetY";
    }
}

void tag_invoke(json::value_from_tag, json::value& jv, const Road& road) {
    if (road.IsHorizontal()) {
        jv = {{keys::x0, road.GetStart().x}, {keys::y0, road.GetStart().y}, {keys::x1, road.GetEnd().x}};
    } else {
        jv = {{keys::x0, road.GetStart().x}, {keys::y0, road.GetStart().y}, {keys::y1, road.GetEnd().y}};
    }
}

void tag_invoke(json::value_from_tag, json::value& jv, const Building& building) {
    const auto& bounds = building.GetBounds();
    jv = {{keys::x, bounds.position.x}, {keys::y, bounds.position.y}, 
          {keys::w, bounds.size.width}, {keys::h, bounds.size.height}};
}

void tag_invoke(json::value_from_tag, json::value& jv, const Office& office) {
    jv = {{keys::id, *office.GetId()}, {keys::x, office.GetPosition().x}, {keys::y, office.GetPosition().y},
          {keys::offsetX, office.GetOffset().dx}, {keys::offsetY, office.GetOffset().dy}};
}

void tag_invoke(json::value_from_tag, json::value& jv, const Map& map) {
    jv = {{keys::id, *map.GetId()}, {keys::name, map.GetName()},
          {keys::roads, map.GetRoads()}, {keys::buildings, map.GetBuildings()}, {keys::offices, map.GetOffices()}};
}

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

namespace app {

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Player& player) {
    jv = boost::json::object{
        {"id", *player.GetId()},
        {"name", player.GetName()}
    };
}

} // namespace app
