#pragma once

#include <boost/json.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string_view>
#include <string>
#include "logging_handler.h"

namespace logger {

namespace json = boost::json;

/**
 * @brief Инициализация системы логирования.
 * 
 * Текущая реализация является заглушкой и не выполняет никаких действий.
 * В будущем может быть расширена для настройки уровня логирования,
 * вывода в файл или другие источники.
 */
void InitLogger();

/**
 * @brief Создает JSON-сообщение лога с временной меткой.
 * 
 * @tparam T Тип данных для сериализации
 * @param msg Текстовое сообщение
 * @param data Данные для включения в лог
 * @return Сериализованная JSON-строка
 * 
 * Формат выходного сообщения:
 * {
 *   "timestamp": "2026-04-20T12:00:00.000000",
 *   "message": "текст сообщения",
 *   "data": { ... }
 * }
 */
template <class T>
std::string CreateLogMessage(std::string_view msg, const T& data) {
    json::object obj;
    obj["timestamp"] = boost::posix_time::to_iso_extended_string(
        boost::posix_time::microsec_clock::local_time());
    obj["message"] = std::string(msg);
    obj["data"] = json::value_from(data);
    return json::serialize(obj);
}

/**
 * @brief Логирование информационного сообщения без дополнительных данных.
 * 
 * @param msg Сообщение для логирования
 * 
 * Форматирует сообщение в JSON и записывает в стандартный поток ошибок.
 * Использует мьютекс для потокобезопасности.
 */
void LogInfo(std::string_view msg);

/**
 * @brief Логирование информационного сообщения с данными.
 * 
 * @tparam T Тип данных для сериализации
 * @param msg Сообщение для логирования
 * @param data Данные для включения в лог
 * 
 * Полная версия LogInfo, включающая произвольные данные в сообщение лога.
 */
template <typename T>
void LogInfo(std::string_view msg, const T& data) {
    std::clog << CreateLogMessage(msg, data) << std::endl;
}

/**
 * @brief Логирование сообщения об ошибке с данными.
 * 
 * @tparam T Тип данных для сериализации
 * @param msg Сообщение об ошибке
 * @param data Данные для включения в лог
 * 
 * В текущей реализации аналогичен LogInfo, но может быть расширен
 * для вывода в отдельный поток ошибок в будущем.
 */
template <typename T>
void LogError(std::string_view msg, const T& data) {
    std::clog << CreateLogMessage(msg, data) << std::endl;
}

}  // namespace logger