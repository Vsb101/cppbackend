#include "http_server.h"

#include <boost/asio/dispatch.hpp>

namespace http_server {

void SessionBase::Run() {
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
    request_ = {};
    stream_.expires_after(std::chrono::seconds(30));
    http::async_read(stream_, buffer_, request_,
                     beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

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

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}

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
