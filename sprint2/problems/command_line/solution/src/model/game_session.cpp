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

std::shared_ptr<Dog> GameSession::CreateDog(const std::string& name, bool randomize) {
    auto dog = std::make_shared<Dog>(Dog::Id{next_dog_id_++}, name);
    if (randomize) {
        PutDogInRndPosition(dog);
    } else {
        // Ставим в начало первой дороги (требование задания)
        const auto& road = map_->GetRoads().at(0);
        auto start = road.GetStart();
        dog->SetPosition({static_cast<double>(start.x), static_cast<double>(start.y)});
    }
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
    static constexpr double EPS = 1e-9;

    for (auto& dog : dogs_) {
        auto speed = dog->GetSpeed();
        if (speed.vx == 0 && speed.vy == 0) continue;

        double remaining_dt = dt_seconds;
        
        // Цикл позволяет проехать через несколько стыкующихся дорог за один тик
        while (remaining_dt > EPS) {
            Position pos = dog->GetPosition();
            auto roads = FindRoadsAtPosition(pos);
            
            if (roads.empty()) break; // Собака вне дорог — стоп

            // Определяем границы текущего "коридора"
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

            Position potential_next_pos = {
                pos.x + speed.vx * remaining_dt,
                pos.y + speed.vy * remaining_dt
            };

            // Ограничиваем движение текущими дорогами
            Position clamped_pos = {
                std::clamp(potential_next_pos.x, min_x, max_x),
                std::clamp(potential_next_pos.y, min_y, max_y)
            };

            // Считаем, какое расстояние мы фактически прошли
            double moved_x = clamped_pos.x - pos.x;
            double moved_y = clamped_pos.y - pos.y;
            double moved_dist = std::sqrt(moved_x * moved_x + moved_y * moved_y);
            
            // Сколько времени это заняло
            double v_mag = std::sqrt(speed.vx * speed.vx + speed.vy * speed.vy);
            double spent_dt = (v_mag > EPS) ? (moved_dist / v_mag) : remaining_dt;

            dog->SetPosition(clamped_pos);
            remaining_dt -= spent_dt;

            // Если мы уперлись в границу (не доехали до желаемой точки)
            if (std::abs(clamped_pos.x - potential_next_pos.x) > EPS || 
                std::abs(clamped_pos.y - potential_next_pos.y) > EPS) {
                
                // Проверяем: а нет ли ПРЯМО ТУТ другой дороги, которая ведет дальше?
                auto roads_at_edge = FindRoadsAtPosition(clamped_pos);
                
                // Если новых дорог нет или они те же самые — мы реально в тупике
                bool found_new_road = false;
                for (auto* r_new : roads_at_edge) {
                    bool is_old = false;
                    for (auto* r_old : roads) if (r_new == r_old) is_old = true;
                    if (!is_old) { found_new_road = true; break; }
                }

                if (!found_new_road) {
                    dog->SetSpeed({0, 0});
                    break; 
                }
            } else {
                break; // Доехали до конца пути без препятствий
            }
        }
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
    static constexpr double HALF_WIDTH = 0.4;
    static constexpr double EPS = 0.001; // Маленький запас для стыков
    
    auto start = road.GetStart();
    auto end = road.GetEnd();
    auto [x_min, x_max] = std::minmax({static_cast<double>(start.x), static_cast<double>(end.x)});
    auto [y_min, y_max] = std::minmax({static_cast<double>(start.y), static_cast<double>(end.y)});
    
    // Добавляем EPS к границам дороги, чтобы "склеить" их для double
    return (pos.x >= x_min - HALF_WIDTH - EPS && pos.x <= x_max + HALF_WIDTH + EPS &&
            pos.y >= y_min - HALF_WIDTH - EPS && pos.y <= y_max + HALF_WIDTH + EPS);
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
