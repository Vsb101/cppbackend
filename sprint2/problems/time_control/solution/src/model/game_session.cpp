#include "game_session.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <vector>

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
        auto [x_min, x_max] = std::minmax(start.x, end.x);
        std::uniform_real_distribution<double> x_dist(x_min, x_max);
        dog->SetPosition({x_dist(eng), static_cast<double>(start.y)});
    } else {
        auto [y_min, y_max] = std::minmax(start.y, end.y);
        std::uniform_real_distribution<double> y_dist(y_min, y_max);
        dog->SetPosition({static_cast<double>(start.x), y_dist(eng)});
    }
}

void GameSession::Update(double dt_seconds) {
    static constexpr double HALF_WIDTH = 0.4;

    for (auto& dog : dogs_) {
        const auto& pos = dog->GetPosition();
        const auto& speed = dog->GetSpeed();
        if (speed.vx == 0 && speed.vy == 0) continue;

        auto roads = FindRoadsAtPosition(pos);
        if (roads.empty()) continue;

        // Считаем максимально возможные границы из всех дорог под собакой
        double min_x = 1e9, max_x = -1e9, min_y = 1e9, max_y = -1e9;
        for (const auto* r : roads) {
            auto s = r->GetStart();
            auto e = r->GetEnd();
            auto [x1, x2] = std::minmax({(double)s.x, (double)e.x});
            auto [y1, y2] = std::minmax({(double)s.y, (double)e.y});
            
            min_x = std::min(min_x, x1 - HALF_WIDTH);
            max_x = std::max(max_x, x2 + HALF_WIDTH);
            min_y = std::min(min_y, y1 - HALF_WIDTH);
            max_y = std::max(max_y, y2 + HALF_WIDTH);
        }

        Position next_pos = {pos.x + speed.vx * dt_seconds, pos.y + speed.vy * dt_seconds};
        
        // ВАЖНО: Ограничиваем движение только по той оси, по которой идем
        if (speed.vx != 0) {
            next_pos.y = pos.y; // Фиксируем перпендикулярную ось
            if (next_pos.x < min_x || next_pos.x > max_x) {
                next_pos.x = std::clamp(next_pos.x, min_x, max_x);
                dog->SetSpeed({0, 0});
            }
        } else if (speed.vy != 0) {
            next_pos.x = pos.x; // Фиксируем перпендикулярную ось
            if (next_pos.y < min_y || next_pos.y > max_y) {
                next_pos.y = std::clamp(next_pos.y, min_y, max_y);
                dog->SetSpeed({0, 0});
            }
        }

        dog->SetPosition(next_pos);
    }
}

std::vector<const Road*> GameSession::FindRoadsAtPosition(Position pos) const {
    std::vector<const Road*> result;
    // Осевые линии дорог имеют целочисленные координаты
    int x = static_cast<int>(std::round(pos.x));
    int y = static_cast<int>(std::round(pos.y));

    // Проверяем горизонтальные дороги, проходящие через этот Y
    for (const auto* road : map_->GetHorizontalRoadsAt(y)) {
        if (IsPositionOnRoad(pos, *road)) result.push_back(road);
    }
    // Проверяем вертикальные дороги, проходящие через этот X
    for (const auto* road : map_->GetVerticalRoadsAt(x)) {
        if (IsPositionOnRoad(pos, *road)) result.push_back(road);
    }
    return result;
}

bool GameSession::IsPositionOnRoad(Position pos, const Road& road) const {
    const double HALF_WIDTH = 0.4;
    
    auto start = road.GetStart();
    auto end = road.GetEnd();
    
    if (road.IsHorizontal()) {
        double road_y = static_cast<double>(start.y);
        double x_min = std::min(static_cast<double>(start.x), static_cast<double>(end.x));
        double x_max = std::max(static_cast<double>(start.x), static_cast<double>(end.x));
        
        return (pos.x >= x_min - HALF_WIDTH && pos.x <= x_max + HALF_WIDTH &&
                pos.y >= road_y - HALF_WIDTH && pos.y <= road_y + HALF_WIDTH);
    } else {
        double road_x = static_cast<double>(start.x);
        double y_min = std::min(static_cast<double>(start.y), static_cast<double>(end.y));
        double y_max = std::max(static_cast<double>(start.y), static_cast<double>(end.y));
        
        return (pos.x >= road_x - HALF_WIDTH && pos.x <= road_x + HALF_WIDTH &&
                pos.y >= y_min - HALF_WIDTH && pos.y <= y_max + HALF_WIDTH);
    }
}

const Road* GameSession::FindRoadAndBounds(Position pos) const {
    static constexpr double ROAD_WIDTH = 4.0; // Полная ширина дороги для проверки попадания
    
    for (const auto& road : map_->GetRoads()) {
        auto start = road.GetStart();
        auto end = road.GetEnd();
        
        if (road.IsHorizontal()) {
            // Для горизонтальной дороги проверяем Y координату
            double road_y = static_cast<double>(start.y);
            if (std::abs(pos.y - road_y) <= ROAD_WIDTH) {
                return &road;
            }
        } else {
            // Для вертикальной дороги проверяем X координату
            double road_x = static_cast<double>(start.x);
            if (std::abs(pos.x - road_x) <= ROAD_WIDTH) {
                return &road;
            }
        }
    }
    return nullptr;
}

}  // namespace model
