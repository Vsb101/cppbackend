#pragma once

/**
 * @file static_file_request_handler.h
 * @brief Обработчик статических файлов
 */

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <boost/beast/http.hpp>

#include "../logger/logger.h"
#include "../other/file_system.h"

namespace requestHandler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
using StringResponse = http::response<http::string_body>;
using namespace std::literals;

/**
 * @brief Карта расширений файлов к MIME-типам
 */
const std::unordered_map<std::string_view, std::string_view> k_content_type = {
    {".htm", "text/html"},       {".html", "text/html"},
    {".css", "text/css"},        {".txt", "text/plain"},
    {".js", "text/javascript"},  {".json", "application/json"},
    {".xml", "application/xml"}, {".png", "image/png"},
    {".jpg", "image/jpeg"},      {".jpe", "image/jpeg"},
    {".jpeg", "image/jpeg"},     {".gif", "image/gif"},
    {".bmp", "image/bmp"},       {".ico", "image/vnd.microsoft.icon"},
    {".tiff", "image/tiff"},     {".tif", "image/tiff"},
    {".svg", "image/svg+xml"},   {".svgz", "image/svg+xml"},
    {".mp3", "audio/mpeg"}};

/**
 * @brief Имя файла по умолчанию для директорий
 */
constexpr std::string_view k_index_file_name = "index.html";

/**
 * @brief Проверка: файл статического контента не найден
 * 
 * @tparam Body Тип тела запроса
 * @tparam Allocator Алокатор полей
 * @param req HTTP-запрос
 * @param static_content_path Путь к корню статического контента
 * @return true если файл не найден
 * @return false если файл найден
 */
template <typename Body, typename Allocator>
[[nodiscard]] bool IsStaticContentFileNotFound(
    const http::request<Body, http::basic_fields<Allocator>>& req,
    const std::filesystem::path& static_content_path) {
    std::filesystem::path static_content{static_content_path};
    if (req.target().empty() || req.target() == "/") {
        std::filesystem::path index_path{k_index_file_name.data()};
        static_content = std::filesystem::weakly_canonical(static_content / index_path);
    } else {
        std::string_view path_str = req.target().substr(1, req.target().size() - 1);
        std::filesystem::path index_path{path_str};
        static_content = std::filesystem::weakly_canonical(static_content / index_path);
        if (std::filesystem::is_directory(static_content)) {
            std::filesystem::path index_path{k_index_file_name.data()};
            static_content = std::filesystem::weakly_canonical(static_content / index_path);
        }
    }
    return !std::filesystem::exists(static_content);
}

/**
 * @brief Обработчик: файл статического контента не найден
 * 
 * @tparam Request Тип запроса
 * @tparam Send Тип функции отправки
 * @param req HTTP-запрос
 * @param static_content_path Путь к корню статического контента
 * @param send Функция отправки ответа
 */
template <typename Request, typename Send>
void StaticContentFileNotFound(const Request& req,
                               const std::filesystem::path& static_content_path,
                               Send&& send) {
    StringResponse response(http::status::not_found, req.version());
    response.set(http::field::content_type, "text/plain");
    response.body() = "Static content file not found.";
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    send(response);
}

/**
 * @brief Проверка: выход за пределы корня статического контента
 * 
 * @tparam Request Тип запроса
 * @param req HTTP-запрос
 * @param static_content_path Путь к корню статического контента
 * @return true если выход за пределы
 * @return false если внутри корня
 */
template <typename Request>
[[nodiscard]] bool IsLeaveStaticContentRootDir(
    const Request& req, const std::filesystem::path& static_content_path) {
    std::filesystem::path static_content{static_content_path};
    std::string_view path_str = req.target().substr(1, req.target().size() - 1);
    std::filesystem::path tmp_path{path_str};
    static_content = std::filesystem::weakly_canonical(static_content / tmp_path);
    return !util::IsSubPath(static_content, static_content_path);
}

/**
 * @brief Обработчик: выход за пределы корня статического контента
 * 
 * @tparam Request Тип запроса
 * @tparam Send Тип функции отправки
 * @param req HTTP-запрос
 * @param static_content_path Путь к корню статического контента
 * @param send Функция отправки ответа
 */
template <typename Request, typename Send>
void LeaveStaticContentRootDir(const Request& req,
                               const std::filesystem::path& static_content_path,
                               Send&& send) {
    StringResponse response(http::status::bad_request, req.version());
    response.set(http::field::content_type, "text/plain");
    response.body() = "Leave static content root directory.";
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    send(response);
}

/**
 * @brief Проверка: получение статического файла
 * 
 * @tparam Request Тип запроса
 * @param req HTTP-запрос
 * @param static_content_path Путь к корню статического контента
 * @return true всегда (пропускает все запросы)
 */
template <typename Request>
[[nodiscard]] bool IsGetStaticContentFile(
    const Request& req, const std::filesystem::path& static_content_path) {
    return true;
}

/**
 * @brief Обработчик: получение статического файла
 * 
 * @tparam Request Тип запроса
 * @tparam Send Тип функции отправки
 * @param req HTTP-запрос
 * @param static_content_path Путь к корню статического контента
 * @param send Функция отправки ответа
 */
template <typename Request, typename Send>
void GetStaticContentFile(const Request& req,
                          const std::filesystem::path& static_content_path,
                          Send&& send) {
    http::response<http::file_body> tmp_res;
    tmp_res.version(11);
    tmp_res.result(http::status::ok);

    std::filesystem::path static_content{static_content_path};
    if (req.target().empty() || req.target() == "/") {
        std::filesystem::path index_path{k_index_file_name.data()};
        static_content = std::filesystem::weakly_canonical(static_content / index_path);
    } else {
        std::string_view path_str = req.target().substr(1, req.target().size() - 1);
        std::filesystem::path index_path{path_str};
        static_content = std::filesystem::weakly_canonical(static_content / index_path);
    }

    auto it = k_content_type.find(static_content.extension().string());
    if (it != k_content_type.end()) {
        tmp_res.insert(http::field::content_type, it->second);
    } else {
        tmp_res.insert(http::field::content_type, "application/octet-stream");
    }

    http::file_body::value_type file;
    std::string static_content_str = static_content.string();

    if (sys::error_code ec; file.open(static_content_str.c_str(), beast::file_mode::read, ec),
        ec) {
        logger::LogError("error"sv,
                         logger::ExceptionLog(0, "Failed to open static content file "sv,
                                              ec.what()));
    } else {
        tmp_res.body() = std::move(file);
    }
    tmp_res.prepare_payload();
    send(std::move(tmp_res));
}

}  // namespace requestHandler