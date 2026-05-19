#pragma once
#include <chrono>

namespace infra {

// Интерфейс для слушателей событий игрового тика
// Используется паттерн Наблюдатель для отвязки доменного слоя от инфраструктурного
class StateListener {
public:
    virtual ~StateListener() = default;
    
    // Вызывается при каждом тике игровых часов
    // delta - время, прошедшее с предыдущего тика
    virtual void OnTick(std::chrono::milliseconds delta) = 0;
    
    // Вызывается перед завершением работы сервера
    virtual void OnShutdown() = 0;
};

} // namespace infra
