#pragma once

#include <boost/log/trivial.hpp>
#include <boost/json.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string_view>
#include <string>
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
    BOOST_LOG_TRIVIAL(info) << json::serialize(obj);
}

template <typename T>
inline void LogInfo(std::string_view msg, const T& data) {
    BOOST_LOG_TRIVIAL(info) << CreateLogMessage(msg, data);
}

template <typename T>
inline void LogError(std::string_view msg, const T& data) {
    BOOST_LOG_TRIVIAL(error) << CreateLogMessage(msg, data);
}

}  // namespace logger
