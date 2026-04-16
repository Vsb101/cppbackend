#pragma once
#include <filesystem>

#include "../app/app.h"
#include "../request_handler/api_request_handler_proxy.h"
#include "../request_handler/static_file_request_handler_proxy.h"
#include "../server/http_server.h"

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
using StringResponse = http::response<http::string_body>;

class RequestHandler {
 public:
  explicit RequestHandler(app::Application& application,
                          std::filesystem::path staticContentPath)
      : application_{application}, staticContentPath_{staticContentPath} {}

  RequestHandler(const RequestHandler&) = delete;
  RequestHandler& operator=(const RequestHandler&) = delete;

  /*Новый обработчик, вроде короче...но вызывает сомнения в производительности. рошлый
   * стал большим и нечитаемым*/
  template <typename Body, typename Allocator, typename Send>
  void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                  Send&& send) {
    try {
      // Обработать запрос request и отправить ответ, используя send
      if (requestHandler::ApiRequestHandlerProxy<
              http::request<Body, http::basic_fields<Allocator>>, Send>::GetInstance()
              .Execute(req, application_, std::move(send))) {
        return;
      } else if (requestHandler::StaticFileRequestHandlerProxy<
                     http::request<Body, http::basic_fields<Allocator>>,
                     Send>::GetInstance()
                     .Execute(req, staticContentPath_, std::move(send))) {
        return;
      };
      requestHandler::PageNotFound(req, application_, send);
    } catch (const std::exception& ex) {
      StringResponse response(http::status::internal_server_error, req.version());
      response.set(http::field::content_type, "application/json");
      response.body() = R"({"code": "internalError", "message": "Internal server error"})";
      response.content_length(response.body().size());
      response.keep_alive(req.keep_alive());
      send(response);
    } catch (...) {
      StringResponse response(http::status::internal_server_error, req.version());
      response.set(http::field::content_type, "application/json");
      response.body() = R"({"code": "internalError", "message": "Internal server error"})";
      response.content_length(response.body().size());
      response.keep_alive(req.keep_alive());
      send(response);
    }
  }

 private:
  app::Application& application_;  // Тут теперь апп, объект игры уже внутри апп, ио там
                                   // же
  std::filesystem::path staticContentPath_;  // Путь где зранятся файлы, static content
};

}  // namespace http_handler