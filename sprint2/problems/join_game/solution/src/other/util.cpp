#include "../other/util.h"
#include <string>

namespace requestHandler {

    const std::string BEARER = "Bearer";
    const size_t TOKEN_SIZE = 32;
    const size_t AUTHORIZATION_NUMBER_PARTS = 2;
    const size_t BEARER_INDEX = 0;
    const size_t TOKEN_INDEX = 1;

    /*С бустовским split почему-то валятся тесты,
    хотя при дебаге всё работает как надо. Оставил пока как есть.*/
    std::vector<std::string_view> SplitStr(std::string_view str) {
        std::vector<std::string_view> result;
        std::string delim = "/";
        if (str.empty() || str == delim) return result;
        auto tmpStr = str.substr(1);
        auto start = 0U;
        auto end = tmpStr.find(delim);
        while (end != std::string::npos)
        {
            result.push_back(tmpStr.substr(start, end - start));
            start = end + delim.length();
            end = tmpStr.find(delim, start);
        }
        result.push_back(tmpStr.substr(start, end));
        return result;
    };

    //https://ru.hexlet.io/qna/glossary/questions/bearer-token-chto-eto
    //Будем использовать bearer токен
    std::string GetBearerToken(std::string_view bearer_string) {
        std::string token;
        std::vector<std::string_view> splitted;
        std::string delim = " ";
        if (bearer_string.empty() or bearer_string == delim) {
            return token;
        }
        auto start = 0U;
        auto end = bearer_string.find(delim, start);
        while (end != std::string::npos) {
            splitted.push_back(bearer_string.substr(start, end - start));
            start = end + delim.length();
            end = bearer_string.find(delim, start);
        }
        splitted.push_back(bearer_string.substr(start, end));
        if (splitted.size() != AUTHORIZATION_NUMBER_PARTS ||
            splitted[BEARER_INDEX] != BEARER ||
            splitted[TOKEN_INDEX].size() != TOKEN_SIZE) {
            return token;
        }
        return std::string(splitted[TOKEN_INDEX]);
    };

}//namespace requestHandler 