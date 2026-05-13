#include "json_parser.hpp"

std::optional<std::string> JsonParser::ExtractStringValue(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t key_pos = json.find(search_key);
    if (key_pos == std::string::npos) return std::nullopt;
    
    size_t colon_pos = json.find(':', key_pos + search_key.length());
    if (colon_pos == std::string::npos) return std::nullopt;
    
    size_t value_start = json.find('"', colon_pos + 1);
    if (value_start == std::string::npos) return std::nullopt;
    
    size_t value_end = value_start + 1;
    std::string value;
    while (value_end < json.length()) {
        if (json[value_end] == '"' && json[value_end - 1] != '\\') {
            break;
        }
        value += json[value_end];
        value_end++;
    }
    
    // Раскодирование экранированных символов
    std::string decoded;
    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '\\' && i + 1 < value.length()) {
            if (value[i + 1] == '"') decoded += '"';
            else if (value[i + 1] == '\\') decoded += '\\';
            else decoded += value[i + 1];
            i++;
        } else {
            decoded += value[i];
        }
    }
    
    return decoded;
}

std::optional<int> JsonParser::ExtractIntValue(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t key_pos = json.find(search_key);
    if (key_pos == std::string::npos) return std::nullopt;
    
    size_t colon_pos = json.find(':', key_pos + search_key.length());
    if (colon_pos == std::string::npos) return std::nullopt;
    
    size_t value_start = colon_pos + 1;
    while (value_start < json.length() && (json[value_start] == ' ' || json[value_start] == '\t')) {
        value_start++;
    }
    
    if (value_start >= json.length()) return std::nullopt;
    
    // Проверяем, не null ли значение
    if (json.substr(value_start, 4) == "null") {
        return std::nullopt;
    }
    
    // Читаем число
    std::string num_str;
    while (value_start < json.length() && std::isdigit(json[value_start])) {
        num_str += json[value_start];
        value_start++;
    }
    
    if (num_str.empty()) return std::nullopt;
    return std::stoi(num_str);
}

bool JsonParser::ExtractNullValue(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t key_pos = json.find(search_key);
    if (key_pos == std::string::npos) return false;
    
    size_t colon_pos = json.find(':', key_pos + search_key.length());
    if (colon_pos == std::string::npos) return false;
    
    size_t value_start = colon_pos + 1;
    while (value_start < json.length() && (json[value_start] == ' ' || json[value_start] == '\t')) {
        value_start++;
    }
    
    return json.substr(value_start, 4) == "null";
}
