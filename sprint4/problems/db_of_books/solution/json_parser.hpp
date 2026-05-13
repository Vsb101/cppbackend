#pragma once

#include <string>
#include <optional>

class JsonParser {
public:
    static std::optional<std::string> ExtractStringValue(std::string_view json, std::string_view key);
    static std::optional<int> ExtractIntValue(std::string_view json, std::string_view key);
    static bool ExtractNullValue(std::string_view json, std::string_view key);
};
