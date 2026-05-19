#include "sdk.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>  // add_console_log()
#include <boost/program_options.hpp>
#include <boost/signals2.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "server_params.h"
#include "json_loader.h"
#include "request_handler.h"
#include "game.h"
#include "logger.h"
#include "request_handler_logging.h"
#include "json_fields.h"
#include "app.h"
#include "ticker.h"

#include "state_serialization.h"

using namespace std::literals;
using milliseconds = std::chrono::milliseconds;

namespace net = boost::asio;
namespace sys = boost::system;
namespace sig = boost::signals2;

namespace fs = std::filesystem;
namespace logging = boost::log;


namespace {

struct Args {
    bool is_dt_set = false;
    unsigned long dt;
    std::string config_file;
    std::string static_path;
    bool is_state_path_set = false;
    std::string state_path;
    bool is_save_state_period_set = false;
    unsigned long save_state_period;
    bool is_random_spawn;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"All options"s};

    Args args;
    desc.add_options()
        // Добавляем опцию --help и её короткую версию -h
        ("help,h", "Show help")
        // Опция --tick-period milliseconds, сохраняющая свои аргументы в поле args.dt
        ("tick-period,t", po::value(&args.dt)->value_name("milliseconds"s), "set tick period")
        // Опция --config-file file, сохраняющая свой аргумент в поле args.config_file
        ("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")
        // Опция --www-root path, сохраняющая свой аргумент в поле args.static_path
        ("www-root,w", po::value(&args.static_path)->value_name("dir"s), "set static files root")
        // Опция --state-file <путь-к-файлу>, сохраняющая свой аргумент в поле args.state_path
        ("state-file,w", po::value(&args.state_path)->value_name("file"s), "set game state file path")
        // Опция --save-state-period <игровое-время-в-миллисекундах>, сохраняющая свой аргумент в поле args.save_state_period
        ("save-state-period,w", po::value(&args.save_state_period)->value_name("milliseconds"s), "set game state save period")
        // Опция --randomize-spawn-points, сохраняющая свой аргумент в поле args.is_random_spawn
        ("randomize-spawn-points", po::bool_switch(&args.is_random_spawn),"spawn dogs at random positions");

    // variables_map хранит значения опций после разбора
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        // Если был указан параметр --help, то выводим справку и возвращаем nullopt
        std::cout << desc;
        return std::nullopt;
    }

    // Проверяем наличие опций
    if (vm.contains("tick-period"s)) {
        args.is_dt_set = true;
    }
    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file path have not been specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static files root is not specified"s);
    }

    if (vm.contains("state-file"s)) {
        args.is_state_path_set = true;
    }

    if (vm.contains("save-state-period"s)) {
        args.is_save_state_period_set = true;
    }

    // С опциями программы всё в порядке, возвращаем структуру args
    return args;
}

// Запускает функцию func на n_threads потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n_threads, const Fn& func) {
    // число рабочих потоков (минимум один)
    n_threads = std::max(1u, n_threads);
    std::vector<std::jthread> workers;
    workers.reserve(n_threads - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию func
    while (--n_threads) {
        workers.emplace_back(func);
    }
    func();
}

}  // namespace

int main(int argc, const char* argv[]) {
    try {
        std::optional<Args> args = ParseCommandLine(argc, argv);
        if ( !args.has_value() ) {
            return EXIT_SUCCESS;
        }

        // Загружаем карту из файла и построить модель игры
        auto [game,extra_data] = json_loader::LoadGame(args->config_file);

        // Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // strand для выполнения запросов к API
        auto api_strand = net::make_strand(ioc);

        // Получаем URL для подключения к базе данных
        const char* db_url = std::getenv("GAME_DB_URL");
        if (!db_url) {
            throw std::runtime_error("DB URL is not specified");
        }

        // Объект Application содержит сценарии использования
        app::Application app(game, db_url);

        // Объект StateSerializer содержит механизмы сериализации/десериализации состояния игры
        serialization::StateSerializer serializer(game, app);

        if (args->is_dt_set) {
            // Настраиваем вызов метода Application::ExecuteTick каждые args->dt миллисекунд внутри strand
            auto ticker = std::make_shared<utils::Ticker>(api_strand, std::chrono::milliseconds(args->dt),
                [&app](std::chrono::milliseconds delta) { app.ExecuteTick(delta); }
            );
            ticker->Start();
        }

        sig::scoped_connection conn;
        if (args->is_state_path_set) {
            // Пробуем азгрузить состояние игры из файла
            if (std::filesystem::exists(args->state_path)) {
                try {
                    serializer.Deserialize(args->state_path);
                } catch (const std::exception& ex) {
                    std::cerr << ex.what() << std::endl;
                    return EXIT_FAILURE;
                }
            }

            // Если задано сохранение состояния по времени, то настраиваем обработчик
            if (args->is_save_state_period_set) {
                // Лямбда-функция будет вызываться всякий раз, когда Application будет слать сигнал tick
                // Функция перестанет вызываться после разрушения conn.
                conn = app.DoOnTick([total = 0ms, &serializer, &args](milliseconds delta) mutable {
                    serializer.Serialize(args->state_path);
                    total += delta;
                    std::cout << "Tick! Delta: " << delta.count() << "ms, Total: " << total.count() << "ms" << std::endl;
                });
            }
        }

        // Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        // Подписываемся на сигналы и при их получении завершаем работу сервера
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
            BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, boost::json::value({json_field::ERROR_CODE, EXIT_SUCCESS}))
                                    << server_params::EXIT_MESSAGE;
        });

        // Устанавливаем путь к статическим файлам
        fs::path base_path{std::string(args->static_path)};

        // Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto handler = make_shared<http_handler::RequestHandler>(api_strand, app, base_path, extra_data);

        // endpoint нужен и известен только внутри логгера, поэтому тут он не нужен
        http_handler::LoggingRequestHandler logging_handler{ [handler](auto&& req, auto&& send) {
            (*handler)(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        } };

        const auto address = net::ip::make_address(server_params::ADRESS);
        // Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        http_server::ServeHttp(ioc, {address, server_params::PORT}, logging_handler);

        // Настраиваем логгер
        boost::log::add_common_attributes(); 
        logging::add_console_log( 
            std::cout,
            boost::log::keywords::format = &logger::MyFormatter,
            logging::keywords::auto_flush = true
        );

        boost::json::object server_params_jobject;
        server_params_jobject[json_field::SERVER_PORT] = server_params::PORT;
        server_params_jobject[json_field::SERVER_ADDRESS] = server_params::ADRESS;
        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, boost::json::value(server_params_jobject))
                                << server_params::START_MESSAGE;

        // Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        // В этой точке все асинхронные операции уже завершены и можно 
        // сохранить состояние сервера в файл
        if (args->is_state_path_set) {
            serializer.Serialize(args->state_path);
        }
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error) << boost::log::add_value(additional_data, boost::json::value({json_field::ERROR_CODE, EXIT_FAILURE}))
                                 << ex.what();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
