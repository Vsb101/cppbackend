// src/json_builder.cpp

#include "json_builder.h"
#include <boost/json.hpp>
#include <vector>

namespace json_builder {

namespace json = boost::json;

namespace {

template<typename T, typename F>
json::array SerializeVector(const std::vector<T>& items, F serializer) {
    json::array arr;
    for (const auto& item : items) {
        arr.push_back(serializer(item));
    }
    return arr;
}

json::object SerializeRoad(const model::Road& road) {
    json::object obj;
    obj["x0"] = road.GetStart().x;
    obj["y0"] = road.GetStart().y;
    if (road.IsHorizontal()) {
        obj["x1"] = road.GetEnd().x;
    } else {
        obj["y1"] = road.GetEnd().y;
    }
    return obj;
}

json::object SerializeBuilding(const model::Building& building) {
    const auto& bounds = building.GetBounds();
    json::object obj;
    obj["x"] = bounds.position.x;
    obj["y"] = bounds.position.y;
    obj["w"] = bounds.size.width;
    obj["h"] = bounds.size.height;
    return obj;
}

json::object SerializeOffice(const model::Office& office) {
    json::object obj;
    obj["id"] = *office.GetId();
    obj["x"] = office.GetPosition().x;
    obj["y"] = office.GetPosition().y;
    obj["offsetX"] = office.GetOffset().dx;
    obj["offsetY"] = office.GetOffset().dy;
    return obj;
}

} // namespace

std::string BuildMap(const model::Map& map) {
    json::object result;
    result["id"] = *map.GetId();
    result["name"] = map.GetName();

    result["roads"] = SerializeVector(map.GetRoads(), SerializeRoad);
    result["buildings"] = SerializeVector(map.GetBuildings(), SerializeBuilding);
    result["offices"] = SerializeVector(map.GetOffices(), SerializeOffice);

    return json::serialize(result);
}

std::string BuildMapsList(const model::Game& game) {
    json::array maps_array;
    for (const auto& map : game.GetMaps()) {
        json::object map_obj;
        map_obj["id"] = *map.GetId();
        map_obj["name"] = map.GetName();
        maps_array.push_back(std::move(map_obj));
    }
    return json::serialize(maps_array);
}

} // namespace json_builder