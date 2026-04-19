#pragma once

/**
 * @file logger.h
 * @brief Инициализация и утилиты логирования
 */

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "../logger/logging_handler.h"

#include <string>
#include <string_view>

namespace logger {

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = logging::expressions;
namespace json = boost::json;

/**
 * @brief Инициализирует логгер с консольным выводом в JSON-формате
 */
void InitLogger();

/**
 * @brief Создаёт JSON-сообщение лога
 * 
 * @param msg Текст сообщения
 * @param data Данные для сериализации в JSON
 * @return JSON-строка
 */
template <class T>
std::string CreateLogMessage(std::string_view msg, const T& data) {
    const auto timestamp = boost::posix_time::to_iso_extended_string(
        boost::posix_time::microsec_clock::local_time());
    
    json::value json_data = json::value_from(data);
    
    return json::serialize(json::value{
        {"timestamp", timestamp},
        {"data", json_data},
        {"message", std::string(msg)}
    });
}

/**
 * @brief Логирование информационного сообщения с JSON-данными
 *
 * @param msg Текст сообщения
 * @param data Данные для сериализации в JSON
 */
template <typename T>
inline void LogInfo(std::string_view msg, const T& data) {
    BOOST_LOG_TRIVIAL(info) << CreateLogMessage(msg, data);
}

/**
 * @brief Логирование ошибки с JSON-данными
 *
 * @param msg Текст сообщения
 * @param data Данные для сериализации в JSON
 */
template <typename T>
inline void LogError(std::string_view msg, const T& data) {
    BOOST_LOG_TRIVIAL(error) << CreateLogMessage(msg, data);
}

/**
 * @brief Логирование предупреждения с JSON-данными
 *
 * @param msg Текст сообщения
 * @param data Данные для сериализации в JSON
 */
template <typename T>
inline void LogWarning(std::string_view msg, const T& data) {
    BOOST_LOG_TRIVIAL(warning) << CreateLogMessage(msg, data);
}

}  // namespace logger