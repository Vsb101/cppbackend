#pragma once

#include "../other/tagged.h"
#include <string>
#include <vector>
#include <cstdint>

namespace model {

enum class Direction { NORTH, SOUTH, WEST, EAST };

struct Position { double x, y; };
struct Speed { double vx, vy; };

class Dog {
public:
    using Id = util::Tagged<size_t, Dog>;

    struct BagItem {
        uint32_t id;
        size_t type;

        BagItem() = default;
        BagItem(uint32_t item_id, size_t item_type) : id(item_id), type(item_type) {}
    };

    Dog(Id id, std::string name, size_t bag_capacity = 3)
        : id_(std::move(id))
        , name_(std::move(name))
        , bag_capacity_(bag_capacity) {
    }

    const Id& GetId() const { return id_; }
    const std::string& GetName() const { return name_; }

    Direction GetDirection() const { return direction_; }
    void SetDirection(Direction direction) { direction_ = direction; }

    const Position& GetPosition() const { return position_; }
    void SetPosition(Position position) { position_ = position; }

    const Speed& GetSpeed() const { return speed_; }
    void SetSpeed(Speed speed) { speed_ = speed; }

    const std::vector<BagItem>& GetBag() const { return bag_; }
    size_t GetBagCapacity() const { return bag_capacity_; }
    bool IsBagFull() const { return bag_.size() >= bag_capacity_; }

    int GetScore() const { return score_; }
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
