#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {

void ErrorMessage(beast::error_code ec, std::string_view w) {
  BOOST_LOG_TRIVIAL(error) << logger::CreateLogMessage(
      "error"sv, logger::ExceptionLog(0, ec.what(), w));
}

void SessionBase::Run() {
  net::dispatch(stream_.get_executor(),
                beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

SessionBase::SessionBase(tcp::socket&& socket) : stream_(std::move(socket)) {}

void SessionBase::Read() {
  using namespace std::literals;

  request_ = {};  // ������� ������ ������
  stream_.expires_after(std::chrono::seconds(20));
  http::async_read(
      stream_, buffer_, request_,
      beast::bind_front_handler(&SessionBase::OnRead,
                                GetSharedThis()));  // �������, � ������� OnRead
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
  using namespace std::literals;
  if (ec == http::error::end_of_stream) {
    return Close();  // ������� �� ���������� �������
  }
  if (ec) {
    return ErrorMessage(ec, "Read Error"sv);
  }
  HandleRequest(std::move(request_));
}

void SessionBase::Close() {
  beast::error_code ec;
  stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}

void SessionBase::OnWrite(bool close, beast::error_code ec,
                          [[maybe_unused]] std::size_t bytes_written) {
  if (ec) {
    return ErrorMessage(ec, "Write Error"sv);
  }

  if (close) {
    return Close();
  }

  Read();  // ������ ��. ������
}

}  // namespace http_server