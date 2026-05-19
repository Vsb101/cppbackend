#pragma once

#include "../other/tagged.h"
#include <string>
#include <vector>
#include <cstdint>

namespace model {

enum class Direction { NORTH, SOUTH, WEST, EAST };

struct Position { 
    double x = 0.0; 
    double y = 0.0;
    
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar& x;
        ar& y;
    }
};

struct Speed { 
    double vx = 0.0; 
    double vy = 0.0;
    
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar& vx;
        ar& vy;
    }
};

class Dog {
public:
    using Id = util::Tagged<size_t, Dog>;
    
    // Конструктор по умолчанию для Boost.Serialization
    Dog() = default;

    struct BagItem {
        uint32_t id{0};
        size_t type{0};

        BagItem() = default;
        BagItem(uint32_t item_id, size_t item_type) : id(item_id), type(item_type) {}
        
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar& id;
            ar& type;
        }
    };

    Dog(Id id, std::string name, size_t bag_capacity = 3)
        : id_(std::move(id))
        , name_(std::move(name))
        , bag_capacity_(bag_capacity) {
    }

    const Id& GetId() const { return id_; }
    [[nodiscard]] const std::string& GetName() const { return name_; }

    [[nodiscard]] Direction GetDirection() const { return direction_; }
    void SetDirection(Direction direction) { direction_ = direction; }

    [[nodiscard]] const Position& GetPosition() const { return position_; }
    void SetPosition(Position position) { position_ = position; }

    [[nodiscard]] const Speed& GetSpeed() const { return speed_; }
    void SetSpeed(Speed speed) { speed_ = speed; }

    [[nodiscard]] const std::vector<BagItem>& GetBag() const { return bag_; }
    [[nodiscard]] size_t GetBagCapacity() const { return bag_capacity_; }
    [[nodiscard]] bool IsBagFull() const { return bag_.size() >= bag_capacity_; }

    [[nodiscard]] int GetScore() const { return score_; }
    void AddScore(int points) { score_ += points; }

    bool AddToBag(uint32_t item_id, size_t item_type) {
        return bag_.size() >= bag_capacity_
            ? false
            : (bag_.push_back({item_id, item_type}), true);
    }

    std::vector<BagItem> ClearBag() {
        std::vector<BagItem> result = std::move(bag_);
        bag_.clear();
        return result;
    }

    // -Методы для работы с игровым временем и бездействием
    [[nodiscard]] double GetPlayTime() const noexcept { return play_time_; }
    [[nodiscard]] double GetIdleTime() const noexcept { return idle_time_; }

    void UpdateTime(double delta_time) noexcept {
        play_time_ += delta_time;
        idle_time_ = (speed_.vx == 0.0 && speed_.vy == 0.0) ? idle_time_ + delta_time : 0.0;
    }

    // Метод явного сброса времени бездействия (вызывается при получении команды движения)
    void ResetIdleTime()noexcept { idle_time_ = 0.0; }

    // Методы для сериализации
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar&* id_;
        ar& name_;
        ar& direction_;
        ar& position_;
        ar& speed_;
        ar& bag_;
        ar& bag_capacity_;
        ar& score_;
        ar& play_time_;
        ar& idle_time_;
    }

private:
    Id id_;
    std::string name_;
    Direction direction_{Direction::NORTH};
    Position position_{0.0, 0.0};
    Speed speed_{0.0, 0.0};
    std::vector<BagItem> bag_;
    size_t bag_capacity_;
    int score_{0};
    
    // Дополнительные внутренние счетчики
    double play_time_{0.0};
    double idle_time_{0.0};
};

}  // namespace model
