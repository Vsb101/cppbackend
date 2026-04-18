/**
 * @file main.cpp
 * @brief Точка входа в игровой сервер
 */

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>
#include <tuple>
#include <vector>

#include "app/app.h"
#include "json/json_loader.h"
#include "logger/logger.h"
#include "other/sdk.h"
#include "request_handler/request_handler.h"

#define BOOST_USE_WINAPI_VERSION 0x0501

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace config {
    constexpr std::string_view kDefaultConfig = "data/config.json";
    constexpr std::string_view kDefaultStatic = "static";
    constexpr uint16_t kDefaultPort = 8080;
    constexpr std::string_view kUsageMessage = "Usage: game_server <config-json> <static-path>";
}  // namespace config

namespace {

/**
 * @brief Парсинг аргументов командной строки
 * 
 * @param argc Количество аргументов
 * @param argv Массив аргументов
 * @return std::optional<std::tuple<std::string, std::string>> Кортеж (config_path, static_path) или nullopt
 */
auto ParseArgs(int argc, const char* argv[])
    -> std::optional<std::tuple<std::string, std::string>> {
    if (argc != 3) {
        return std::nullopt;
    }
    return std::make_tuple(argv[1], argv[2]);
}

/**
 * @brief Проверка режима отладки через ENV переменную
 */
[[nodiscard]] bool IsDebugMode() {
    return std::getenv("DEBUG") != nullptr;
}

}  // namespace

int main(int argc, const char* argv[]) {
    logger::InitLogger();

    try {
        // Парсинг аргументов
        auto args = ParseArgs(argc, argv);
        if (!args) {
            logger::LogError(
                "error"sv,
                logger::ExceptionLog(EXIT_FAILURE, config::kUsageMessage, "Invalid arguments"));
            return EXIT_FAILURE;
        }
        const auto& [config_path, static_path] = args.value();

        // Загрузка карты из файла и построение модели игры
        model::Game game = IsDebugMode()
                               ? json_loader::LoadGame(std::string(config::kDefaultConfig))
                               : json_loader::LoadGame(config_path);

        // Установка пути до статического контента
        std::filesystem::path static_content_path = IsDebugMode()
                                                        ? std::string(config::kDefaultStatic)
                                                        : static_path;

        // 3. Инициализация io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        app::Application application(std::move(game), ioc);

        // Обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait(
            [&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
                if (!ec) {
                    logger::LogInfo("server was forcibly closed"sv, logger::ExitCodeLog(0));
                    ioc.stop();
                }
            });

        // Создание обработчика HTTP-запросов
        http_handler::RequestHandler handler{application, static_content_path};

        // Запуск HTTP-сервера
        const auto address = net::ip::make_address("0.0.0.0");
        http_server::ServeHttp(ioc, {address, config::kDefaultPort},
                               [&handler](auto&& req, auto&& send) {
                                   handler(std::forward<decltype(req)>(req),
                                           std::forward<decltype(send)>(send));
                               });

        logger::LogInfo("server started"sv,
                        logger::ServerAddrPortLog(address.to_string(), config::kDefaultPort));

        // Запуск обработки асинхронных операций
        unsigned worker_threads = std::max(1u, num_threads);
        std::vector<std::jthread> workers;
        workers.reserve(worker_threads - 1);
        while (--worker_threads) {
            workers.emplace_back([&ioc] { ioc.run(); });
        }
        ioc.run();
    } catch (const std::exception& ex) {
        logger::LogError("error"sv,
                         logger::ExceptionLog(EXIT_FAILURE, "Server not started"sv, ex.what()));
        return EXIT_FAILURE;
    }
}