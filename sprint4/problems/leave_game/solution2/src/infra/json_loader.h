#pragma once

#include <filesystem>
#include "../model/game.h"
#include "../app/extra_data.h"

namespace json_loader {

struct ProjectConfig {
    model::Game game;
    app::ExtraData extra_data;
    double dog_retirement_time = 60.0;
};

// ProjectConfig
ProjectConfig LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
