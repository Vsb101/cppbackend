#include "request_handler.h"

#include <boost/json.hpp>

namespace http_handler {

namespace json = boost::json;

std::string MapToJson(const model::Map& map) {
    json::object result;
    
    result["id"] = *map.GetId();
    result["name"] = map.GetName();
    
    // Сериализация дорог
    json::array roads;
    for (const auto& road : map.GetRoads()) {
        json::object road_obj;
        road_obj["x0"] = road.GetStart().x;
        road_obj["y0"] = road.GetStart().y;
        
        if (road.IsHorizontal()) {
            road_obj["x1"] = road.GetEnd().x;
        } else {
            road_obj["y1"] = road.GetEnd().y;
        }
        roads.push_back(road_obj);
    }
    result["roads"] = roads;
    
    // Сериализация зданий
    json::array buildings;
    for (const auto& building : map.GetBuildings()) {
        json::object building_obj;
        const auto& bounds = building.GetBounds();
        building_obj["x"] = bounds.position.x;
        building_obj["y"] = bounds.position.y;
        building_obj["w"] = bounds.size.width;
        building_obj["h"] = bounds.size.height;
        buildings.push_back(building_obj);
    }
    result["buildings"] = buildings;
    
    // Сериализация офисов
    json::array offices;
    for (const auto& office : map.GetOffices()) {
        json::object office_obj;
        office_obj["id"] = *office.GetId();
        office_obj["x"] = office.GetPosition().x;
        office_obj["y"] = office.GetPosition().y;
        office_obj["offsetX"] = office.GetOffset().dx;
        office_obj["offsetY"] = office.GetOffset().dy;
        offices.push_back(office_obj);
    }
    result["offices"] = offices;
    
    return json::serialize(result);
}

std::string MapsListToJson(const model::Game& game) {
    json::array maps_array;
    
    for (const auto& map : game.GetMaps()) {
        json::object map_obj;
        map_obj["id"] = *map.GetId();
        map_obj["name"] = map.GetName();
        maps_array.push_back(map_obj);
    }
    
    return json::serialize(maps_array);
}

}  // namespace http_handler
