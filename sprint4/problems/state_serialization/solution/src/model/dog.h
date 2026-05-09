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
        uint32_t id = 0;
        size_t type = 0;

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
        if (bag_.size() >= bag_capacity_) {
            return false;
        }
        bag_.push_back({item_id, item_type});
        return true;
    }

    std::vector<BagItem> ClearBag() {
        std::vector<BagItem> result = std::move(bag_);
        bag_.clear();
        return result;
    }

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
    }

private:
    Id id_;
    std::string name_;
    Direction direction_{Direction::NORTH};
    Position position_{0.0, 0.0};
    Speed speed_{0.0, 0.0};
    std::vector<BagItem> bag_;
    size_t bag_capacity_;
    int score_ = 0;
};

}  // namespace model
