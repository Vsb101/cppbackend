#include "json_parser.hpp"
#include <cctype>

using namespace std::literals; // Позволяет использовать литерал ""sv

static size_t FindKeyPosition(std::string_view json, std::string_view key) {
    size_t pos = 0;
    while ((pos = json.find(key, pos)) != std::string_view::npos) {
        if (pos > 0 && json[pos - 1] == '"' && 
            pos + key.length() < json.length() && json[pos + key.length()] == '"') {
            return pos - 1;
        }
        pos += key.length();
    }
    return std::string_view::npos;
}

std::optional<std::string> JsonParser::ExtractStringValue(std::string_view json, std::string_view key) {
    auto key_pos = FindKeyPosition(json, key);
    if (key_pos == std::string_view::npos) return std::nullopt;
    
    auto colon_pos = json.find(':', key_pos + key.length() + 2);
    if (colon_pos == std::string_view::npos) return std::nullopt;
    
    auto value_start = json.find('"', colon_pos + 1);
    if (value_start == std::string_view::npos) return std::nullopt;
    
    size_t value_end = std::string_view::npos;
    size_t slashes = 0;
    for (size_t i = value_start + 1; i < json.length(); ++i) {
        if (json[i] == '\\') {
            ++slashes;
        } else if (json[i] == '"') {
            if (slashes % 2 == 0) {
                value_end = i;
                break;
            }
            slashes = 0;
        } else {
            slashes = 0;
        }
    }
    
    if (value_end == std::string_view::npos) return std::nullopt;
    
    std::string_view raw_value = json.substr(value_start + 1, value_end - value_start - 1);
    
    std::string decoded;
    decoded.reserve(raw_value.length());
    for (size_t i = 0; i < raw_value.length(); ++i) {
        if (raw_value[i] == '\\' && i + 1 < raw_value.length()) {
            switch (raw_value[i + 1]) {
                case '"':  decoded += '"';  break;
                case '\\': decoded += '\\'; break;
                case '/':  decoded += '/';  break;
                case 'b':  decoded += '\b'; break;
                case 'f':  decoded += '\f'; break;
                case 'n':  decoded += '\n'; break;
                case 'r':  decoded += '\r'; break;
                case 't':  decoded += '\t'; break;
                default:   decoded += raw_value[i + 1]; break;
            }
            ++i;
        } else {
            decoded += raw_value[i];
        }
    }
    
    return decoded;
}

std::optional<int> JsonParser::ExtractIntValue(std::string_view json, std::string_view key) {
    auto key_pos = FindKeyPosition(json, key);
    if (key_pos == std::string_view::npos) return std::nullopt;
    
    auto colon_pos = json.find(':', key_pos + key.length() + 2);
    if (colon_pos == std::string_view::npos) return std::nullopt;
    
    // C++20: Отрезаем обработанную часть, двигаясь по string_view вперед без индексов
    std::string_view tail = json.substr(colon_pos + 1);
    while (!tail.empty() && (tail.front() == ' ' || tail.front() == '\t')) {
        tail.remove_prefix(1);
    }
    
    if (tail.empty()) return std::nullopt;
    
    // C++20 фича: использование читаемого .starts_with() с ""sv литералом
    if (tail.starts_with("null"sv)) {
        return std::nullopt;
    }
    
    bool is_negative = false;
    if (tail.front() == '-') {
        is_negative = true;
        tail.remove_prefix(1);
    }
    
    if (tail.empty() || !std::isdigit(tail.front())) {
        return std::nullopt;
    }
    
    int num = 0;
    while (!tail.empty() && std::isdigit(tail.front())) {
        num = num * 10 + (tail.front() - '0');
        tail.remove_prefix(1);
    }
    
    return is_negative ? -num : num;
}

bool JsonParser::ExtractNullValue(std::string_view json, std::string_view key) {
    auto key_pos = FindKeyPosition(json, key);
    if (key_pos == std::string_view::npos) return false;
    
    auto colon_pos = json.find(':', key_pos + key.length() + 2);
    if (colon_pos == std::string_view::npos) return false;
    
    std::string_view tail = json.substr(colon_pos + 1);
    while (!tail.empty() && (tail.front() == ' ' || tail.front() == '\t')) {
        tail.remove_prefix(1);
    }
    
    // C++20: Заменили громоздкий .compare() на лаконичный .starts_with()
    return tail.starts_with("null"sv);
}
