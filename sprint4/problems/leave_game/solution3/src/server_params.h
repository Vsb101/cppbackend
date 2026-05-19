#pragma once

#include <boost/asio/ip/basic_endpoint.hpp>

#include <string_view>

namespace server_params {
    using namespace std::literals;
    constexpr boost::asio::ip::port_type PORT         = 8080;
    constexpr static std::string_view ADRESS          = "0.0.0.0"sv;
    constexpr static std::string_view START_MESSAGE   = "server started"sv;
    constexpr static std::string_view EXIT_MESSAGE    = "server exited"sv;
    constexpr static std::string_view REQUEST_RECEIV_MESSAGE = "request received"sv;
    constexpr static std::string_view RESPONSE_SENT_MESSAGE  = "response sent"sv;
}