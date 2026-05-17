#pragma once
#include <string_view>

namespace api {

// Константы путей API эндпоинтов.
// Все пути начинаются с префикса /api/v1
class Endpoints {
public:
    // Игровые сессии
    static constexpr std::string_view JoinGame() { return "/api/v1/game/join"; }
    static constexpr std::string_view Tick() { return "/api/v1/game/tick"; }
    static constexpr std::string_view GetState() { return "/api/v1/game/state"; }
    static constexpr std::string_view PlayerAction() { return "/api/v1/game/player/action"; }
    static constexpr std::string_view GetPlayers() { return "/api/v1/game/players"; }

    // Карты
    static constexpr std::string_view GetMaps() { return "/api/v1/maps"; }
    static constexpr std::string_view GetMap() { return "/api/v1/maps/"; }

    static constexpr std::string_view GetRecords() { return "/api/v1/game/records"; }

};

}  // namespace api
