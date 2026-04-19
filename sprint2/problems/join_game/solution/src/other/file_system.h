#pragma once

#include <filesystem>

namespace util {

/**
 * @brief Проверяет, является ли путь подпутём базового
 * 
 * Пример: IsSubPath("/app/static/css/main.css", "/app/static") -> true
 * 
 * @param path Проверяемый путь
 * @param base Базовый путь
 * @return true если path начинается с base
 */
[[nodiscard]] bool IsSubPath(const std::filesystem::path& path,
                             const std::filesystem::path& base);

}  // namespace util