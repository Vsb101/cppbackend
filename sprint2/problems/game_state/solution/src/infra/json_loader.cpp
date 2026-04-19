#include <boost/json.hpp>
#include "json_loader.h"
#include "../api/json_serialization.h"
#include <fstream>

namespace json_loader {
namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path) {
    std::ifstream ifs(json_path);
    if (!ifs) throw std::runtime_error("Can't open " + json_path.string());

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    auto value = json::parse(content);
    
    model::Game game;
    // Используем tag_invoke из api/json_serialization.h
    auto maps = json::value_to<std::vector<model::Map>>(value.as_object().at("maps"));
    
    for (auto& map : maps) {
        game.AddMap(std::move(map));
    }
    return game;
}
} // namespace json_loader
