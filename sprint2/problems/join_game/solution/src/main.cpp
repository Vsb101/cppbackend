#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "logger.h"
#include "logging_request_handler.h"

using namespace std::literals;
namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static-files-root>"sv << std::endl;
        return EXIT_FAILURE;
    }
    
    // Инициализируем логгер
    InitLogger();
    
    int exit_code = EXIT_SUCCESS;
    
    try {
        //Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);

        // Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](beast::error_code ec, int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        // Создаём обработчик HTTP-запросов и связываем его с моделью игры и статическими файлами
        http_handler::RequestHandler handler{game, argv[2]};

        // Создаём декоратор для логирования
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        net::ip::tcp::endpoint endpoint{address, port};
        
        // Логгируем запуск сервера
        js::object start_data;
        start_data["port"] = port;
        start_data["address"] = "0.0.0.0"s;
        LogJson("server started", start_data);
        
        // Создаём декоратор и запускаем серве
        auto logging_handler = http_handler::LoggingRequestHandler{handler};
        http_server::ServeHttp(ioc, endpoint, [&logging_handler](auto&& req, auto&& send) {
            logging_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        // Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        // Логгируем ошибку с исключением
        js::object error_data;
        error_data["code"] = 1;
        error_data["text"] = ex.what();
        error_data["where"] = "main";
        LogJson("error", error_data);
        exit_code = EXIT_FAILURE;
    }
    
    // Логгируем остановку сервера
    js::object exit_data;
    exit_data["code"] = exit_code;
    LogJson("server exited", exit_data);
    
    return exit_code;
}
