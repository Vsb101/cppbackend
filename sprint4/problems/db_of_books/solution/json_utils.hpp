#pragma once

#include <string>
#include <optional>
#include <sstream>
#include <iomanip>

namespace JsonUtils {

inline std::string EscapeJsonString(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

inline std::string FormatJsonBool(bool value) {
    return value ? "true" : "false";
}

inline std::string FormatNullableString(const std::optional<std::string>& value) {
    if (!value) {
        return "null";
    }
    return "\"" + EscapeJsonString(*value) + "\"";
}

} // namespace JsonUtils
