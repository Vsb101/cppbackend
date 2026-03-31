#include "http_server.h"

#include <boost/asio/dispatch.hpp>

namespace http_server {

/**
 * @brief Запускает обработку сессии.
 * Использует Boost.Asio для асинхронного выполнения функции Read.
 * Вызывается при старте сессии, чтобы начать чтение запросов от клиента.
 * 
 * Пример вызова:
 * @code
 * void StartSession(tcp::socket&& socket) {
 *     auto session = std::make_shared<MySession>(std::move(socket));
 *     session->Run(); // Запускает цикл обработки запросов
 * }
 * @endcode
 */
void SessionBase::Run() {
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

/**
 * @brief Асинхронно начинает чтение HTTP-запроса от клиента.
 * Сбрасывает текущий запрос, устанавливает таймаут на 30 секунд и инициирует асинхронное чтение.
 * После завершения чтения вызывается OnRead.
 */
void SessionBase::Read() {
    request_ = {};
    stream_.expires_after(std::chrono::seconds(30));
    http::async_read(stream_, buffer_, request_,
                     beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

/**
 * @brief Вызывается после завершения чтения HTTP-запроса.
 * @param ec Код ошибки, если операция чтения завершилась с ошибкой.
 * @param bytes_read Количество прочитанных байт.
 * Обрабатывает результат чтения: 
 * - Если достигнут конец потока (end_of_stream), закрывает соединение.
 * - При других ошибках выводит сообщение об ошибке.
 * - В случае успеха передаёт запрос на обработку методом HandleRequest.
 * 
 * Пример ошибки:
 * @code
 * if (ec) {
 *     std::cerr << "Read error: " << ec.message() << std::endl;
 * }
 * @endcode
 */
void SessionBase::OnRead(beast::error_code ec, std::size_t bytes_read) {
    if (ec == http::error::end_of_stream) {
        return Close();
    }
    if (ec) {
        SessionBase::ReportError(ec, "read");
        return;
    }
    HandleRequest(std::move(request_));
}

/**
 * @brief Закрывает TCP-соединение с клиентом.
 * Выполняет shutdown сокета для отправки сигнала завершения соединения (FIN).
 * При ошибке выводит сообщение об ошибке, но сессия завершается в любом случае.
 */
void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    if (ec) {
        ReportError(ec, "shutdown");
    }
}

/**
 * @brief Вызывается после завершения отправки HTTP-ответа клиенту.
 * @param close Флаг, указывающий, нужно ли закрыть соединение после ответа.
 * @param ec Код ошибки при отправке.
 * @param bytes_written Количество отправленных байт.
 * При ошибке выводит её и завершает сессию.
 * Если соединение не должно закрываться (например, для keep-alive), 
 * начинает чтение следующего запроса с помощью Read().
 */
void SessionBase::OnWrite(bool close, beast::error_code ec, std::size_t bytes_written) {
    if (ec) {
        SessionBase::ReportError(ec, "write");
        return;
    }
    if (close) {
        return Close();
    }
    Read();
}

/**
 * @brief Асинхронно отправляет HTTP-ответ клиенту.
 * @param response Ответ, который нужно отправить (передаётся по значению и перемещается).
 * Создаёт shared_ptr на ответ для продления его жизни до завершения отправки.
 * Использует callback для обработки завершения отправки через OnWrite.
 * Примечание по shared_ptr. Потому что async_write возвращает управление сразу,
 * а отправка происходит асинхронно.
 * Указатель гарантирует, что объект ответа будет жить до завершения операции.
 */
template <typename Body, typename Fields>
void SessionBase::Write(http::response<Body, Fields>&& response) {
    auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));
    auto self = GetSharedThis();
    http::async_write(stream_, *safe_response,
                      [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                          self->OnWrite(safe_response->need_eof(), ec, bytes_written);
                      });
}

// Явная инстанциация шаблона (Explicit Template Instantiation)
template void SessionBase::Write(http::response<http::string_body>&&);

}  // namespace http_server
