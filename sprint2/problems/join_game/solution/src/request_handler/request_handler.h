#pragma once

#include <filesystem>
#include <string>

#include "../app/app.h"
#include "../server/http_server.h"
#include "api_request_handler_proxy.h"
#include "static_file_request_handler_proxy.h"

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
using StringResponse = http::response<http::string_body>;

/**
 * @brief Основной обработчик HTTP-запросов
 * 
 * Делегирует запросы API-обработчику или обработчику статических файлов
 */
class RequestHandler {
 public:
    explicit RequestHandler(app::Application& application,
                            std::filesystem::path static_content_path)
        : application_{application}
        , static_content_path_{std::move(static_content_path)} {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    /**
     * @brief Обработка HTTP-запроса
     * 
     * @tparam Body Тип тела запроса
     * @tparam Allocator Алокатор полей
     * @tparam Send Тип функции отправки ответа
     * @param req HTTP-запрос
     * @param send Функция отправки ответа
     */
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                    Send&& send) {
        try {
            if (requestHandler::ApiRequestHandlerProxy<
                    http::request<Body, http::basic_fields<Allocator>>, Send>::GetInstance()
                    .Execute(req, application_, std::forward<Send>(send))) {
                return;
            }
            if (requestHandler::StaticFileRequestHandlerProxy<
                    http::request<Body, http::basic_fields<Allocator>>, Send>::GetInstance()
                    .Execute(req, static_content_path_, std::forward<Send>(send))) {
                return;
            }
            (void)requestHandler::PageNotFound(req, application_, std::forward<Send>(send));
        } catch (const std::exception& ex) {
            SendErrorResponse(req.version(), "internalError", "Internal server error",
                              std::forward<Send>(send));
        } catch (...) {
            SendErrorResponse(req.version(), "internalError", "Internal server error",
                              std::forward<Send>(send));
        }
    }

 private:
    /**
     * @brief Отправка ответа об ошибке
     */
    template <typename Send>
    static void SendErrorResponse(
        unsigned int version,
        std::string_view code, std::string_view message, Send&& send) {
        StringResponse response(http::status::internal_server_error, version);
        response.set(http::field::content_type, "application/json");
        response.body() = R"({"code": "internalError", "message": "Internal server error"})";
        response.content_length(response.body().size());
        response.set(http::field::keep_alive, "true");
        send(std::move(response));
    }

    app::Application& application_;
    std::filesystem::path static_content_path_;
};

}  // namespace http_handler