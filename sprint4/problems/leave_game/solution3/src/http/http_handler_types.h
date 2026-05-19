#pragma once

#include <boost/json/conversion.hpp>
#include <boost/beast/http.hpp>
#include <chrono>

namespace http_handler {

class JoinParams {
public:
    std::string name;
    std::string map_id;
};

class PlayerActionParams {
public:
    std::string direction;
};

class TickParams {
public:
    std::chrono::milliseconds dt;   // в миллисекундах
};

namespace beast = boost::beast;
namespace http = beast::http;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

// Ответ, тело которого представлено в виде бинарной последовательности
using FileResponse = http::response<http::file_body>;
// @todo
// using FileRequestResult = std::variant<EmptyResponse, StringResponse, FileResponse>;
using FileRequestResult = std::variant<StringResponse, FileResponse>;

struct ResponseError {
    std::string code;
    std::string message;
};

// сериализация экземпляра класса ResponseError в JSON-значение
void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, http_handler::ResponseError const& resp_err);

}  // namespace http_handler
