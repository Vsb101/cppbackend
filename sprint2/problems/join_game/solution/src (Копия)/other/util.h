#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace util {

/**
 * @brief Разбивает строку на сегменты по разделителю '/'
 * 
 * Пример: "/api/v1/maps" -> ["api", "v1", "maps"]
 * 
 * @param str Входная строка
 * @return Вектор строковых представлений сегментов
 */
[[nodiscard]] std::vector<std::string_view> SplitStr(std::string_view str);

/**
 * @brief Извлекает Bearer токен из строки авторизации
 * 
 * Ожидает формат: "Bearer {32-символьный-токен}"
 * 
 * @param bearer_string Строка авторизации
 * @return Токен или пустая строка при ошибке
 */
[[nodiscard]] std::string GetBearerToken(std::string_view bearer_string);

}  // namespace util
