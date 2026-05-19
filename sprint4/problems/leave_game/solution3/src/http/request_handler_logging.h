#pragma once

#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL

#include "json_fields.h"
#include "request_handler.h"
#include <boost/json.hpp>
#include <boost/json/value.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/timer/timer.hpp>
#include <memory>

#include "server_params.h"
#include "logger.h"

namespace http_handler {

template<class SomeRequestHandler>
class LoggingRequestHandler {
public:
    LoggingRequestHandler(SomeRequestHandler hend) : decorated_(hend) {}

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, boost::asio::ip::tcp::endpoint&& endpoint) {
        // Формируем объект с информацией о запросе и печатаем
        boost::json::object request_jobject;
        request_jobject[json_field::REQUEST_IP] = endpoint.address().to_string();
        request_jobject[json_field::REQUEST_URI] = std::string(req.target());
        request_jobject[json_field::REQUEST_METHOD] = req.method_string();

        BOOST_LOG_TRIVIAL(info)  << boost::log::add_value(additional_data, boost::json::value(request_jobject)) 
                                 << server_params::REQUEST_RECEIV_MESSAGE;

        // Заводим объект под информацию об ответе
        auto response_jobject = std::make_shared<boost::json::object>();

        // Заводим таймер, чтобы засечь время формирования ответа
        auto timer = std::make_shared<boost::timer::cpu_timer>();

        auto loggingResponse = [send, response_jobject, timer](auto&& response) {
            timer->stop();
            boost::timer::cpu_times times = timer->elapsed();

            // Заполняем информацию об ответе. Обработка запроса к этому времени уже произведена
            (*response_jobject)[json_field::RESPONSE_TIME] = static_cast<unsigned long>(times.wall / 1'000'000);   // наносекунды в миллисекунды
            (*response_jobject)[json_field::RESPONSE_CODE] = response.result_int();
            (*response_jobject)[json_field::RESPONSE_CONTENT_TYPE] = response[http::field::content_type];

            // непосредственная отправка ответа
            send(response);

            BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, boost::json::value(*response_jobject))
                                    << server_params::RESPONSE_SENT_MESSAGE;

        };

        // Непосредственно обработка запроса с использованием лямбды
        decorated_(std::forward<decltype(req)>(req), std::move(loggingResponse));
    }

private:
     SomeRequestHandler decorated_;
};

}  // namespace http_handler
