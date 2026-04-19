#include "json_handler.h"
#include "../model/map.h"
#include <map>
#include <sstream>
#include <boost/json/array.hpp>
#include <boost/json.hpp>                                                                   //Возврат на буст. Он читабельнее, хоть и менее понятен

/*Всё что ниже переделано на буст с jsoncpp*/
namespace jsonOperation {

    std::string GameToJson(const model::Game::Maps& game) {
        boost::json::array maps_arr;
        for (const auto& map : game) {
            boost::json::object item;
            item[std::string(model::kMapId)] = *(map->GetId());
            item[std::string(model::kMapName)] = map->GetName();
            maps_arr.push_back(item);
        }
        return boost::json::serialize(maps_arr);
    }

    std::string MapToJson(const model::Map& map) {                                              
        return boost::json::serialize(boost::json::value_from(map));
    }

    std::string MapNotFound() {
        boost::json::value msg = { {RESPONSE_CODE, "mapNotFound"},
                                    {RESPONSE_MESSAGE, "Map not found"} };
        return boost::json::serialize(msg);
    };

    std::string BadRequest() {
        boost::json::value msg = { {RESPONSE_CODE, "badRequest"},
                                    {RESPONSE_MESSAGE, "Bad request"} };
        return boost::json::serialize(msg);
    };

    std::string PageNotFound() {
        boost::json::value msg = { {RESPONSE_CODE, "pageNotFound"},
                                    {RESPONSE_MESSAGE, "Page not found"} };
        return boost::json::serialize(msg);
    };

    std::string JoinToGameInvalidArgument() {
        boost::json::value msg = { {RESPONSE_CODE, "invalidArgument"},
                            {RESPONSE_MESSAGE, "Join game request parse error"} };
        return boost::json::serialize(msg);
    };

    std::string JoinToGameMapNotFound() {
        boost::json::value msg = { {RESPONSE_CODE, "mapNotFound"},
                            {RESPONSE_MESSAGE, "Map not found"} };
        return boost::json::serialize(msg);
    };

    std::string JoinToGameEmptyPlayerName() {
        boost::json::value msg = { {RESPONSE_CODE, "invalidArgument"},
                            {RESPONSE_MESSAGE, "Invalid name"} };
        return boost::json::serialize(msg);
    };

    std::string PlayersListOnMap(const std::vector< std::weak_ptr<model::Player> >& players) {
        boost::json::value jv;
        boost::json::object& obj = jv.emplace_object();
        for (auto item : players) {
            auto player = item.lock();
            std::stringstream ss;
            ss << *(player->GetId());
            boost::json::value jv_item = { {RESPONSE_NAME, player->GetName()} };
            obj[ss.str()] = jv_item;
        }
        return boost::json::serialize(jv);
    };

    std::string InvalidMethod() {
        boost::json::value msg = { {RESPONSE_CODE, "invalidMethod"},
                            {RESPONSE_MESSAGE, "Invalid method"} };
        return boost::json::serialize(msg);
    };

    std::string EmptyAuthorization() {
        boost::json::value msg = { {RESPONSE_CODE, "invalidToken"},
                            {RESPONSE_MESSAGE, "Authorization header is missing"} };
        return boost::json::serialize(msg);
    };

    std::string UnknownToken() {
        boost::json::value msg = { {RESPONSE_CODE, "unknownToken"},
                            {RESPONSE_MESSAGE, "Player token has not been found"} };
        return boost::json::serialize(msg);
    };

    std::string JoinToGame(const std::string& token, size_t player_id) {
        boost::json::value msg = { {RESPONSE_AUTHTOKEN, token},
                            {RESPONSE_PLAYERID, player_id} };
        return boost::json::serialize(msg);
    };

    std::string OnlyPostMethodAllowed() {
        boost::json::value msg = { {RESPONSE_CODE, "invalidMethod"},
                            {RESPONSE_MESSAGE, "Only POST method is expected"} };
        return boost::json::serialize(msg);
    };

    std::optional< std::tuple<std::string, model::Map::Id> > ParseJoinToGame(const std::string& msg) {
        try {
            if (msg.empty()) {
                return std::nullopt;
            }
            boost::json::value jv = boost::json::parse(msg);
            if (!jv.is_object()) {
                return std::nullopt;
            }
            auto& obj = jv.as_object();
            if (obj.find(REQUEST_NAME) == obj.end() || obj.find(REQUEST_MAPID) == obj.end()) {
                return std::nullopt;
            }
            auto name_val = obj.at(REQUEST_NAME);
            auto mapid_val = obj.at(REQUEST_MAPID);
            if (!name_val.is_string() || !mapid_val.is_string()) {
                return std::nullopt;
            }
            std::string player_name = boost::json::value_to<std::string>(name_val);
            model::Map::Id map_id{ boost::json::value_to<std::string>(mapid_val) };
            return std::make_tuple(player_name, map_id);
        }
        catch (...) {
            return std::nullopt;
        }
    };

    std::string GameState(const std::vector<std::weak_ptr<model::Player>>& players) {
        boost::json::value jv;
        boost::json::object obj;
        for (const auto& item : players) {
            auto player = item.lock();
            auto dog = player->GetDog().lock();
            std::stringstream ss;
            ss << *(player->GetId());
            boost::json::array pos = {dog->GetPosition().x, dog->GetPosition().y};
            boost::json::array speed = {dog->GetSpeed().vx, dog->GetSpeed().vy};
            boost::json::value jv_item = {{RESPONSE_DOG_POSITION, pos},
                                          {RESPONSE_DOG_SPEED, speed},
                                          {RESPONSE_DOG_DIRECTION, std::string(model::DirectionToJson(dog->GetDirection()))}};
            obj[ss.str()] = jv_item;
        }
        jv.emplace_object()[RESPONSE_PLAYERS] = obj;
        return boost::json::serialize(jv);
    };

}