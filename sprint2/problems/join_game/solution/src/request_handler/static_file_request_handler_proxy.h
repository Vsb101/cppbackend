#pragma once
#include <functional>
#include <vector>

#include "../request_handler/request_handler_unit.h"
#include "../request_handler/static_file_request_handler.h"

namespace requestHandler {

template <typename Request, typename Send>
class StaticFileRequestHandlerProxy {
  using ActivatorType = bool (*)(const Request&, const std::filesystem::path&);
  using HandlerType = void (*)(const Request&, const std::filesystem::path&, Send&&);

 public:
  /*Всё копирование запрещено*/
  StaticFileRequestHandlerProxy(const StaticFileRequestHandlerProxy&) = delete;
  StaticFileRequestHandlerProxy& operator=(const StaticFileRequestHandlerProxy&) = delete;
  StaticFileRequestHandlerProxy(StaticFileRequestHandlerProxy&&) = delete;
  StaticFileRequestHandlerProxy& operator=(StaticFileRequestHandlerProxy&&) = delete;

  static StaticFileRequestHandlerProxy& GetInstance() {  // Ссыль на объект
    static StaticFileRequestHandlerProxy obj;
    return obj;
  };

  bool Execute(const Request& req, const std::filesystem::path& static_content_root,
               Send&& send) {  // Сам исполнитель
    for (auto item : requests_) {
      if (item.GetActivator()(req, static_content_root)) {
        item.GetHandler(req.method())(req, static_content_root, std::move(send));
        return true;
      }
    }
    return false;
  };

 private:
  std::vector<RHUnit<ActivatorType, HandlerType> > requests_ = {
      RHUnit<ActivatorType, HandlerType>(StaticContentFileNotFoundCheck,
                                         {{http::verb::get, StaticContentFileNotFound}},
                                         StaticContentFileNotFound),
      RHUnit<ActivatorType, HandlerType>(LeaveStaticContentRootDirCheck,
                                         {{http::verb::get, LeaveStaticContentRootDir}},
                                         LeaveStaticContentRootDir),
      RHUnit<ActivatorType, HandlerType>(GetStaticContentFileCheck,
                                         {{http::verb::get, GetStaticContentFile}},
                                         GetStaticContentFile)};

  StaticFileRequestHandlerProxy() = default;
};

}  // namespace requestHandler