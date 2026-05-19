#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "http_handler_types.h"
#include "http_handler_defs.h"

#include <boost/asio/strand.hpp>

#include <iostream>
#include <filesystem>

namespace http_handler {

namespace fs = std::filesystem;
namespace net = boost::asio;

using namespace std::literals;

class FileHandler : public std::enable_shared_from_this<FileHandler> {
public:
    using Strand = net::strand<net::io_context::executor_type>;

    explicit FileHandler(fs::path path) 
        : server_files_path_{path} {
        // Поддерживаемые расширения файлов для отдачи, для которых можем указать ContentType
        supported_files_[".htm"s]  = ContentType::TEXT_HTML;
        supported_files_[".html"s] = ContentType::TEXT_HTML;
        supported_files_[".css"s]  = ContentType::TEXT_CSS;
        supported_files_[".txt"s]  = ContentType::TEXT_PLAIN;
        supported_files_[".js"s]   = ContentType::TEXT_JS;
        supported_files_[".json"s] = ContentType::APP_JSON;
        supported_files_[".xml"s]  = ContentType::APP_XML;
        supported_files_[".png"s]  = ContentType::IMG_PNG;
        supported_files_[".jpg"s]  = ContentType::IMG_JPEG;
        supported_files_[".jpe"s]  = ContentType::IMG_JPEG;
        supported_files_[".jpeg"s] = ContentType::IMG_JPEG;
        supported_files_[".gif"s]  = ContentType::IMG_GIF;
        supported_files_[".bmp"s]  = ContentType::IMG_BMP;
        supported_files_[".ico"s]  = ContentType::IMG_ICON;
        supported_files_[".tiff"s] = ContentType::IMG_TIFF;
        supported_files_[".tif"s]  = ContentType::IMG_TIFF;
        supported_files_[".svg"s]  = ContentType::IMG_SVG;
        supported_files_[".svgz"s] = ContentType::IMG_SVG;
        supported_files_[".mp3"s]  = ContentType::AUDIO_MPEG;
    }

    FileHandler(const FileHandler&) = delete;
    FileHandler& operator=(const FileHandler&) = delete;

    // Обработчик запросов к файловой системе
    FileRequestResult HandleFileRequest(const StringRequest& req) const;

private:
    bool IsSubPath(fs::path path, fs::path base) const;

    std::string GetPathFromUri(boost::core::string_view s) const;
    std::string GetFileExtention(const std::string& file_name) const;
    std::string GetContentType(const std::string& file_name) const;

    StringResponse MakeStringResponse(http::status status, std::string_view body, size_t size, unsigned http_version,
                                      bool keep_alive, std::string_view content_type) const;
    FileResponse MakeFileResponse(http::status status, http::file_body::value_type& body, size_t size, unsigned http_version,
                                  bool keep_alive, std::string_view content_type) const;

    fs::path server_files_path_;
    std::unordered_map<std::string, std::string> supported_files_;
};

}  // namespace http_handler
