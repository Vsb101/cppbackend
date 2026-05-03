#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/program_options.hpp>
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
#include "logger/logging_handler.h"
#include "request_handler.h"
#include "other/sdk.h"
#include <boost/asio/ip/tcp.hpp>
#include "infra/http_server.h" 

using namespace std::string_literals;
using namespace std::string_view_literals;
namespace net = boost::asio;
namespace sys = boost::system;

struct Args {
    std::optional<uint64_t> tick_period;
    std::string config_file;
    std::string www_root;
    bool randomize_spawn_points = false;
};

std::optional<Args> ParseCommandLine(int argc, const char* argv[]) {
    namespace po = boost::program_options;
    po::options_description desc{"Allowed options"};

    Args args;
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value<uint64_t>(), "set tick period")
        ("config-file,c", po::value<std::string>(&args.config_file)->value_name("file"), "set config file path")
        ("www-root,w", po::value<std::string>(&args.www_root)->value_name("dir"), "set static files root")
        ("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points), "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return std::nullopt;
    }

    if (!vm.count("config-file") || !vm.count("www-root")) {
        throw std::runtime_error("Config file and www-root paths are required");
    }

    if (vm.count("tick-period")) {
        args.tick_period = vm["tick-period"].as<uint64_t>();
    }

    return args;
}

void StartAutoTick(std::shared_ptr<net::strand<net::io_context::executor_type>> strand, 
                   std::chrono::milliseconds period, 
                   app::Application& app) {
    auto timer = std::make_shared<net::steady_timer>(*strand, period);
    timer->async_wait(net::bind_executor(*strand, [timer, period, &app, strand](sys::error_code ec) {
        if (!ec) {
            app.Tick(period);
            timer->expires_at(timer->expiry() + period);
            StartAutoTick(strand, period, app);
        }
    }));
}

int main(int argc, const char* argv[]) {
    logger::InitLogger();

    try {
        auto args_opt = ParseCommandLine(argc, argv);
        if (!args_opt) return EXIT_SUCCESS;
        const auto& args = *args_opt;

        // 1. Загружаем конфиг (теперь это ProjectConfig)
        auto config = json_loader::LoadGame(args.config_file);

        // 2. Инициализируем Application
        app::Application application(
            std::move(config.game), 
            std::move(config.extra_data), 
            args.tick_period.has_value(), 
            args.randomize_spawn_points
        );

        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Создаем Strand для защиты игрового состояния
        auto strand = std::make_shared<net::strand<net::io_context::executor_type>>(net::make_strand(ioc));

        if (args.tick_period) {
            StartAutoTick(strand, std::chrono::milliseconds(*args.tick_period), application);
        }

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, int) {
            if (!ec) {
                logger::LogInfo("server was closed"sv);
                ioc.stop();
            }
        });

        // 4. Оборачиваем обработчик в shared_ptr
        auto handler = std::make_shared<http_handler::RequestHandler>(application, std::filesystem::path(args.www_root));

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr uint16_t port = 8080;

        // 5. Запуск сервера с выполнением всех запросов строго через strand
        http_server::ServeHttp(ioc, {address, port},
            [handler, strand](auto&& req, auto&& send) {
                net::dispatch(*strand, [handler, req = std::forward<decltype(req)>(req), send = std::forward<decltype(send)>(send)]() mutable {
                    (*handler)(std::move(req), std::move(send));
                });
            });

        logger::LogInfo("server started"sv, logger::ServerAddrPortLog(address.to_string(), port));

        std::vector<std::thread> workers;
        for (unsigned i = 0; i < num_threads - 1; ++i) {
            workers.emplace_back([&ioc] { ioc.run(); });
        }
        ioc.run();

        for (auto& t : workers) { t.join(); }

        logger::LogInfo("server exited"sv, logger::ExitCodeLog(0));

    } catch (const std::exception& ex) {
        logger::LogError("error"sv, logger::ExceptionLog(EXIT_FAILURE, "Server critical failure"s, ex.what()));
        return EXIT_FAILURE;
    }
}
