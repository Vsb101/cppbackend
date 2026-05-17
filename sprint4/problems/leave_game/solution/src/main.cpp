#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/thread_pool.hpp>
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
#include "infra/state_serializer.h"
#include "infra/retirement_repository.h"
#include "logger/logger.h"
#include "request_handler.h"
#include "infra/http_server.h" 

using namespace std::string_literals;
using namespace std::string_view_literals;
namespace net = boost::asio;
namespace sys = boost::system;

struct Args {
    std::optional<uint64_t> tick_period;
    std::optional<uint64_t> save_state_period;
    std::string state_file;
    std::string config_file;
    std::string www_root;
    bool randomize_spawn_points = false;
};

std::optional<Args> ParseCommandLine(int argc, const char* argv[]) {
    namespace po = boost::program_options;
    po::options_description desc{"Allowed options"};
    Args args;
    uint64_t save_state_period_val = 0;
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value<uint64_t>(), "set tick period")
        ("state-file,s", po::value<std::string>(&args.state_file)->value_name("file"), "set state file path for save/load game state")
        ("save-state-period,p", po::value<uint64_t>(&save_state_period_val)->value_name("milliseconds"), "set auto-save period in milliseconds")
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
    if (vm.count("tick-period")) args.tick_period = vm["tick-period"].as<uint64_t>();
    if (vm.count("save-state-period")) args.save_state_period = save_state_period_val;
    return args;
}

void StartAutoTick(std::shared_ptr<net::strand<net::io_context::executor_type>> strand, 
                   std::chrono::milliseconds period, 
                   std::shared_ptr<app::Application> app) {
    auto timer = std::make_shared<net::steady_timer>(*strand, period);
    timer->async_wait(net::bind_executor(*strand, [timer, period, app, strand](sys::error_code ec) {
        if (!ec) {
            app->Tick(period);
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

        auto config = json_loader::LoadGame(args.config_file);
        
        // Создаем выделенный пул потоков для работы со сторонней блокирующей СУБД (PostgreSQL).
        // 4 потоков обычно достаточно для параллельной обработки тяжелых SQL запросов пагинации.
        constexpr unsigned db_threads_count = 4;
        auto db_pool = std::make_shared<net::thread_pool>(db_threads_count);

        // Используем shared_ptr для безопасного времени жизни
        // Создаем репозиторий для рекордов, передавая ему и URL, и выделенный пул потоков
        std::shared_ptr<infra::RetirementRepository> retirement_repo;
        const char* db_url = std::getenv("GAME_DB_URL");
        if (db_url) {
            logger::LogInfo("DB URL found, creating repository"sv, std::string(db_url));
            retirement_repo = std::make_shared<infra::RetirementRepository>(db_url, db_pool);
        } else {
            logger::LogInfo("No DB URL, retirement records disabled"sv);
        }

        auto application = std::make_shared<app::Application>(
            std::move(config.game), 
            std::move(config.extra_data), 
            args.tick_period.has_value(), 
            args.randomize_spawn_points,
            retirement_repo
        );

        // Настраиваем сериализацию состояния
        std::unique_ptr<infra::SerializingListener> serializer_listener;
        if (!args.state_file.empty()) {
            // Пробуем загрузить состояние из файла
            std::chrono::milliseconds save_period{0};
            if (args.save_state_period.has_value()) {
                save_period = std::chrono::milliseconds(*args.save_state_period);
            }
            
            serializer_listener = std::make_unique<infra::SerializingListener>(
                args.state_file, save_period, application.get()
            );
            
            // Пытаемся загрузить состояние
            try {
                if (!serializer_listener->LoadState()) {
                    // Файл не существует - начинаем с чистого листа
                    logger::LogInfo("State file not found, starting fresh"sv);
                }
            } catch (const std::exception& ex) {
                // Ошибка при загрузке состояния
                logger::LogError("error"sv, logger::ExceptionLog(
                    EXIT_FAILURE, 
                    "Failed to load state from file"s, 
                    ex.what()
                ));
                return EXIT_FAILURE;
            }
            
            // Регистрируем слушатель
            application->SetApplicationListener(std::move(serializer_listener));
        }

        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        auto strand = std::make_shared<net::strand<net::io_context::executor_type>>(net::make_strand(ioc));

        if (args.tick_period) {
            StartAutoTick(strand, std::chrono::milliseconds(*args.tick_period), application);
        }

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, int) {
            if (!ec) ioc.stop();
        });

        auto handler = std::make_shared<http_handler::RequestHandler>(*application, std::filesystem::path(args.www_root));

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr uint16_t port = 8080;

        http_server::ServeHttp(ioc, {address, port},
            [handler, strand](auto&& req, auto&& send) {
                // Все запросы СТРОГО через strand
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
        for (auto& t : workers) { if(t.joinable()) t.join(); }

        // Сохраняем состояние перед завершением
        if (application) {
            application->NotifyShutdown();
        }

        // Перед выходом дожидаемся завершения всех активных транзакций в СУБД
        if (db_pool) {
            db_pool->join();
        }

    } catch (const std::exception& ex) {
        logger::LogError("error"sv, logger::ExceptionLog(EXIT_FAILURE, "Server failure"s, ex.what()));
        return EXIT_FAILURE;
    }
}
