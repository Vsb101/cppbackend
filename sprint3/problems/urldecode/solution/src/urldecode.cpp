#include <string>
#include <string_view>
#include <stdexcept>
#include <cctype>

std::string UrlDecode(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    
    auto hex_to_int = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%') {
            if (i + 2 >= str.size()) {
                throw std::invalid_argument("Incomplete percent-encoding");
            }
            char high = str[i + 1];
            char low = str[i + 2];
            int high_val = hex_to_int(high);
            int low_val = hex_to_int(low);
            if (high_val == -1 || low_val == -1) {
                throw std::invalid_argument("Invalid percent-encoding");
            }
            result += static_cast<char>(high_val * 16 + low_val);
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    
    return result;
}
