#pragma once

#include <filesystem>
#include <string_view>

#include "model/model.h"

namespace json_loader {

void LoadGame(std::string_view json_path, model::Game& game);

}  // namespace json_loader
