#pragma once

#include "model/model.h"
#include <string>

#include <boost/json.hpp>

namespace json_builder {

std::string BuildMap(const model::Map& map);

std::string BuildMapsList(const model::Game& game);

} // namespace json_builder