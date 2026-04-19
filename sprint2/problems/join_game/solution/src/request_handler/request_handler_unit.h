#pragma once

/**
 * @file request_handler_unit.h
 * @brief Единица обработки HTTP-запросов
 */

#include <functional>
#include <unordered_map>
#include <vector>
#include <boost/beast/http.hpp>

namespace requestHandler {

namespace beast = boost::beast;
namespace http = beast::http;

/**
 * @brief Единица обработки запросов с активатором и набором хендлеров
 * 
 * @tparam Activator Тип активатора (функция проверки условия)
 * @tparam Handler Тип хендлера (функция обработки)
 */
template <typename Activator, typename Handler>
class RhUnit {
 public:
    RhUnit(Activator activator,
           std::unordered_map<http::verb, Handler> handlers,
           Handler fault_handler,
           std::vector<Handler>&& add_handlers = std::vector<Handler>())
        : activator_(std::move(activator))
        , handlers_(std::move(handlers))
        , fault_handler_(std::move(fault_handler))
        , add_handlers_(std::move(add_handlers)) {}

    RhUnit(const RhUnit&) = default;
    RhUnit(RhUnit&&) = default;
    RhUnit& operator=(const RhUnit&) = default;
    RhUnit& operator=(RhUnit&&) = default;
    virtual ~RhUnit() = default;

    /**
     * @brief Получение хендлера для метода
     * 
     * @param method HTTP-метод
     * @return Handler& Хендлер для метода или хендлер ошибок
     */
    [[nodiscard]] Handler& GetHandler(http::verb method) {
        if (handlers_.contains(method)) {
            return handlers_.at(method);
        }
        return fault_handler_;
    }

    /**
     * @brief Получение дополнительного хендлера по индексу
     * 
     * @param idx Индекс хендлера
     * @return Handler Хендлер или nullptr
     */
    [[nodiscard]] Handler GetAddHandlerByIndex(size_t idx) {
        if (idx < add_handlers_.size()) {
            return add_handlers_.at(idx);
        }
        return nullptr;
    }

    /**
     * @brief Получение активатора
     * 
     * @return Activator& Активатор
     */
    [[nodiscard]] Activator& GetActivator() {
        return activator_;
    }

 private:
    Activator activator_;
    std::unordered_map<http::verb, Handler> handlers_;
    Handler fault_handler_;
    std::vector<Handler> add_handlers_;
};

}  // namespace requestHandler