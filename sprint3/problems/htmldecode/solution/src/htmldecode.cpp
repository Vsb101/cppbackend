#include "htmldecode.h"
#include <string>
#include <unordered_map>
#include <algorithm>

static bool isAllLower(std::string_view s) {
    for (char c : s) {
        if (c >= 'A' && c <= 'Z') return false;
    }
    return !s.empty();
}

static bool isAllUpper(std::string_view s) {
    for (char c : s) {
        if (c >= 'a' && c <= 'z') return false;
    }
    return !s.empty();
}

static bool isValidMnemonic(std::string_view s) {
    return isAllLower(s) || isAllUpper(s);
}

static std::string toLower(std::string_view s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        if (c >= 'A' && c <= 'Z') {
            result += static_cast<char>(c - 'A' + 'a');
        } else {
            result += c;
        }
    }
    return result;
}

std::string HtmlDecode(std::string_view str) {
    static const std::unordered_map<std::string, char> entities = {
        {"lt", '<'},
        {"gt", '>'},
        {"amp", '&'},
        {"apos", '\''},
        {"quot", '"'}
    };
    
    std::string result;
    result.reserve(str.size());
    
    size_t i = 0;
    while (i < str.size()) {
        if (str[i] == '&' && i + 1 < str.size()) {
            // Ищем точку с запятой
            size_t semiPos = str.find(';', i + 1);
            
            if (semiPos != std::string_view::npos) {
                // Есть точка с запятой - проверяем мнемоника между & и ;
                std::string_view entityName = str.substr(i + 1, semiPos - i - 1);
                
                if (isValidMnemonic(entityName)) {
                    std::string lowerEntity = toLower(entityName);
                    auto it = entities.find(lowerEntity);
                    if (it != entities.end()) {
                        result += it->second;
                        i = semiPos + 1;
                        continue;
                    }
                }
            }
            
            // Пробуем без точки с запятой - ищем до следующего & или конца строки
            size_t nextAmp = str.find('&', i + 1);
            if (nextAmp == std::string_view::npos) {
                nextAmp = str.size();
            }
            
            std::string_view entityName = str.substr(i + 1, nextAmp - i - 1);
            
            if (isValidMnemonic(entityName)) {
                std::string lowerEntity = toLower(entityName);
                auto it = entities.find(lowerEntity);
                if (it != entities.end()) {
                    result += it->second;
                    i = nextAmp;
                    continue;
                }
            }
            
            // Не мнемоника - добавляем & и продолжаем
            result += '&';
            ++i;
        } else {
            result += str[i];
            ++i;
        }
    }
    
    return result;
}
