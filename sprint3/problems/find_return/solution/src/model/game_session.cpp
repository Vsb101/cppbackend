#include "game_session.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <vector>
#include <chrono>
#include "collision_detector.h"
#include "geom.h"

namespace model {

//  Обновленный конструктор с инициализацией генератора
GameSession::GameSession(std::shared_ptr<Map> map, loot_gen::LootGenerator loot_gen)
    : map_(std::move(map))
    , id_(*(map_->GetId()))
    , loot_generator_(std::move(loot_gen)) {}

const GameSession::Id& GameSession::GetId() const noexcept {
    return id_;
}

std::shared_ptr<Map> GameSession::GetMap() const noexcept {
    return map_;
}

const std::vector<std::shared_ptr<Dog>>& GameSession::GetDogs() const noexcept {
    std::lock_guard lock(mutex_); 
    return dogs_;
}

const std::map<uint32_t, LostObject>& GameSession::GetLostObjects() const noexcept {
    std::lock_guard lock(mutex_); // Обязательно блокируем, так как Update может менять этот контейнер
    return lost_objects_;
}


std::shared_ptr<Dog> GameSession::CreateDog(const std::string& name, bool randomize) {
    std::lock_guard lock(mutex_);
    
    // Получаем вместимость рюкзака из карты (если есть) или дефолтную
    size_t bag_capacity = map_->GetBagCapacity();
    
    auto dog = std::make_shared<Dog>(Dog::Id{next_dog_id_++}, name, bag_capacity);
    
    const auto& roads = map_->GetRoads(); // Берем все дороги сразу
    
    if (randomize && !roads.empty()) {
        PutDogInRndPosition(dog);
    } else {
        // Защита: если дорог нет вообще, ставим в 0,0
        if (!roads.empty()) {
            const auto& road = roads.at(0);
            auto start = road.GetStart();
            dog->SetPosition({static_cast<double>(start.x), static_cast<double>(start.y)});
        } else {
            dog->SetPosition({0.0, 0.0});
        }
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

// 2. Метод для генерации предметов
void GameSession::GenerateLoot(std::chrono::milliseconds delta) {
    unsigned new_loot_count = loot_generator_.Generate(delta, lost_objects_.size(), dogs_.size());
    
    if (new_loot_count == 0) return;

    static std::default_random_engine eng{std::random_device{}()};
    size_t loot_types_count = map_->GetLootTypesCount();
    
    if (loot_types_count == 0) return;

    for (unsigned i = 0; i < new_loot_count; ++i) {
        std::uniform_int_distribution<size_t> type_dist(0, loot_types_count - 1);
        size_t type = type_dist(eng);

        const auto& roads = map_->GetRoads();
        std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
        const auto& road = roads[road_dist(eng)];
        
        Point start = road.GetStart();
        Point end = road.GetEnd();
        double x, y;

        if (road.IsHorizontal()) {
            auto [x_min, x_max] = std::minmax(start.x, end.x);
            std::uniform_real_distribution<double> x_dist(x_min, x_max);
            x = x_dist(eng);
            y = static_cast<double>(start.y);
        } else {
            auto [y_min, y_max] = std::minmax(start.y, end.y);
            std::uniform_real_distribution<double> y_dist(y_min, y_max);
            x = static_cast<double>(start.x);
            y = y_dist(eng);
        }

        uint32_t id = next_loot_id_++;
        lost_objects_[id] = {id, type, {x, y}, false};
    }
}


void GameSession::Update(double dt_seconds) {
    std::lock_guard lock(mutex_);
    // Вызываем генерацию лута (переводим double секунды в миллисекунды)
    using namespace std::chrono;
    milliseconds delta = duration_cast<milliseconds>(duration<double>(dt_seconds));
    GenerateLoot(delta);

    static constexpr double HALF_WIDTH = 0.4;
    static constexpr double EPS = 1e-9;

    // Ширина объектов для коллизий
    static constexpr double DOG_WIDTH = 0.6;      // Игрок
    static constexpr double BASE_WIDTH = 0.5;     // База (офис)
    static constexpr double ITEM_WIDTH = 0.0;     // Предмет (точка)
    
    // Радиусы сбора: 
    // - Предмет: половина ширины игрока = 0.6 / 2 = 0.3
    // - База: половина ширины игрока + половина ширины базы = 0.6/2 + 0.5/2 = 0.55
    
    // --- ШАГ 1: Запоминаем начальные позиции всех собак ---
    struct DogMovement {
        Position start_pos;
        Position end_pos;
        std::shared_ptr<Dog> dog;
    };
    std::vector<DogMovement> movements;
    movements.reserve(dogs_.size());
    
    for (auto& dog : dogs_) {
        Position start_pos = dog->GetPosition();
        movements.push_back({start_pos, start_pos, dog});
    }
    
    // --- ШАГ 2: Двигаем всех собак (текущая логика) ---
    for (size_t idx = 0; idx < dogs_.size(); ++idx) {
        auto& dog = dogs_[idx];
        auto& movement = movements[idx];
        
        auto speed = dog->GetSpeed();
        if (speed.vx == 0 && speed.vy == 0) continue;

        double remaining_dt = dt_seconds;
        
        while (remaining_dt > EPS) {
            Position pos = dog->GetPosition();
            auto roads = FindRoadsAtPosition(pos);
            
            if (roads.empty()) break; 

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

            Position clamped_pos = {
                std::clamp(potential_next_pos.x, min_x, max_x),
                std::clamp(potential_next_pos.y, min_y, max_y)
            };

            double moved_x = clamped_pos.x - pos.x;
            double moved_y = clamped_pos.y - pos.y;
            double moved_dist = std::sqrt(moved_x * moved_x + moved_y * moved_y);
            
            double v_mag = std::sqrt(speed.vx * speed.vx + speed.vy * speed.vy);
            double spent_dt = (v_mag > EPS) ? (moved_dist / v_mag) : remaining_dt;

            dog->SetPosition(clamped_pos);
            movement.end_pos = clamped_pos;  // Обновляем конечную позицию
            remaining_dt -= spent_dt;

            if (std::abs(clamped_pos.x - potential_next_pos.x) > EPS || 
                std::abs(clamped_pos.y - potential_next_pos.y) > EPS) {
                
                auto roads_at_edge = FindRoadsAtPosition(clamped_pos);
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
                break; 
            }
        }
    }
    
    // --- ШАГ 3: Обработка сбора предметов через collision_detector ---
    // Создаем провайдер для FindGatherEvents
    class DogGathererProvider : public collision_detector::ItemGathererProvider {
    public:
        DogGathererProvider(const std::vector<DogMovement>& movements, 
                           const std::map<uint32_t, LostObject>& lost_objects)
            : movements_(movements) {
            // Собираем ID несобранных предметов
            for (const auto& [id, loot] : lost_objects) {
                if (!loot.collected) {
                    item_ids_.push_back(id);
                    item_positions_.push_back(
                        geom::Point2D{loot.pos.first, loot.pos.second}
                    );
                }
            }
        }
        
        size_t ItemsCount() const override {
            return item_ids_.size();
        }
        
        collision_detector::Item GetItem(size_t idx) const override {
            return collision_detector::Item{
                item_positions_[idx],
                ITEM_WIDTH
            };
        }
        
        size_t GatherersCount() const override {
            return movements_.size();
        }
        
        collision_detector::Gatherer GetGatherer(size_t idx) const override {
            const auto& m = movements_[idx];
            return collision_detector::Gatherer{
                geom::Point2D{m.start_pos.x, m.start_pos.y},
                geom::Point2D{m.end_pos.x, m.end_pos.y},
                DOG_WIDTH  // Ширина игрока
            };
        }
        
        uint32_t GetItemId(size_t idx) const {
            return item_ids_[idx];
        }
        
    private:
        const std::vector<DogMovement>& movements_;
        std::vector<uint32_t> item_ids_;
        std::vector<geom::Point2D> item_positions_;
    };
    
    DogGathererProvider provider(movements, lost_objects_);
    auto events = collision_detector::FindGatherEvents(provider);
    
    // --- ШАГ 4: Обработка событий в хронологическом порядке ---
    // Для каждого предмета находим первого собирателя (по времени)
    std::map<uint32_t, std::pair<size_t, double>> item_events;  // item_id -> (dog_idx, time)
    
    for (const auto& evt : events) {
        uint32_t item_id = provider.GetItemId(evt.item_id);
        if (!item_events.contains(item_id)) {
            item_events[item_id] = {evt.gatherer_id, evt.time};
        } else {
            // Если несколько собирателей достигают предмета - берем того, кто раньше
            auto& [existing_dog_idx, existing_time] = item_events[item_id];
            if (evt.time < existing_time) {
                item_events[item_id] = {evt.gatherer_id, evt.time};
            }
        }
    }
    
    // Собираем события в вектор и сортируем по времени
    struct EventWithTime {
        uint32_t item_id;
        size_t dog_idx;
        double time;
    };
    std::vector<EventWithTime> sorted_events;
    for (const auto& [item_id, dog_time_pair] : item_events) {
        const auto& [dog_idx, time] = dog_time_pair;
        sorted_events.push_back({item_id, dog_idx, time});
    }
    std::sort(sorted_events.begin(), sorted_events.end(),
              [](const EventWithTime& a, const EventWithTime& b) {
                  return a.time < b.time;
              });
    
    // --- ШАГ 5: Обработка событий в хронологическом порядке ---
    for (const auto& evt : sorted_events) {
        auto& dog = dogs_[evt.dog_idx];
        
        // Проверяем, не собран ли уже предмет
        auto it = lost_objects_.find(evt.item_id);
        if (it == lost_objects_.end() || it->second.collected) {
            continue;
        }
        
        // Проверяем, достиг ли собакой офиса (базы) в момент события
        // Вычисляем позицию собаки в момент события
        const auto& movement = movements[evt.dog_idx];
        Position event_pos;
        if (evt.time <= 0) {
            event_pos = movement.start_pos;
        } else if (evt.time >= 1) {
            event_pos = movement.end_pos;
        } else {
            event_pos.x = movement.start_pos.x + evt.time * (movement.end_pos.x - movement.start_pos.x);
            event_pos.y = movement.start_pos.y + evt.time * (movement.end_pos.y - movement.start_pos.y);
        }
        
        // Проверяем, находится ли собака в офисе
        bool at_office = false;
        for (const auto& office : map_->GetOffices()) {
            auto off_pos = office.GetPosition();
            double dx = event_pos.x - static_cast<double>(off_pos.x);
            double dy = event_pos.y - static_cast<double>(off_pos.y);
            double dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= (BASE_WIDTH / 2 + DOG_WIDTH / 2)) {  // 0.55
                at_office = true;
                break;
            }
        }
        
        if (at_office) {
            // Сдаем все предметы на базе
            dog->ClearBag();
        }
        
        // Проверяем вместимость рюкзака
        if (dog->IsBagFull()) {
            // Рюкзак полон - пропускаем предмет
            continue;
        }
        
        // Берем предмет
        const auto& loot = it->second;
        dog->AddToBag(loot.id, loot.type);
        lost_objects_[evt.item_id].collected = true;
    }
}

std::vector<const Road*> GameSession::FindRoadsAtPosition(Position pos) const {
    std::vector<const Road*> result;
    int x = static_cast<int>(std::round(pos.x));
    int y = static_cast<int>(std::round(pos.y));

    for (const auto* road : map_->GetHorizontalRoadsAt(y)) {
        if (IsPositionOnRoad(pos, *road)) result.push_back(road);
    }
    for (const auto* road : map_->GetVerticalRoadsAt(x)) {
        if (IsPositionOnRoad(pos, *road)) result.push_back(road);
    }
    return result;
}

bool GameSession::IsPositionOnRoad(Position pos, const Road& road) const {
    static constexpr double HALF_WIDTH = 0.4;
    static constexpr double EPS = 0.001;
    
    auto start = road.GetStart();
    auto end = road.GetEnd();
    auto [x_min, x_max] = std::minmax({static_cast<double>(start.x), static_cast<double>(end.x)});
    auto [y_min, y_max] = std::minmax({static_cast<double>(start.y), static_cast<double>(end.y)});
    
    return (pos.x >= x_min - HALF_WIDTH - EPS && pos.x <= x_max + HALF_WIDTH + EPS &&
            pos.y >= y_min - HALF_WIDTH - EPS && pos.y <= y_max + HALF_WIDTH + EPS);
}

const Road* GameSession::FindRoadAndBounds(Position pos) const {
    for (const auto& road : map_->GetRoads()) {
        if (IsPositionOnRoad(pos, road)) return &road;
    }
    return nullptr;
}

}  // namespace model
