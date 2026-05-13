#pragma once

#include <string>
#include <optional>

class JsonParser {
public:
    static std::optional<std::string> ExtractStringValue(const std::string& json, const std::string& key);
    static std::optional<int> ExtractIntValue(const std::string& json, const std::string& key);
    static bool ExtractNullValue(const std::string& json, const std::string& key);
};
