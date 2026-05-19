#pragma once

#include <boost/json.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string_view>
#include <string>
#include <iostream>
#include <mutex>
#include "logging_handler.h"

namespace logger {

namespace json = boost::json;

// Мьютекс для потокобезопасного вывода логов.
inline std::mutex& GetLogMutex() {
    static std::mutex m;
    return m;
}

// Инициализация логгера (заглушка).
inline void InitLogger() {
    // В будущем: настройка уровня логирования, вывод в файл и т.д.
}

// Форматирует сообщение с меткой времени и данными.
template <class T>
std::string CreateLogMessage(std::string_view msg, const T& data) {
    json::object obj;
    obj["timestamp"] = boost::posix_time::to_iso_extended_string(
        boost::posix_time::microsec_clock::local_time());
    obj["message"] = std::string(msg);
    obj["data"] = json::value_from(data);
    return json::serialize(obj);
}

// Логирует сообщение без данных.
inline void LogInfo(std::string_view msg) {
    json::object obj;
    obj["timestamp"] = boost::posix_time::to_iso_extended_string(
        boost::posix_time::microsec_clock::local_time());
    obj["message"] = std::string(msg);

    std::lock_guard lock(GetLogMutex());
    std::clog << json::serialize(obj) << std::endl;
}

// Логирует сообщение с данными.
template <typename T>
void LogInfo(std::string_view msg, const T& data) {
    std::lock_guard lock(GetLogMutex());
    std::clog << CreateLogMessage(msg, data) << std::endl;
}

// Логирует ошибку с данными.
template <typename T>
void LogError(std::string_view msg, const T& data) {
    std::lock_guard lock(GetLogMutex());
    std::clog << CreateLogMessage(msg, data) << std::endl;
}

}  // namespace logger