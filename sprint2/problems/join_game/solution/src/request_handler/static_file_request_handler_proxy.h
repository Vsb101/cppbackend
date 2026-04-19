#pragma once

/**
 * @file static_file_request_handler_proxy.h
 * @brief Прокси для обработки запросов статических файлов
 */

#include <functional>
#include <vector>

#include "../request_handler/request_handler_unit.h"
#include "../request_handler/static_file_request_handler.h"

namespace requestHandler {

/**
 * @brief Прокси для обработки запросов статических файлов
 * 
 * @tparam Request Тип HTTP-запроса
 * @tparam Send Тип функции отправки ответа
 */
template <typename Request, typename Send>
class StaticFileRequestHandlerProxy {
    using activator_type = bool (*)(const Request&, const std::filesystem::path&);
    using handler_type = void (*)(const Request&, const std::filesystem::path&, Send&&);

 public:
    StaticFileRequestHandlerProxy(const StaticFileRequestHandlerProxy&) = delete;
    StaticFileRequestHandlerProxy& operator=(const StaticFileRequestHandlerProxy&) = delete;
    StaticFileRequestHandlerProxy(StaticFileRequestHandlerProxy&&) = delete;
    StaticFileRequestHandlerProxy& operator=(StaticFileRequestHandlerProxy&&) = delete;

    /**
     * @brief Получение единственного экземпляра (Singleton)
     */
    static StaticFileRequestHandlerProxy& GetInstance() {
        static StaticFileRequestHandlerProxy obj;
        return obj;
    }

    /**
     * @brief Выполнение обработки запроса
     * 
     * @param req HTTP-запрос
     * @param static_content_root Корень статического контента
     * @param send Функция отправки ответа
     * @return true если запрос обработан
     * @return false если запрос не обработан
     */
    bool Execute(const Request& req, const std::filesystem::path& static_content_root,
                 Send&& send) {
        for (auto& item : requests_) {
            if (item.GetActivator()(req, static_content_root)) {
                item.GetHandler(req.method())(req, static_content_root,
                                              std::forward<Send>(send));
                return true;
            }
        }
        return false;
    }

 private:
    std::vector<RhUnit<activator_type, handler_type>> requests_ = {
        RhUnit<activator_type, handler_type>(IsStaticContentFileNotFound,
                                             {{http::verb::get, StaticContentFileNotFound}},
                                             StaticContentFileNotFound),
        RhUnit<activator_type, handler_type>(IsLeaveStaticContentRootDir,
                                             {{http::verb::get, LeaveStaticContentRootDir}},
                                             LeaveStaticContentRootDir),
        RhUnit<activator_type, handler_type>(IsGetStaticContentFile,
                                             {{http::verb::get, GetStaticContentFile}},
                                             GetStaticContentFile)};

    StaticFileRequestHandlerProxy() = default;
};

}  // namespace requestHandler