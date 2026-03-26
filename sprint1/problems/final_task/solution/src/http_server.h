#pragma once
#include "sdk.h"
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>

namespace http_server {

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;

//=============================================================================
// SessionBase - базовый класс сессии
//=============================================================================

/**
 * @class SessionBase
 * @brief Базовый класс для управления HTTP-сессией.
 *
 * Отвечает за:
 * - Асинхронное чтение входящих HTTP-запросов
 * - Асинхронную отправку HTTP-ответов
 * - Управление жизненным циклом соединения
 *
 * Использует Boost.Asio и Boost.Beast для асинхронного I/O.
 * Паттерн: каждый клиент получает свою сессию, сессия живёт пока клиент не закроет соединение.
 */
class SessionBase {
protected:
    /// Тип HTTP-запроса (стрроковое тело)
    using HttpRequest = http::request<http::string_body>;

    ~SessionBase() = default;

    // Запрещаем копирование и присваивание
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;

    /**
     * @brief Конструктор сессии
     * @param socket TCP-сокет, полученный от акцептора
     */
    explicit SessionBase(tcp::socket&& socket)
        : stream_(std::move(socket)) {}

public:
    /// @brief Запускает сессию (начинает обрабатывать запросы)
    void Run();

    /// @brief Асинхронно отправляет HTTP-ответ клиенту
    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields>&& response);

    /// @brief Асинхронно читает следующий HTTP-запрос от клиента
    void Read();

    /**
     * @brief Обрабатывает запрос (абстрактный метод)
     * @param request HTTP-запрос от клиента
     * Реализуется в наследнике (Session)
     */
    virtual void HandleRequest(HttpRequest&& request) = 0;

private:
    /// @brief Вызывается после завершения чтения запроса
    void OnRead(beast::error_code ec, std::size_t bytes_read);

    /// @brief Вызывается после отправки ответа
    void OnWrite(bool close, beast::error_code ec, std::size_t bytes_written);

    /// @brief Закрывает соединение с клиентом
    void Close();

    /// @brief Возвращает shared_ptr на текущий объект (для захвата в callbacks)
    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;

    beast::tcp_stream stream_;          ///< Поток для чтения/записи (TCP)
    beast::flat_buffer buffer_;         ///< Буфер для входящих данных
    HttpRequest request_;               ///< Текущий обрабатываемый запрос

    /// @brief Выводит информацию об ошибке в stderr
    static void ReportError(beast::error_code ec, const char* what) {
        std::cerr << what << ": " << ec.message() << std::endl;
    }
};

//=============================================================================
// Session - конкретная реализация сессии с обработчиком
//=============================================================================

/**
 * @class Session
 * @brief Шаблонный класс сессии с конкретным обработчиком запросов.
 *
 * @tparam RequestHandler Тип обработчика запросов (например, RequestHandler)
 *
 * Пример использования:
 * @code
 * auto session = std::make_shared<Session<RequestHandler>>(std::move(socket), handler);
 * session->Run();
 * @endcode
 */
template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
    RequestHandler request_handler_;  ///< Обработчик HTTP-запросов

public:
    /**
     * @brief Конструктор сессии
     * @param socket TCP-сокет для связи с клиентом
     * @param handler Обработчик запросов
     */
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& handler)
        : SessionBase(std::move(socket))
        , request_handler_(std::forward<Handler>(handler)) {}

private:
    /// @brief Реализация обработки запроса - делегирует запрос обработчику
    void HandleRequest(HttpRequest&& request) override {
        request_handler_(std::move(request), [self = this->shared_from_this()](auto&& response) {
            self->Write(std::move(response));
        });
    }

    /// @brief Возвращает shared_ptr на базовый класс
    std::shared_ptr<SessionBase> GetSharedThis() override {
        return this->shared_from_this();
    }
};

//=============================================================================
// Listener - акцептор входящих соединений
//=============================================================================

/**
 * @class Listener
 * @brief Слушает входящие TCP-соединения и создаёт сессии для каждого клиента.
 *
 * @tparam RequestHandler Тип обработчика запросов
 *
 * Работает в одном потоке, используя асинхронные операции Boost.Asio.
 * Для каждого нового соединения создаётся объект Session.
 */
template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
    net::io_context& ioc_;           ///< IO-контекст для асинхронных операций
    tcp::acceptor acceptor_;         ///< Акцептор для прослушивания порта
    RequestHandler request_handler_; ///< Обработчик запросов (общий для всех сессий)

public:
    /**
     * @brief Конструктор Listener
     * @param ioc IO-контекст
     * @param endpoint Адрес и порт для прослушивания
     * @param handler Обработчик запросов
     */
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& handler)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc))
        , request_handler_(std::forward<Handler>(handler))
    {
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(net::socket_base::max_listen_connections);
    }

    /// @brief Запускает прослушивание входящих соединений
    void Run() {
        DoAccept();
    }

private:
    /// @brief Начинает асинхронное принятие следующего соединения
    void DoAccept() {
        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }

    /// @brief Вызывается при установлении нового соединения
    void OnAccept(beast::error_code ec, tcp::socket socket) {
        if (ec) {
            ReportError(ec, "accept");
        } else {
            // Создаём новую сессию для клиента и запускаем её
            std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();
            // Продолжаем принимать новые соединения
            DoAccept();
        }
    }

    /// @brief Выводит информацию об ошибке в stderr
    static void ReportError(beast::error_code ec, const char* what) {
        std::cerr << what << ": " << ec.message() << std::endl;
    }
};

//=============================================================================
// Функции для запуска сервера
//=============================================================================

/**
 * @brief Запускает HTTP-сервер
 * @param ioc IO-контекст (обычно создаётся в main())
 * @param endpoint Адрес и порт для прослушивания (например, {0.0.0.0}, 8080)
 * @param handler Обработчик HTTP-запросов
 *
 * Пример использования:
 * @code
 * net::io_context ioc;
 * RequestHandler handler{game};
 * ServeHttp(ioc, {net::ip::make_address("0.0.0.0"), 8080}, handler);
 * ioc.run();
 * @endcode
 */
template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    using MyListener = Listener<std::decay_t<RequestHandler>>;
    std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
}

}  // namespace http_server
