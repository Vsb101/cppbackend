#pragma once

#include <filesystem>

#include "game.h"
#include "http_handler_types.h"
#include "join_use_case.h"
#include "players_use_case.h"
#include "state_use_case.h"
#include "player_use_case.h"
#include "tick_use_case.h"
#include "records_use_case.h"
#include "extra_data.h"

namespace model {
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Office const& office);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Building const& building);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Road const& road);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Position const& pos);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Speed const& speed);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Dog const& dog);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Item const& item);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, std::shared_ptr<Item> const& item);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, ItemInBag<Item> const& item);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Map const& map);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, std::vector<Map> const& maps);
} // namespace model

namespace extra_data {
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, ExtendedMap const& maps_loot_types);
}

namespace loot_gen {
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, LootGeneratorInfo const& loot_gen_info);
}

namespace app {
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, JoinGameResult const& join_result);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, PlayerInfo const& player);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, ListPlayersResult const& players_result);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, GetStateResult const& state_result);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, PlayerActionResult const& action_result);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, TickResult const& action_result);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, RecordsResult const& action_result);
} // namespace app

namespace json_loader {
    std::pair<model::Game, extra_data::MapsLootTypes> LoadGame(const std::filesystem::path& json_path);
    bool ReadJoinParamsFromString(http_handler::JoinParams& params, std::string str);
    bool ReadPlayerActionParamsFromString(http_handler::PlayerActionParams& params, std::string str);
    bool ReadTickParamsFromString(http_handler::TickParams& params, std::string str);
}  // namespace json_loader
