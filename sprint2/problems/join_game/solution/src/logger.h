#pragma once

#include <boost/json.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <string>

namespace js = boost::json;

/**
 * @brief Форматирует лог-запись в JSON и выводит в stdout
 *
 * Формат: {"timestamp":"...","message":"...","data":{...}}
 */
void LogJson(const std::string& message, const js::object& data);

/**
 * @brief Инициализирует логгер
 *
 * Настраивает Boost.Log для вывода в стандартный поток stdout
 */
void InitLogger();
