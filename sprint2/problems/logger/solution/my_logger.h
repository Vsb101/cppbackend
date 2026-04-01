#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <filesystem>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    // 1. Сначала все приватные вспомогательные методы
    std::chrono::system_clock::time_point GetTime() const {
        return manual_ts_.value_or(std::chrono::system_clock::now());
    }

    std::string GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#ifdef _WIN32
        gmtime_s(&tm, &t_c); // Используем gmtime вместо localtime
#else
        gmtime_r(&t_c, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    std::string GetFileTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#ifdef _WIN32
        gmtime_s(&tm, &t_c); // Здесь тоже gmtime
#else
        gmtime_r(&t_c, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y_%m_%d");
        return oss.str();
    }


    void CheckAndOpenNewFile() {
        // Используем актуальную дату (из SetTimestamp или текущую)
        std::string suffix = GetFileTimeStamp();
        std::string new_filename = "/var/log/sample_log_" + suffix + ".log";
        
        if (new_filename != current_filename_) {
            log_file_.close();
            // Очищаем поток перед открытием нового файла
            log_file_.clear(); 
            
            std::filesystem::create_directories("/var/log");
            // Открываем в режиме ios::app, как просит задание
            log_file_.open(new_filename, std::ios::app);
            current_filename_ = std::move(new_filename);
        }
    }


    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 2. Затем приватные поля
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::ofstream log_file_;
    std::string current_filename_;
    mutable std::mutex mutex_;

public: // 3. В самом конце публичные методы (шаблоны теперь видят всё, что выше)
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        CheckAndOpenNewFile();
        
        // Попробуйте убрать пробел ПОСЛЕ двоеточия, если размер не совпадает
        // Но сначала проверьте этот вариант:
        log_file_ << GetTimeStamp() <<  ": "; 
        ((log_file_ << args), ...);
        log_file_ << "\n"; // Вместо std::endl
        log_file_.flush(); // Чтобы данные точно записались
    }


    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> lock(mutex_);
        manual_ts_ = ts;
    }
};
