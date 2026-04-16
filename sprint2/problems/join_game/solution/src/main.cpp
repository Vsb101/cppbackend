#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <filesystem>
#include <iostream>
#include <thread>

#include "app/app.h"
#include "json/json_loader.h"
#include "logger/logger.h"
#include "other/sdk.h"
#include "request_handler/request_handler.h"

#define BOOST_USE_WINAPI_VERSION 0x0501
// #define DEBUG

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

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
  logger::InitLogger();
  // if (argc != 3) {
  //     BOOST_LOG_TRIVIAL(error) << logger::CreateLogMessage("Usage: game_server
  //     <game-config-json>"sv,
  //         logger::ExitCodeLog(EXIT_FAILURE));
  //     return EXIT_FAILURE;
  // }
  try {
    // 1. Загружаем карту из файла и построить модель игры
#ifndef DEBUG
    model::Game game = json_loader::LoadGame(argv[1]);
#else
    model::Game game = json_loader::LoadGame("data/config.json");  // для дебага
#endif
    // 2. Устанавливаем путь до статического контента.
#ifndef DEBUG
    std::filesystem::path staticContentPath{argv[2]};
#else
    std::filesystem::path staticContentPath{"static"};  // для дебага
#endif
    // 3. Инициализируем io_context
    const unsigned num_threads = std::thread::hardware_concurrency();
    net::io_context ioc(num_threads);
    app::Application application(std::move(game), ioc);

    // 4. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
          if (!ec) {
            BOOST_LOG_TRIVIAL(info) << logger::CreateLogMessage(
                "server was forcibly closed."sv, logger::ExitCodeLog(0));
            ioc.stop();
          }
        });

    // 5. Создаём обработчик HTTP-запросов и связываем его с моделью игры, задаем путь до
    // статического контента.
    http_handler::RequestHandler handler{application, staticContentPath};

    // 6. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;
    http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
      handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
    });

    // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
    BOOST_LOG_TRIVIAL(info) << logger::CreateLogMessage(
        "server started"sv, logger::ServerAddrPortLog(address.to_string(), port));

    // 7. Запускаем обработку асинхронных операций
    RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });
  } catch (const std::exception& ex) {
    BOOST_LOG_TRIVIAL(error) << logger::CreateLogMessage(
        "error"sv, logger::ExceptionLog(EXIT_FAILURE, "Server not started"sv, ex.what()));
    return EXIT_FAILURE;
  }
}