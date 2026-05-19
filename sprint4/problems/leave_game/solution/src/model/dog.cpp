#include "dog.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace model {

// Пытается добавить предмет в сумку
// Возвращает true, если предмет успешно добавлен
bool Dog::AddToBag(uint32_t item_id, size_t item_type) {
    return bag_.size() >= bag_capacity_
        ? false
        : (bag_.push_back({item_id, item_type}), true);
}

// Очищает сумку и возвращает все предметы
std::vector<Dog::BagItem> Dog::ClearBag() {
    std::vector<BagItem> result = std::move(bag_);
    bag_.clear();
    return result;
}

// Обновляет таймеры игры и бездействия
void Dog::UpdateTime(double delta_time) noexcept {
    play_time_ += delta_time;
    idle_time_ = (speed_.vx == 0.0 && speed_.vy == 0.0)
        ? idle_time_ + delta_time
        : 0.0;
}

// Сбрасывает таймер бездействия
void Dog::ResetIdleTime() noexcept {
    idle_time_ = 0.0;
}

// Метод для Boost.Serialization
template<class Archive>
void Dog::serialize(Archive& ar, const unsigned int version) {
    ar & id_;
    ar & name_;
    ar & direction_;
    ar & position_;
    ar & speed_;
    ar & bag_;
    ar & bag_capacity_;
    ar & score_;
    ar & play_time_;
    ar & idle_time_;
}

// Явная инстанциация шаблона для text_oarchive и text_iarchive
// Необходимо для корректной работы Boost.Serialization
// Спецификация требует явного указания типов архивов
template void Dog::serialize<boost::archive::text_oarchive>(boost::archive::text_oarchive& ar, const unsigned int version);
template void Dog::serialize<boost::archive::text_iarchive>(boost::archive::text_iarchive& ar, const unsigned int version);

} // namespace model