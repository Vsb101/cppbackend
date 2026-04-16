#pragma once
#include "../logger/logger.h"
#include "../other/sdk.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace http_server {

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;

using namespace std::literals;

void ErrorMessage(beast::error_code ec, std::string_view what);

class SessionBase {
  // Напишите недостающий код, используя информацию из урока
 public:
  void Run();  // Запуск сессии

  const std::string& GetRemoteIp() {  // Геттер на айпи
    static std::string remote_ip;
    try {
      auto temp = stream_.socket().remote_endpoint().address().to_string();
      remote_ip = temp;
    } catch (...) {
    }
    return remote_ip;
  };

 protected:
  using HttpRequest = http::request<http::string_body>;

  SessionBase(const SessionBase&) = delete;  // Запрет на копирование
  SessionBase& operator=(const SessionBase&) = delete;  // Запрет на присваивание

  ~SessionBase() = default;  // Деструктор

  /*Конструктор c explicit, чтобы не было никаких приведений типа, только явно.
  У наследников будет тоже самое т.к. protected*/
  explicit SessionBase(tcp::socket&& socket);

  /*Из теории*/
  template <typename Body, typename Fields>
  void Write(http::response<Body, Fields>&& response) {
    // Запись выполняется асинхронно, поэтому response перемещаем в область кучи
    auto safe_response =
        std::make_shared<http::response<Body, Fields>>(std::move(response));

    auto self = GetSharedThis();
    http::async_write(
        stream_, *safe_response,
        [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
          self->OnWrite(safe_response->need_eof(), ec, bytes_written);
          BOOST_LOG_TRIVIAL(info) << logger::CreateLogMessage(
              "response sent"sv, logger::ResponseLog<Body, Fields>(
                                     self->GetRemoteIp(),
                                     self->GetDurReceivedRequest(
                                         boost::posix_time::microsec_clock::local_time()),
                                     *safe_response));
        });
  }

  void SetReqRecieveTime(
      const boost::posix_time::ptime& reqTime) {  // Сеттер времени прихода запроса
    reqRecieveTime_ = reqTime;
  }

  long GetDurReceivedRequest(
      const boost::posix_time::ptime& currTime) {  // Геттер на дюрейшн
    boost::posix_time::time_duration dur = currTime - reqRecieveTime_;
    return dur.total_milliseconds();
  }

 private:
  void Read();  // Чтение запроса
  void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);
  void Close();  // Закрыть поток
  void OnWrite(bool close, beast::error_code ec,
               [[maybe_unused]] std::size_t bytes_written);
  virtual void HandleRequest(
      HttpRequest&&
          request) = 0;  // Обрабатывает всё это наследник, по этому вируалим метод
  virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;  // Получение самого себя
  boost::posix_time::ptime
      reqRecieveTime_;  // Время получения запроса (точка отсчёта для dur time)

  beast::tcp_stream
      stream_;  // tcp_stream содержит внутри себя сокет и добавляет поддержку таймаутов
  beast::flat_buffer buffer_;  // буффер для сообщений
  HttpRequest request_;        // Сопсно сам запрос
};

template <typename RequestHandler>
class Session : public SessionBase,
                public std::enable_shared_from_this<Session<RequestHandler>> {
  // Напишите недостающий код, используя информацию из урока
 public:
  template <typename Handler>
  Session(tcp::socket&& socket,
          Handler&& request_handler)  // Конструктор, создаёт сессию с переданым сокетом,
                                      // и обработчик
      : SessionBase(std::move(socket)),
        request_handler_(std::forward<Handler>(request_handler)) {}

 private:
  /*Из теории*/
  void HandleRequest(HttpRequest&& request) override {
    SetReqRecieveTime(boost::posix_time::microsec_clock::local_time());
    BOOST_LOG_TRIVIAL(info) << logger::CreateLogMessage(
        "request received"sv, logger::RequestLog(GetRemoteIp(), request));

    // Захватываем умный указатель на текущий объект Session в лямбде,
    // чтобы продлить время жизни сессии до вызова лямбды.
    // Используется generic-лямбда функция, способная принять response произвольного типа
    request_handler_(std::move(request),
                     [self = this->shared_from_this()](auto&& response) {
                       self->Write(std::move(response));
                     });
  }

  std::shared_ptr<SessionBase> GetSharedThis() override {  // Указатель на самого себя
    return this->shared_from_this();
  }

  RequestHandler request_handler_;
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
  // Напишите недостающий код, используя информацию из урока
 public:
  template <typename Handler>
  Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
      : ioc_(ioc),
        acceptor_(net::make_strand(ioc))  // Акцептор, со своим стренд
        ,
        request_handler_(std::forward<Handler>(request_handler)) {
    acceptor_.open(endpoint.protocol());  // Открываем акцептор
    acceptor_.set_option(net::socket_base::reuse_address(
        true));  // Почему-то без этого не открывает сокет, на повторных коннектах
                 // https://www.boost.org/doc/libs/master/doc/html/boost_asio/reference/socket_base/reuse_address.html
    acceptor_.bind(endpoint);  // Отдаём акцептору эндпоинт
    acceptor_.listen(
        net::socket_base::max_listen_connections);  // Переводим в состояние слушания
  }

  void Run() { DoAccept(); }

 private:
  void DoAccept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),  // Передаем ioc
        beast::bind_front_handler(
            &Listener::OnAccept,
            this->shared_from_this()));  // Тут надо передать указатель, иначе будет
                                         // думать что shared_from_this тупо функция
  }

  void OnAccept(sys::error_code ec, tcp::socket socket) {  // Создание сокета
    using namespace std::literals;

    if (ec) {
      return ErrorMessage(ec, "Accept Error"sv);
    }
    AsyncRunSession(std::move(socket));  // Асинзронная сессия
    DoAccept();
  }

  void AsyncRunSession(tcp::socket&& socket) {
    std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)
        ->Run();  // Создаем shraedptr и вызываем ран
  }

  net::io_context& ioc_;
  tcp::acceptor acceptor_;
  RequestHandler request_handler_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint,
               RequestHandler&& handler) {
  // Напишите недостающий код, используя информацию из урока
  std::make_shared<Listener<std::decay_t<RequestHandler>>>(
      ioc, endpoint, std::forward<RequestHandler>(handler))
      ->Run();
}

}  // namespace http_server