#include "logger.h"
#include <iostream>
#include <mutex>

namespace logger {

namespace {

// Глобальный мьютекс для синхронизации доступа к логам.
// Размещён в анонимном пространстве имён для инкапсуляции.
// Используется для защиты от гонки данных при многопоточной записи.
std::mutex& GetLogMutex() {
    static std::mutex m;
    return m;
}

}  // namespace

void InitLogger() {
}

void LogInfo(std::string_view msg) {
    json::object obj;
    obj["timestamp"] = boost::posix_time::to_iso_extended_string(
        boost::posix_time::microsec_clock::local_time());
    obj["message"] = std::string(msg);

    std::lock_guard lock(GetLogMutex());
    std::clog << json::serialize(obj) << std::endl;
}

}  // namespace logger