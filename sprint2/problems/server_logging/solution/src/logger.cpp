#include "logger.h"

#include <boost/log/core.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace js = boost::json;

void LogJson(const std::string& message, const js::object& data) {
    // Получаем текущее время
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()) % 1000000;
    
    std::tm tm_now;
    gmtime_r(&time_t_now, &tm_now);
    
    // Форматируем timestamp в ISO 8601 с микросекундами
    std::ostringstream timestamp_ss;
    timestamp_ss << std::put_time(&tm_now, "%Y-%m-%dT%H:%M:%S");
    timestamp_ss << '.' << std::setfill('0') << std::setw(6) << ms.count();
    
    // Формируем JSON (порядок полей: timestamp, data, message - как в примере)
    js::object obj;
    obj["timestamp"] = timestamp_ss.str();
    obj["data"] = data;
    obj["message"] = message;
    
    // Выводим в stdout
    std::cout << js::serialize(obj) << "\n";
    std::cout.flush();
}

void InitLogger() {
    // Для этого задания мы используем простую функцию LogJson вместо Boost.Log
    // так как API add_value недоступен в Boost 1.78
}
