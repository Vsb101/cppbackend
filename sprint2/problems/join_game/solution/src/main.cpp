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
#include <vector>

#include "app/app.h"
#include "infra/json_loader.h"
#include "logger/logger.h"
#include "request_handler.h"
#include "other/sdk.h"
#include <boost/asio/ip/tcp.hpp>
#include "infra/http_server.h" 

using namespace std::string_literals; // Для "string"s
using namespace std::string_view_literals; // Для "view"sv
namespace net = boost::asio;
namespace sys = boost::system;

namespace config {
    constexpr std::string_view kDefaultConfig = "data/config.json";
    constexpr std::string_view kDefaultStatic = "static";
    constexpr uint16_t kDefaultPort = 8080;
}

namespace {

// Парсинг аргументов командной строки
auto ParseArgs(int argc, const char* argv[])
    -> std::optional<std::pair<std::string, std::string>> {
    if (argc != 3) {
        return std::nullopt;
    }
    return std::make_pair(argv[1], argv[2]);
}

}  // namespace

int main(int argc, const char* argv[]) {
    logger::InitLogger();

    try {
        // 1. Парсинг аргументов
        auto args = ParseArgs(argc, argv);
        if (!args) {
            std::cerr << "Usage: game_server <config-json> <static-path>" << std::endl;
            return EXIT_FAILURE;
        }
        const auto& [config_path, static_path] = args.value();

        // 2. Загрузка модели игры
        model::Game game = json_loader::LoadGame(config_path);

        // 3. Инициализация прикладного слоя (Application)
        // Теперь Application чист от сетевых зависимостей
        app::Application application(std::move(game));

        // 4. Подготовка сетевой инфраструктуры
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // Обработчик сигналов для остановки
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait(
            [&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
                if (!ec) {
                    logger::LogInfo("server was closed"sv);
                    ioc.stop();
                }
            });

        // 5. Создание главного обработчика (Dispatcher)
        // Передаем ссылку на Application и путь к статике
        http_handler::RequestHandler handler{application, std::filesystem::path(static_path)};

        // 6. Запуск HTTP-сервера
        const auto address = net::ip::make_address("0.0.0.0");
        http_server::ServeHttp(ioc, {address, config::kDefaultPort},
                               [&handler](auto&& req, auto&& send) {
                                   handler(std::forward<decltype(req)>(req),
                                           std::forward<decltype(send)>(send));
                               });

        logger::LogInfo("server started"sv,
                        logger::ServerAddrPortLog(address.to_string(), config::kDefaultPort));

        // 7. Запуск многопоточной обработки
        std::vector<std::jthread> workers;
        for (unsigned i = 0; i < num_threads - 1; ++i) {
            workers.emplace_back([&ioc] { ioc.run(); });
        }
        ioc.run();

    } catch (const std::exception& ex) {
        logger::LogError("error"sv, logger::ExceptionLog(EXIT_FAILURE, "Server critical failure"s, ex.what()));
        return EXIT_FAILURE;
    }
}
