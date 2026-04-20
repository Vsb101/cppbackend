#include "game_session.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <iostream>
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
    for (auto& dog : dogs_) {
        const auto& pos = dog->GetPosition();
        const auto& speed = dog->GetSpeed();
        
        if (speed.vx == 0 && speed.vy == 0) continue;

        // Находим все дороги, которые пересекаются в текущей позиции (перекрёсток)
        auto roads = FindRoadsAtPosition(pos);
        if (roads.empty()) continue; // Если собака не на дороге - не двигаем её
        
        Position next_pos = pos;
        bool stopped = false;
        const double HALF_WIDTH = 0.4;
        
        // Вычисляем потенциальную новую позицию
        next_pos.x = pos.x + speed.vx * dt_seconds;
        next_pos.y = pos.y + speed.vy * dt_seconds;
        
        // Проверяем, остаётся ли новая позиция в пределах любой из дорог
        bool can_move = false;
        for (const Road* r : roads) {
            if (IsPositionOnRoad(next_pos, *r)) {
                can_move = true;
                break;
            }
        }
        
        if (can_move) {
            // Двигаем собаку
            dog->SetPosition(next_pos);
        } else {
            // Проверяем, можно двигаться хотя бы по одной оси
            Position test_pos_x = {next_pos.x, pos.y};
            Position test_pos_y = {pos.x, next_pos.y};
            
            bool can_move_x = false, can_move_y = false;
            for (const Road* r : roads) {
                if (IsPositionOnRoad(test_pos_x, *r)) can_move_x = true;
                if (IsPositionOnRoad(test_pos_y, *r)) can_move_y = true;
            }
            
            if (speed.vx != 0 && speed.vy == 0) {
                // Только горизонтальное движение
                if (can_move_x) {
                    dog->SetPosition(test_pos_x);
                } else {
                    dog->SetSpeed({0.0, 0.0});
                }
            } else if (speed.vy != 0 && speed.vx == 0) {
                // Только вертикальное движение
                if (can_move_y) {
                    dog->SetPosition(test_pos_y);
                } else {
                    dog->SetSpeed({0.0, 0.0});
                }
            } else {
                // Движение по обоим осям - это не должно happen, так как скорость всегда только по одной оси
                dog->SetSpeed({0.0, 0.0});
            }
        }
    }
}

std::vector<const Road*> GameSession::FindRoadsAtPosition(Position pos) const {
    std::vector<const Road*> result;
    const double HALF_WIDTH = 0.4;
    
    for (const auto& road : map_->GetRoads()) {
        if (IsPositionOnRoad(pos, road)) {
            result.push_back(&road);
        }
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
