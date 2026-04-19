#include "../json/json_loader.h"
#include "../logger/logger.h"

#include <fstream>
#include <sstream>
#include <string_view>
#include <string>

namespace json_loader {

namespace json = boost::json;
using namespace std::literals;

boost::json::value ReadJSONFile(const std::filesystem::path& json_path) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        logger::LogError("error",
            logger::ExceptionLog(EXIT_FAILURE,
                "Error: Can not open current file", "Invalid path"));
        throw std::invalid_argument("Invalid path, can not open file");
    }

    std::stringstream ss;
    ss << file.rdbuf();
    boost::json::value root = boost::json::parse(ss.str());
    return root;
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    model::Game game;
    boost::json::value json_val = ReadJSONFile(json_path);
    std::vector<model::Map> maps = boost::json::value_to<std::vector<model::Map>>(
        json_val.as_object().at(std::string(model::kMaps)));
    game.AddMaps(maps);
    return game;
}

}  // namespace json_loader