#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <atomic>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io}
        , gas_cooker_{std::make_shared<GasCooker>(io_, 8)} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        auto bread = store_.GetBread();
        auto sausage = store_.GetSausage();
        auto bread_baked = std::make_shared<bool>(false);
        auto sausage_fried = std::make_shared<bool>(false);
        auto handler_call_made = std::make_shared<bool>(false);
        
        // Создаём отдельные таймеры для булки и сосиски
        auto bread_timer = std::make_shared<net::steady_timer>(io_);
        auto sausage_timer = std::make_shared<net::steady_timer>(io_);
        
        // Захватываем gas_cooker_ по значению (shared_ptr), чтобы он жил до конца операции
        auto gas_cooker = gas_cooker_;
        
        // Используем atomic_fetch_add для потокобезопасного увеличения ID
        int hot_dog_id = next_hot_dog_id_.fetch_add(1);

        auto call_handler_if_ready = [handler, bread, sausage, bread_baked, sausage_fried, handler_call_made, hot_dog_id]() {
            if (*bread_baked && *sausage_fried && !*handler_call_made) {
                *handler_call_made = true;
                try {
                    auto hot_dog = HotDog{hot_dog_id, sausage, bread};
                    handler(Result<HotDog>{std::move(hot_dog)});
                } catch (const std::exception&) {
                    handler(Result<HotDog>{std::current_exception()});
                }
            }
        };

        bread->StartBake(*gas_cooker, [bread_timer, bread, bread_baked, call_handler_if_ready]() {
            bread_timer->expires_after(Milliseconds{1000});
            bread_timer->async_wait([bread_timer, bread, bread_baked, call_handler_if_ready](const boost::system::error_code&) {
                bread->StopBaking();
                *bread_baked = true;
                call_handler_if_ready();
            });
        });

        sausage->StartFry(*gas_cooker, [sausage_timer, sausage, sausage_fried, call_handler_if_ready]() {
            sausage_timer->expires_after(Milliseconds{1500});
            sausage_timer->async_wait([sausage_timer, sausage, sausage_fried, call_handler_if_ready](const boost::system::error_code&) {
                sausage->StopFry();
                *sausage_fried = true;
                call_handler_if_ready();
            });
        });
    }

private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_;
    std::atomic<int> next_hot_dog_id_{0};
};

