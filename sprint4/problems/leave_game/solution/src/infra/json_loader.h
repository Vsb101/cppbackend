#pragma once

#include <filesystem>
#include "../model/game.h"
#include "../app/extra_data.h"

namespace json_loader {

struct ProjectConfig {
    model::Game game;
    app::ExtraData extra_data;
};

// ProjectConfig
ProjectConfig LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
