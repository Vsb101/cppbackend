#include "game_session.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <vector>
#include <chrono>
#include "collision_detector.h"
#include "geom.h"

namespace model {

namespace {

// Ширина объектов для коллизий
constexpr double DOG_WIDTH = 0.6;
constexpr double BASE_WIDTH = 0.5;
constexpr double ITEM_WIDTH = 0.0;

// Геометрия
constexpr double HALF_WIDTH = 0.4;
constexpr double EPS = 1e-9;
constexpr double EPS_POSITION = 0.001;
constexpr double INF = 1e9;
constexpr double OFFICE_COLLECTION_RADIUS = BASE_WIDTH / 2 + DOG_WIDTH / 2;
constexpr double OFFICE_COLLECTION_RADIUS_SQ = OFFICE_COLLECTION_RADIUS * OFFICE_COLLECTION_RADIUS;
constexpr double ITEM_COLLECTION_RADIUS = DOG_WIDTH / 2;
constexpr double ITEM_COLLECTION_RADIUS_SQ = ITEM_COLLECTION_RADIUS * ITEM_COLLECTION_RADIUS;

class DogGathererProvider : public collision_detector::ItemGathererProvider {
public:
    DogGathererProvider(const std::vector<DogMovement>& movements,
                        const std::unordered_map<uint32_t, LostObject>& lost_objects)
        : movements_(movements) {
        item_ids_.reserve(lost_objects.size());
        item_positions_.reserve(lost_objects.size());
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
            DOG_WIDTH
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

} // namespace

// ============================================================================
// Конструктор
// ============================================================================

GameSession::GameSession(std::shared_ptr<Map> map, loot_gen::LootGenerator loot_gen)
    : map_(std::move(map))
    , id_(*(map_->GetId()))
    , loot_generator_(std::move(loot_gen)) {}

// ============================================================================
// Игровая логика
// ============================================================================

std::shared_ptr<Dog> GameSession::CreateDog(const std::string& name, bool randomize) {
    std::lock_guard lock(mutex_);

    size_t bag_capacity = map_->GetBagCapacity();
    auto dog = std::make_shared<Dog>(Dog::Id{next_dog_id_++}, name, bag_capacity);

    const auto& roads = map_->GetRoads();

    if (randomize && !roads.empty()) {
        PutDogInRndPosition(dog);
    } else {
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

void GameSession::Update(double dt_seconds) {
    std::lock_guard lock(mutex_);

    using namespace std::chrono;
    milliseconds delta = duration_cast<milliseconds>(duration<double>(dt_seconds));
    GenerateLoot(delta);

    auto movements = PrepareDogMovements();
    MoveDogs(dt_seconds, movements);
    ProcessGatherEvents(movements);
}

// ============================================================================
// Геттеры
// ============================================================================

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

const std::unordered_map<uint32_t, LostObject>& GameSession::GetLostObjects() const noexcept {
    std::lock_guard lock(mutex_);
    return lost_objects_;
}

// ============================================================================
// Движение
// ============================================================================

std::vector<DogMovement> GameSession::PrepareDogMovements() {
    std::vector<DogMovement> movements;
    movements.reserve(dogs_.size());

    for (auto& dog : dogs_) {
        Position start_pos = dog->GetPosition();
        movements.push_back({start_pos, start_pos, dog});
    }

    return movements;
}

void GameSession::MoveDogs(double dt_seconds, std::vector<DogMovement>& movements) {
    for (size_t idx = 0; idx < dogs_.size(); ++idx) {
        auto& dog = dogs_[idx];
        auto& movement = movements[idx];

        auto speed = dog->GetSpeed();
        if (speed.vx == 0 && speed.vy == 0) continue;

        double remaining_dt = dt_seconds;

        while (remaining_dt > EPS) {
            Position pos = dog->GetPosition();
            auto roads = FindRoadsAtPosition(pos);

            if (roads.empty()) {
                break;
            }

            double min_x = INF, max_x = -INF, min_y = INF, max_y = -INF;
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
            movement.end_pos = clamped_pos;
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
}

// ============================================================================
// Сбор предметов
// ============================================================================

void GameSession::ProcessGatherEvents(const std::vector<DogMovement>& movements) {
    DogGathererProvider provider(movements, lost_objects_);
    auto events = collision_detector::FindGatherEvents(provider);

    std::unordered_map<uint32_t, std::pair<size_t, double>> item_events;

    for (const auto& evt : events) {
        uint32_t item_id = provider.GetItemId(evt.item_id);
        auto it = item_events.find(item_id);
        if (it == item_events.end() || evt.time < it->second.second) {
            item_events[item_id] = {evt.gatherer_id, evt.time};
        }
    }

    struct EventWithTime {
        uint32_t item_id;
        size_t dog_idx;
        double time;
    };

    std::vector<EventWithTime> sorted_events;
    sorted_events.reserve(item_events.size());
    for (const auto& [item_id, dog_time_pair] : item_events) {
        const auto& [dog_idx, time] = dog_time_pair;
        sorted_events.push_back({item_id, dog_idx, time});
    }

    std::sort(sorted_events.begin(), sorted_events.end(),
              [](const EventWithTime& a, const EventWithTime& b) {
                  return a.time < b.time;
              });

    // Кэшируем стоимости типов предметов
    auto loot_types_count = map_->GetLootTypesCount();
    std::vector<int> loot_type_values(loot_types_count);
    for (size_t i = 0; i < loot_types_count; ++i) {
        loot_type_values[i] = map_->GetLootTypeValue(i);
    }

    for (const auto& evt : sorted_events) {
        auto& dog = dogs_[evt.dog_idx];

        auto it = lost_objects_.find(evt.item_id);
        if (it == lost_objects_.end() || it->second.collected) {
            continue;
        }

        Position event_pos = GetEventPosition(movements[evt.dog_idx], evt.time);

        if (IsDogAtOffice(event_pos)) {
            auto returned_items = dog->ClearBag();
            for (const auto& item : returned_items) {
                dog->AddScore(loot_type_values[item.type]);
            }
        }

        if (dog->IsBagFull()) {
            continue;
        }

        const auto& loot = it->second;
        dog->AddToBag(loot.id, loot.type);
        lost_objects_[evt.item_id].collected = true;
    }
}

Position GameSession::GetEventPosition(const DogMovement& movement, double time) const {
    if (time <= 0) {
        return movement.start_pos;
    }
    if (time >= 1) {
        return movement.end_pos;
    }
    return {
        movement.start_pos.x + time * (movement.end_pos.x - movement.start_pos.x),
        movement.start_pos.y + time * (movement.end_pos.y - movement.start_pos.y)
    };
}

bool GameSession::IsDogAtOffice(const Position& pos) const {
    for (const auto& office : map_->GetOffices()) {
        auto off_pos = office.GetPosition();
        double dx = pos.x - static_cast<double>(off_pos.x);
        double dy = pos.y - static_cast<double>(off_pos.y);
        double sq_dist = dx * dx + dy * dy;
        if (sq_dist <= OFFICE_COLLECTION_RADIUS_SQ) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// Генерация лута
// ============================================================================

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

// ============================================================================
// Работа с дорогами
// ============================================================================

const Road* GameSession::FindRoadAndBounds(Position pos) const {
    for (const auto& road : map_->GetRoads()) {
        if (IsPositionOnRoad(pos, road)) return &road;
    }
    return nullptr;
}

std::vector<const Road*> GameSession::FindRoadsAtPosition(Position pos) const {
    std::vector<const Road*> result;
    result.reserve(map_->GetHorizontalRoadsAt(pos.y).size() + map_->GetVerticalRoadsAt(pos.x).size());
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
    auto start = road.GetStart();
    auto end = road.GetEnd();
    auto [x_min, x_max] = std::minmax({static_cast<double>(start.x), static_cast<double>(end.x)});
    auto [y_min, y_max] = std::minmax({static_cast<double>(start.y), static_cast<double>(end.y)});

    return (pos.x >= x_min - HALF_WIDTH - EPS_POSITION && pos.x <= x_max + HALF_WIDTH + EPS_POSITION &&
            pos.y >= y_min - HALF_WIDTH - EPS_POSITION && pos.y <= y_max + HALF_WIDTH + EPS_POSITION);
}

// ============================================================================
// Методы для восстановления состояния (сериализация)
// ============================================================================

void GameSession::RestoreDogs(const std::vector<Dog>& dogs, uint32_t next_dog_id) {
    std::lock_guard lock(mutex_);
    dogs_.clear();
    for (const auto& dog : dogs) {
        // Создаем новую собаку с теми же данными
        auto new_dog = std::make_shared<Dog>(dog.GetId(), dog.GetName(), dog.GetBagCapacity());
        new_dog->SetDirection(dog.GetDirection());
        new_dog->SetPosition(dog.GetPosition());
        new_dog->SetSpeed(dog.GetSpeed());
        
        // Восстанавливаем содержимое сумки
        for (const auto& item : dog.GetBag()) {
            new_dog->AddToBag(item.id, item.type);
        }
        
        new_dog->AddScore(dog.GetScore());
        dogs_.push_back(new_dog);
    }
    next_dog_id_ = next_dog_id;
}

void GameSession::RestoreLostObjects(const std::unordered_map<uint32_t, LostObject>& objects, uint32_t next_loot_id) {
    std::lock_guard lock(mutex_);
    lost_objects_ = objects;
    next_loot_id_ = next_loot_id;
}

}  // namespace model
