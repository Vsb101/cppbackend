#include "../model/map.h"
#include <stdexcept>
#include <string>

namespace model {

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate office");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        offices_.pop_back();
        throw;
    }
}

void tag_invoke(json::value_from_tag, json::value& json_value, const Road& road) {
    if (road.IsHorizontal()) {
        json_value = {{kRoadX0, json::value_from(road.GetStart().x)},
                      {kRoadY0, json::value_from(road.GetStart().y)},
                      {kRoadX1, json::value_from(road.GetEnd().x)}};
    } else {
        json_value = {{kRoadX0, json::value_from(road.GetStart().x)},
                      {kRoadY0, json::value_from(road.GetStart().y)},
                      {kRoadY1, json::value_from(road.GetEnd().y)}};
    }
}

Road tag_invoke(json::value_to_tag<Road>, const json::value& json_value) {
    Point start;
    start.x = json::value_to<int>(json_value.as_object().at(std::string(kRoadX0)));
    start.y = json::value_to<int>(json_value.as_object().at(std::string(kRoadY0)));
    Coord end;
    try {
        end = json::value_to<int>(json_value.as_object().at(std::string(kRoadX1)));
        return Road(Road::HORIZONTAL, start, end);
    } catch (...) {
        end = json::value_to<int>(json_value.as_object().at(std::string(kRoadY1)));
        return Road(Road::VERTICAL, start, end);
    }
}

void tag_invoke(json::value_from_tag, json::value& json_value, const Building& building) {
    json_value = {{kBuildingX, json::value_from(building.GetBounds().position.x)},
                  {kBuildingY, json::value_from(building.GetBounds().position.y)},
                  {kBuildingWidth, json::value_from(building.GetBounds().size.width)},
                  {kBuildingHeight, json::value_from(building.GetBounds().size.height)}};
}

Building tag_invoke(json::value_to_tag<Building>, const json::value& json_value) {
    Point point;
    point.x = json::value_to<int>(json_value.as_object().at(std::string(kBuildingX)));
    point.y = json::value_to<int>(json_value.as_object().at(std::string(kBuildingY)));
    Size size;
    size.width = json::value_to<int>(json_value.as_object().at(std::string(kBuildingWidth)));
    size.height = json::value_to<int>(json_value.as_object().at(std::string(kBuildingHeight)));
    return Building(Rectangle(point, size));
}

void tag_invoke(json::value_from_tag, json::value& json_value, const Office& office) {
    json_value = {{kOfficeId, json::value_from(*(office.GetId()))},
                  {kOfficeX, json::value_from(office.GetPosition().x)},
                  {kOfficeY, json::value_from(office.GetPosition().y)},
                  {kOfficeOffsetX, json::value_from(office.GetOffset().dx)},
                  {kOfficeOffsetY, json::value_from(office.GetOffset().dy)}};
}

Office tag_invoke(json::value_to_tag<Office>, const json::value& json_value) {
    Office::Id id{json::value_to<std::string>(json_value.as_object().at(std::string(kOfficeId)))};
    Point position;
    position.x = json::value_to<int>(json_value.as_object().at(std::string(kOfficeX)));
    position.y = json::value_to<int>(json_value.as_object().at(std::string(kOfficeY)));
    Offset offset;
    offset.dx = json::value_to<int>(json_value.as_object().at(std::string(kOfficeOffsetX)));
    offset.dy = json::value_to<int>(json_value.as_object().at(std::string(kOfficeOffsetY)));
    return Office(id, position, offset);
}

void tag_invoke(json::value_from_tag, json::value& json_value, const Map& map) {
    json_value = {{kMapId, json::value_from(*(map.GetId()))},
                  {kMapName, json::value_from(map.GetName())},
                  {kRoads, json::value_from(map.GetRoads())},
                  {kBuildings, json::value_from(map.GetBuildings())},
                  {kOffices, json::value_from(map.GetOffices())}};
}

Map tag_invoke(json::value_to_tag<Map>, const json::value& json_value) {
    Map::Id id{json::value_to<std::string>(json_value.as_object().at(std::string(kMapId)))};
    std::string name = json::value_to<std::string>(json_value.as_object().at(std::string(kMapName)));
    Map map(id, name);
    std::vector<Road> roads = json::value_to<std::vector<Road>>(json_value.as_object().at(std::string(kRoads)));
    map.AddRoads(roads);
    std::vector<Building> buildings = json::value_to<std::vector<Building>>(json_value.as_object().at(std::string(kBuildings)));
    map.AddBuildings(buildings);
    std::vector<Office> offices = json::value_to<std::vector<Office>>(json_value.as_object().at(std::string(kOffices)));
    map.AddOffices(offices);
    return map;
}

}  // namespace model