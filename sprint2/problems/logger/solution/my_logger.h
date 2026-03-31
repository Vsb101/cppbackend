#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>
#include <filesystem>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        std::ostringstream oss;
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        oss << std::put_time(std::localtime(&t_c), "%Y_%m_%d");
        return oss.str();
    }

    // Проверяет, нужно ли открыть новый файл для записи
    void CheckAndOpenNewFile() {
        std::string new_filename = "/tmp/var/log/sample_log_" + GetFileTimeStamp() + ".log";
        if (new_filename != current_filename_) {
            if (log_file_.is_open()) {
                log_file_.close();
            }
            
            std::filesystem::create_directories("/tmp/var/log");
            log_file_.open(new_filename, std::ios::app);
            current_filename_ = new_filename;
        }
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        CheckAndOpenNewFile();
        log_file_ << GetTimeStamp() << ": ";
        ((log_file_ << args), ...);
        log_file_ << std::endl;
        log_file_.flush();
        

    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> lock(mutex_);
        manual_ts_ = ts;
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::ofstream log_file_;
    std::string current_filename_;
    mutable std::mutex mutex_;
};