#include "urlencode.h"
#include <string>

static bool IsReserved(char c) {
    return c == '!' || c == '#' || c == '$' || c == '&' || c == '\'' || c == '(' ||
           c == ')' || c == '*' || c == ',' || c == '/' || c == ':' ||
           c == ';' || c == '=' || c == '?' || c == '@' || c == '[' ||
           c == ']';
}

static bool IsUnreserved(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' ||
           c == '~';
}

static char ToHexUpper(int value) {
    return value < 10 ? '0' + value : 'A' + (value - 10);
}

std::string UrlEncode(std::string_view str) {
    std::string result;
    result.reserve(str.size() * 3);

    for (unsigned char c : str) {
        if (c == ' ') {
            result += '+';
        } else if (c < 32 || c >= 128 || IsReserved(c)) {
            result += '%';
            result += ToHexUpper(c / 16);
            result += ToHexUpper(c % 16);
        } else {
            result += c;
        }
    }

    return result;
}
