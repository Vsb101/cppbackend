#pragma once

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <string>
#include <string_view>

namespace logger {

namespace http = boost::beast::http;
namespace json = boost::json;

/**
 * @brief Структура для логирования HTTP-запроса.
 * 
 * Содержит основную информацию о входящем HTTP-запросе.
 */
struct RequestLog {
    RequestLog(std::string ip_addr, const http::request<http::string_body>& req)
        : ip(std::move(ip_addr))
        , url(std::string(req.target()))
        , method(std::string(req.method_string())) {}

    std::string ip;     ///< IP-адрес клиента
    std::string url;    ///< URI запроса
    std::string method; ///< HTTP-метод (GET, POST и т.д.)
};

/**
 * @brief Структура для логирования HTTP-ответа.
 * 
 * @tparam Body Тип тела ответа
 * @tparam Fields Тип заголовков ответа
 * 
 * Содержит информацию об отправленном HTTP-ответе.
 */
template <typename Body, typename Fields>
struct ResponseLog {
    ResponseLog(std::string ip_addr,
                long response_time,
                const http::response<Body, Fields>& res)
        : ip(std::move(ip_addr))
        , response_time(response_time)
        , code(res.result_int())
        , content_type(res.count(http::field::content_type) 
             ? std::string(res[http::field::content_type]) 
             : "null") {}

    std::string ip;           ///< IP-адрес клиента
    long response_time;        ///< Время обработки запроса в микросекундах
    int code;                  ///< HTTP-статус код ответа
    std::string content_type;  ///< MIME-тип содержимого ответа
};

/**
 * @brief Структура для логирования адреса и порта сервера.
 * 
 * Используется при старте сервера для записи его сетевых параметров.
 */
struct ServerAddrPortLog {
    std::string address; ///< Сетевой адрес сервера
    uint16_t port;       ///< Порт сервера
};

/**
 * @brief Структура для логирования исключений.
 * 
 * Фиксирует информацию об исключении для диагностики ошибок.
 */
struct ExceptionLog {
    ExceptionLog(int code, std::string text, std::string where)
        : code(code), text(std::move(text)), where(std::move(where)) {}

    int code;             ///< Код ошибки
    std::string text;      ///< Текстовое описание ошибки
    std::string where;     ///< Место возникновения ошибки
};

/**
 * @brief Структура для логирования кода выхода программы.
 * 
 * Используется при завершении работы для записи финального статуса.
 */
struct ExitCodeLog {
    int code; ///< Код возврата программы
};

/**
 * @brief Сериализация RequestLog в JSON.
 * 
 * @param jv Целевое JSON-значение
 * @param log Структура для сериализации
 */
void tag_invoke(json::value_from_tag, json::value& jv, const RequestLog& log);

/**
 * @brief Сериализация ServerAddrPortLog в JSON.
 * 
 * @param jv Целевое JSON-значение
 * @param log Структура для сериализации
 */
void tag_invoke(json::value_from_tag, json::value& jv, const ServerAddrPortLog& log);

/**
 * @brief Сериализация ExceptionLog в JSON.
 * 
 * @param jv Целевое JSON-значение
 * @param log Структура для сериализации
 */
void tag_invoke(json::value_from_tag, json::value& jv, const ExceptionLog& log);

/**
 * @brief Сериализация ExitCodeLog в JSON.
 * 
 * @param jv Целевое JSON-значение
 * @param log Структура для сериализации
 */
void tag_invoke(json::value_from_tag, json::value& jv, const ExitCodeLog& log);

/**
 * @brief Сериализация ResponseLog в JSON.
 * 
 * @tparam Body Тип тела ответа
 * @tparam Fields Тип заголовков ответа
 * @param jv Целевое JSON-значение
 * @param log Структура для сериализации
 */
template <typename Body, typename Fields>
void tag_invoke(json::value_from_tag, json::value& jv, const ResponseLog<Body, Fields>& log) {
    jv = {
        {"ip", log.ip},
        {"response_time", log.response_time},
        {"code", log.code},
        {"content_type", log.content_type}
    };
}

}  // namespace logger