#include "../json/json_loader.h"
#include "../logger/logger.h"

#include <fstream>
#include <iostream>
#include <string_view>
#include <sstream>

#include <iostream>

namespace json_loader {

    namespace json = boost::json;                                                                   //Тут бустом пользуемся
    using namespace std::literals;

    boost::json::value ReadJSONFile(const std::filesystem::path& json_path) {
        std::ifstream file(json_path);
        if (!file.is_open()) {
            BOOST_LOG_TRIVIAL(error) << logger::CreateLogMessage("error"sv,
                logger::ExceptionLog(EXIT_FAILURE,
                    "Error: Can not open current file"sv, "Invalid path"sv)); 
            throw std::invalid_argument("Invalid path, can not open file");                         //Всё плохо, передан кривой путь. 
        }

        std::stringstream ss;
        ss << file.rdbuf();
        boost::json::value root = boost::json::parse(ss.str());
        return root;
    };

    model::Game LoadGame(const std::filesystem::path& json_path) {
        // Загрузить содержимое файла json_path, например, в виде строки
        // Распарсить строку как JSON, используя boost::json::parse
        // Загрузить модель игры из файла
        model::Game game;
        boost::json::value jsonVal = ReadJSONFile(json_path);
        std::vector<model::Map> maps = boost::json::value_to< std::vector<model::Map> >(jsonVal.as_object().at(model::MAPS));
        game.AddMaps(maps);
        return game;
    };

}  // namespace json_loader