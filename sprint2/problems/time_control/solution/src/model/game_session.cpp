#include "game_session.h"
#include <random>
#include <algorithm>
#include <cmath>

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

        Position next_pos = {
            pos.x + speed.vx * dt_seconds,
            pos.y + speed.vy * dt_seconds
        };

        // Находим границы. Нам нужно знать, по какой оси мы движемся
        auto bounds = GetMovementBounds(pos);

        if (speed.vx != 0) { // Горизонтальное движение
            if (next_pos.x < bounds.min_x) {
                next_pos.x = bounds.min_x;
                dog->SetSpeed({0.0, 0.0});
            } else if (next_pos.x > bounds.max_x) {
                next_pos.x = bounds.max_x;
                dog->SetSpeed({0.0, 0.0});
            }
            next_pos.y = pos.y; // Y не меняется
        } else if (speed.vy != 0) { // Вертикальное движение
            if (next_pos.y < bounds.min_y) {
                next_pos.y = bounds.min_y;
                dog->SetSpeed({0.0, 0.0});
            } else if (next_pos.y > bounds.max_y) {
                next_pos.y = bounds.max_y;
                dog->SetSpeed({0.0, 0.0});
            }
            next_pos.x = pos.x; // X не меняется
        }
        dog->SetPosition(next_pos);
    }
}

// Изменим структуру возврата, чтобы возвращать границы по обеим осям сразу
struct AllBounds {
    double min_x, max_x;
    double min_y, max_y;
};

GameSession::Bounds GameSession::GetMovementBounds(Position pos) const {
    static constexpr double HALF_WIDTH = 0.4;
    
    // По умолчанию считаем, что собака не может сдвинуться
    double min_x = pos.x, max_x = pos.x;
    double min_y = pos.y, max_y = pos.y;

    bool first = true;

    for (const auto& road : map_->GetRoads()) {
        auto start = road.GetStart();
        auto end = road.GetEnd();
        
        // Определяем границы "асфальта" для конкретной дороги
        double r_min_x = std::min(static_cast<double>(start.x), static_cast<double>(end.x)) - HALF_WIDTH;
        double r_max_x = std::max(static_cast<double>(start.x), static_cast<double>(end.x)) + HALF_WIDTH;
        double r_min_y = std::min(static_cast<double>(start.y), static_cast<double>(end.y)) - HALF_WIDTH;
        double r_max_y = std::max(static_cast<double>(start.y), static_cast<double>(end.y)) + HALF_WIDTH;

        // Проверяем, находится ли текущая точка внутри этой дороги
        if (pos.x >= r_min_x && pos.x <= r_max_x &&
            pos.y >= r_min_y && pos.y <= r_max_y) {
            
            if (first) {
                min_x = r_min_x; max_x = r_max_x;
                min_y = r_min_y; max_y = r_max_y;
                first = false;
            } else {
                // Если дорог несколько (перекресток), расширяем область дозволенного
                min_x = std::min(min_x, r_min_x);
                max_x = std::max(max_x, r_max_x);
                min_y = std::min(min_y, r_min_y);
                max_y = std::max(max_y, r_max_y);
            }
        }
    }
    return {min_x, max_x, min_y, max_y};
}
        }
    }
    return {min_x, max_x, min_y, max_y};
}

}  // namespace model
