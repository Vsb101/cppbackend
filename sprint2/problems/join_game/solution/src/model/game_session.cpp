#include "../model/game_session.h"
#include <random>

namespace randomgen {

double RandomDouble(double min, double max) {
    std::random_device rd;
    std::default_random_engine eng(rd());
    std::uniform_real_distribution<double> distr(min, max);
    return distr(eng);
}

int RandomInt(int min, int max) {
    std::random_device rd;
    std::default_random_engine eng(rd());
    std::uniform_int_distribution<int> distr(min, max);
    return distr(eng);
}

}  // namespace randomgen

namespace model {

GameSession::GameSession(std::shared_ptr<Map> map, net::io_context& ioc)
    : map_(std::move(map))
    , strand_(std::make_shared<SessionStrand>(net::make_strand(ioc)))
    , id_(*map_->GetId()) {}

const GameSession::Id& GameSession::GetId() const noexcept {
    return id_;
}

std::shared_ptr<Map> GameSession::GetMap() {
    return map_;
}

std::shared_ptr<GameSession::SessionStrand> GameSession::GetStrand() {
    return strand_;
}

std::shared_ptr<Dog> GameSession::CreateDog(const std::string& dog_name) {
    auto dog = std::make_shared<Dog>(dog_name);
    dogs_.push_back(dog);
    PutDogInRndPosition(dog);
    return dog;
}

void GameSession::PutDogInRndPosition(std::shared_ptr<Dog> dog) {
    const auto& roads = map_->GetRoads();
    if (roads.empty()) {
        dog->SetPosition({0.0, 0.0});
        return;
    }

    int road_index = randomgen::RandomInt(0, static_cast<int>(roads.size()) - 1);
    const auto& road = roads[road_index];

    double x, y;
    if (road.IsHorizontal()) {
        x = randomgen::RandomDouble(road.GetStart().x, road.GetEnd().x);
        y = road.GetStart().y;
    } else {
        y = randomgen::RandomDouble(road.GetStart().y, road.GetEnd().y);
        x = road.GetStart().x;
    }
    dog->SetPosition({x, y});
}

}  // namespace model