#include "game_session.h"
#include <random>
#include <algorithm>

namespace model {

GameSession::GameSession(std::shared_ptr<Map> map)
    : map_(std::move(map))
    , id_(*(map_->GetId())) {}

const GameSession::Id& GameSession::GetId() const noexcept {
    return id_;
}

std::shared_ptr<Map> GameSession::GetMap() const noexcept {
    return map_;
}

const std::vector<std::shared_ptr<Dog>>& GameSession::GetDogs() const noexcept {
    return dogs_;
}

std::shared_ptr<Dog> GameSession::CreateDog(const std::string& name) {
    // Исправлено: передаем ID из счетчика
    auto dog = std::make_shared<Dog>(Dog::Id{next_dog_id_++}, name);
    PutDogInRndPosition(dog);
    dogs_.push_back(dog);
    return dog;
}

void GameSession::PutDogInRndPosition(std::shared_ptr<Dog> dog) {
    const auto& roads = map_->GetRoads();
    if (roads.empty()) return;

    static std::default_random_engine eng{std::random_device{}()};
    std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
    
    const auto& road = roads[road_dist(eng)];
    Point start = road.GetStart();
    Point end = road.GetEnd();

    if (road.IsHorizontal()) {
        if (start.x > end.x) std::swap(start.x, end.x);
        std::uniform_real_distribution<double> x_dist(static_cast<double>(start.x), static_cast<double>(end.x));
        dog->SetPosition({x_dist(eng), static_cast<double>(start.y)});
    } else {
        if (start.y > end.y) std::swap(start.y, end.y);
        std::uniform_real_distribution<double> y_dist(static_cast<double>(start.y), static_cast<double>(end.y));
        dog->SetPosition({static_cast<double>(start.x), y_dist(eng)});
    }
}

}  // namespace model
