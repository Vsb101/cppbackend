#include "file_handler.h"

#include "boost/beast/http/status.hpp"
#include <boost/beast/http/file_body.hpp>
#include <boost/json.hpp>
#include "boost/json/serialize.hpp"
#include <boost/url.hpp>
#include <iostream>
#include <string_view>

#include "boost/json/value_from.hpp"
#include "json_fields.h"

namespace json = boost::json;
namespace urls = boost::urls;
namespace core = boost::core;
namespace sys = boost::system;

namespace http_handler {

FileRequestResult FileHandler::HandleFileRequest(const StringRequest& req) const {
    const auto text_response = [&req,this](http::status status, std::string_view text, size_t size, std::string_view content_type) {
        return this->MakeStringResponse(status, text, size, req.version(), req.keep_alive(), content_type);
    };

    auto req_method = req.method();
    // Определяем цель запроса
    std::string req_target(req.target());
    // Удаляем завершающий слэш, чтобы не мешал разбору
    while (!req_target.empty() && req_target.back() == '/') {
        req_target.pop_back();
    }

    http::status status;
    std::string content_type;
    std::string response_body;

    // Получаем декодированный запрос (он же - потенциальный путь к файлу)
    // и генерим путь относительно корневой папки сервера
    auto req_path = GetPathFromUri(req_target);
    if (req_path == "/" || req_path.empty()) {
        req_path = "/index.html";
    }

    auto decoded = server_files_path_ / fs::path("." + req_path);
    // Проверяем, что не убежали из корня
    if (IsSubPath(decoded, server_files_path_)) {
        http::file_body::value_type file;
        if (sys::error_code ec; file.open(decoded.c_str(), beast::file_mode::read, ec), ec) {
            std::cerr << "Failed to open file "sv << decoded << std::endl;
            // Make error response
            content_type = ContentType::TEXT_PLAIN;
            status = http::status::not_found;
            response_body = "File Not Found!"s;
        } else {
            content_type = GetContentType(decoded);
            status = http::status::ok;
            return this->MakeFileResponse(status, file, file.size(), req.version(), req.keep_alive(), content_type);
        }
    } else {
        // Make error response
        content_type = ContentType::TEXT_PLAIN;
        status = http::status::not_found;
        response_body = "File Not Found!"s;
    }

    int response_size = 0;
    if (req_method == http::verb::get) {
        response_size = response_body.size();
    } else if (req_method == http::verb::head) {
        response_size = response_body.size();   // Запоминаем размер
        response_body = ""s;    // Зачищаем тело запроса
    } else {    // Недопустимый метод
        status = http::status::method_not_allowed;
        response_body = "Invalid method"s;
        response_size = response_body.size();   // Запоминаем размер
    }

    return text_response(status, response_body, response_size, content_type);
}

// Создаёт StringResponse с заданными параметрами
StringResponse FileHandler::MakeStringResponse(http::status status, std::string_view body, size_t size, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type) const {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    if (status == http::status::method_not_allowed) {
        response.set(http::field::allow, "GET, HEAD");
    }
    response.body() = body;
    response.content_length(size);
    response.keep_alive(keep_alive);
    return response;
}

// Функция декодирования URI. Возвращает путь в виде строки
std::string FileHandler::GetPathFromUri(core::string_view s) const {
    urls::url_view u(s);
    std::string decoded = u.path();
    // Почему-то плюсы автоматом не переводятся в пробелы. Добавлена дополнительная обработка
    std::replace(decoded.begin(), decoded.end(), '+', ' ');
    return decoded;
}

// Создаёт FileResponse с заданными параметрами
FileResponse FileHandler::MakeFileResponse(http::status status, http::file_body::value_type& body, size_t size, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type) const {
    FileResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = std::move(body);
    response.content_length(size);
    response.keep_alive(keep_alive);
    return response;
}

// Возвращает true, если каталог p содержится внутри base_path.
bool FileHandler::IsSubPath(fs::path path, fs::path base) const {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

// Получает расширение файла по его имени
std::string FileHandler::GetFileExtention(const std::string& file_name) const {
    return file_name.substr(file_name.rfind('.'));
}

// Получает тип контента в файле по его имени
std::string FileHandler::GetContentType(const std::string& file_name) const {
    auto it = supported_files_.find(GetFileExtention(file_name));
    if (it != supported_files_.end()) {
        // Известный тип файла. Возвращаем соответствующий тип контента
        return it->second;
    } else {
        // Неизвестный тип файла. Возвращаем "бинарный" тип 
        return std::string(ContentType::APP_OCT_STREAM);
    }
}

}  // namespace http_handler
