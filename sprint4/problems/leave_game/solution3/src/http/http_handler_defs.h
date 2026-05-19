#pragma once

#include <string_view>

namespace api_strings {
    using namespace std::literals;
    // --- LVL 0 --- // API
    constexpr static int              MAIN_POS     = 0;
    constexpr static std::string_view MAIN_PATH    = "/api/"sv;
    // --- LVL 1 --- // Version
    constexpr static int              VERSION_POS  = 1;
    constexpr static std::string_view VERSION_PATH = "v1"sv;
    // --- LVL 2 --- // Target
    constexpr static int              TARGET_POS   = 2;
    constexpr static std::string_view MAPS_PATH    = "maps"sv;
    constexpr static std::string_view GAME_PATH    = "game"sv;
    // --- LVL 3 --- // Action
    constexpr static int              LVL3_POS     = 3;
    constexpr static std::string_view JOIN_PATH    = "join"sv;
    constexpr static std::string_view PLAYERS_PATH = "players"sv;
    constexpr static std::string_view STATE_PATH   = "state"sv;
    constexpr static std::string_view PLAYER_PATH  = "player"sv;
    constexpr static std::string_view TICK_PATH    = "tick"sv;
    constexpr static std::string_view RECORDS_PATH = "records"sv;
    // --- LVL 4 --- // 
    constexpr static int              LVL4_POS     = 4;
    constexpr static std::string_view ACTION_PATH  = "action"sv;
}

namespace ContentType {
    using namespace std::literals;
    constexpr static std::string_view APP_JSON          = "application/json"sv;
    constexpr static std::string_view APP_OCT_STREAM    = "application/octet-stream"sv;
    constexpr static std::string_view APP_XML           = "application/xml"sv;
    constexpr static std::string_view AUDIO_MPEG        = "audio/mpeg"sv;
    constexpr static std::string_view IMG_BMP           = "image/bmp"sv;
    constexpr static std::string_view IMG_GIF           = "image/gif"sv;
    constexpr static std::string_view IMG_ICON          = "image/vnd.microsoft.icon"sv;
    constexpr static std::string_view IMG_JPEG          = "image/jpeg"sv;
    constexpr static std::string_view IMG_PNG           = "image/png"sv;
    constexpr static std::string_view IMG_SVG           = "image/svg+xml"sv;
    constexpr static std::string_view IMG_TIFF          = "image/tiff"sv;
    constexpr static std::string_view TEXT_CSS          = "text/css"sv;
    constexpr static std::string_view TEXT_HTML         = "text/html"sv;
    constexpr static std::string_view TEXT_JS           = "text/javascript"sv;
    constexpr static std::string_view TEXT_PLAIN        = "text/plain"sv;
};

namespace AllowedMethods {
    using namespace std::literals;
    constexpr static std::string_view MAPS          = "GET, HEAD"sv;
    constexpr static std::string_view JOIN          = "POST"sv;
    constexpr static std::string_view PLAYERS       = "GET, HEAD"sv;
    constexpr static std::string_view STATE         = "GET, HEAD"sv;
    constexpr static std::string_view PLAYER_ACTION = "POST"sv;
    constexpr static std::string_view TICK          = "POST"sv;
    constexpr static std::string_view RECORDS       = "GET, HEAD"sv;
    // Для единообразия. Для ошибки на самом деле не нужен допустимый метод.
    constexpr static std::string_view ERROR   = "GET, HEAD"sv;
};

namespace HttpFildsValue {
    using namespace std::literals;
    constexpr static std::string_view NO_CACHE = "no-cache"sv;
};

namespace TokenParams {
    using namespace std::literals;
    constexpr static std::string_view START_STR = "Bearer "sv;
}