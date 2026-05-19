#pragma once

namespace json_field {
    // Game
    constexpr static char GAME_DEFAULT_DOG_SPEED[]     = "defaultDogSpeed";
    constexpr static char GAME_DEFAULT_BAG_CAPACITY[]  = "defaultBagCapacity";
    constexpr static char GAME_MAPS[]                  = "maps";
    constexpr static char GAME_LOOT_GENERATOR_CONFIG[] = "lootGeneratorConfig";
    constexpr static char GAME_DOG_RETIREMENT_TIME[]   = "dogRetirementTime";
    // LootGeneratorInfo
    constexpr static char LOOT_GENERATOR_CONFIG_PERIOD[]      = "period";
    constexpr static char LOOT_GENERATOR_CONFIG_PROBABILITY[] = "probability";
    // Map
    constexpr static char MAP_DOG_SPEED[]    = "dogSpeed";
    constexpr static char MAP_BAG_CAPACITY[] = "bagCapacity";
    constexpr static char MAP_ID[]           = "id";
    constexpr static char MAP_NAME[]         = "name";
    constexpr static char MAP_ROADS[]        = "roads";
    constexpr static char MAP_BUILDINGS[]    = "buildings";
    constexpr static char MAP_OFFICES[]      = "offices";
    constexpr static char MAP_LOOT_TYPES[]   = "lootTypes";
    // Dog
    constexpr static char DOG_POSITION[]  = "pos";
    constexpr static char DOG_SPEED[]     = "speed";
    constexpr static char DOG_DIRECTION[] = "dir";
    // Player
    constexpr static char PLAYER_BAG[]    = "bag";
    constexpr static char PLAYER_SCORE[]  = "score";
    // Item
    constexpr static char ITEM_ID[]        = "id";
    constexpr static char ITEM_TYPE[]      = "type";
    constexpr static char ITEM_POSITION[]  = "pos";
    // Road
    constexpr static char ROAD_START_X[] = "x0";
    constexpr static char ROAD_START_Y[] = "y0";
    constexpr static char ROAD_END_X[]   = "x1";
    constexpr static char ROAD_END_Y[]   = "y1";
    // Building
    constexpr static char BUILDING_POS_X[]  = "x";
    constexpr static char BUILDING_POS_Y[]  = "y";
    constexpr static char BUILDING_SIZE_W[] = "w";
    constexpr static char BUILDING_SIZE_H[] = "h";
    // Office
    constexpr static char OFFICE_ID[]        = "id";
    constexpr static char OFFICE_POS_X[]     = "x";
    constexpr static char OFFICE_POS_Y[]     = "y";
    constexpr static char OFFICE_OFFSET_DX[] = "offsetX";
    constexpr static char OFFICE_OFFSET_DY[] = "offsetY";
    // Errors
    constexpr static char ERROR_CODE[]    = "code";
    constexpr static char ERROR_MESSAGE[] = "message";
    // Logger
    constexpr static char LOGGER_TIMESTAMP[] = "timestamp";
    constexpr static char LOGGER_THREAD[]    = "thread";
    constexpr static char LOGGER_DATA[]      = "data";
    constexpr static char LOGGER_MESSAGE[]   = "message";
    // Server
    constexpr static char SERVER_PORT[]           = "port";
    constexpr static char SERVER_ADDRESS[]        = "address";
    constexpr static char REQUEST_IP[]            = "ip";
    constexpr static char REQUEST_URI[]           = "URI";
    constexpr static char REQUEST_METHOD[]        = "method";
    constexpr static char RESPONSE_TIME[]         = "response_time";
    constexpr static char RESPONSE_CODE[]         = "code";
    constexpr static char RESPONSE_CONTENT_TYPE[] = "content_type";
    // API
    constexpr static char API_CODE_BAD_REQUEST[]      = "badRequest";
    constexpr static char API_CODE_INVALID_ARGUMENT[] = "invalidArgument";
    constexpr static char API_CODE_INVALID_METHOD[]   = "invalidMethod";
    constexpr static char API_CODE_INVALID_TOKEN[]    = "invalidToken";
    constexpr static char API_CODE_UNKNOWN_TOKEN[]    = "unknownToken";
    constexpr static char API_CODE_MAP_NOT_FOUND[]    = "mapNotFound";
    // JoinParams
    constexpr static char JOIN_NAME[]   = "userName";
    constexpr static char JOIN_MAP_ID[] = "mapId";
    // JoinResult
    constexpr static char JOIN_TOKEN[]  = "authToken";
    constexpr static char JOIN_PLAYER_ID[]  = "playerId";
    // ListPlayers
    constexpr static char LIST_PLAYERS_NAME[]  = "name";
    // GetState
    constexpr static char GET_STATE_PLAYERS[]  = "players";
    constexpr static char GET_STATE_LOOT[]     = "lostObjects";
    // PlayerActionParams
    constexpr static char PLAYER_ACTION_MOVE_DIRECTION[]  = "move";
    // TickParams
    constexpr static char TICK_DT[]  = "timeDelta";
    // RecordsResult
    constexpr static char RECORD_NAME[]  = "name";
    constexpr static char RECORD_SCORE[] = "score";
    constexpr static char RECORD_TIME[]  = "playTime";
}
