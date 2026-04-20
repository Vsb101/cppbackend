#pragma once

#include <boost/json.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string_view>
#include <string>
#include <iostream>
#include "logging_handler.h"

namespace logger {

namespace json = boost::json;

void InitLogger();

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
 * @brief Перегрузка для сообщений без дополнительных данных
 */
inline void LogInfo(std::string_view msg) {
    json::object obj;
    obj["timestamp"] = boost::posix_time::to_iso_extended_string(
        boost::posix_time::microsec_clock::local_time());
    obj["message"] = std::string(msg);
    std::clog << json::serialize(obj) << std::endl;
}

template <typename T>
inline void LogInfo(std::string_view msg, const T& data) {
    std::clog << CreateLogMessage(msg, data) << std::endl;
}

template <typename T>
inline void LogError(std::string_view msg, const T& data) {
    std::clog << CreateLogMessage(msg, data) << std::endl;
}

}  // namespace logger
