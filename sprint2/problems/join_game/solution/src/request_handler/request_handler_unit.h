#pragma once
#include <unordered_map>
#include <vector>
#include <boost/beast/http.hpp>

namespace requestHandler {

    namespace beast = boost::beast;
    namespace http = beast::http;


    template<typename Activator, typename Handler>
    class RHUnit {
    public:
        RHUnit(Activator activator,
                std::unordered_map<http::verb, Handler> handlers,
                Handler fault_handler,
                std::vector<Handler>&& add_Handlers = std::vector<Handler>()) :
            activator_(std::move(activator)),
            handlers_(std::move(handlers)),
            faultHandler_(fault_handler),
            addHandlers_(std::move(add_Handlers)){};

        RHUnit(const RHUnit& other) = default;
        RHUnit(RHUnit&& other) = default;
        RHUnit& operator = (const RHUnit& other) = default;
        RHUnit& operator = (RHUnit&& other) = default;
        virtual ~RHUnit() = default;

        Handler& GetHandler(http::verb method) {            //Геттер на хэндлер, если метода нет, то выдаст хэндл ошибок
            if (handlers_.contains(method)) {
                return handlers_[method];
            }
            else {
                return faultHandler_;
            }
        };

        Handler GetAddHandlerByIndex(size_t idx) {
            if (idx < addHandlers_.size()) {
                return addHandlers_[idx];
            }
            return nullptr;
        };

        Activator& GetActivator() {
            return activator_;
        };

    private:
        Activator activator_;
        std::unordered_map<http::verb, Handler> handlers_;
        Handler faultHandler_;
        std::vector<Handler> addHandlers_;
    };

}