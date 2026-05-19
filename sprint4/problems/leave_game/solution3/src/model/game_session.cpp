#include "game_session.h"

#include "collision_detector.h"

#include <memory>
#include <optional>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <variant>

namespace model {
using namespace std::string_literals;

Dog* GameSession::AddDog(Position pos, const Dog::Name& name) {
    const size_t index = dogs_.size();  // Получаем незанятый индекс
    // Здесь должна быть генерация уникального Id собаки. Пока берём просто индекс.
    // Пробуем добавить
    if (auto [it, inserted] = dog_id_to_index_.emplace(next_dog_index_, index); !inserted) {
        throw std::invalid_argument("Dog with id "s + std::to_string(index) + " already exists"s);
    } else {
        // Создаём на основе индекса и имени экземпляр собаки
        try {
            dogs_.emplace_back(std::make_shared<Dog>(model::Dog::Id(index), name, pos));
        } catch (...) {
            dog_id_to_index_.erase(it);
            throw;
        }
    }
    ++next_dog_index_;
    return dogs_[index].get();
}

void GameSession::AddDog(const Dog& dog) {
    const size_t id = *dog.GetId();  // Получаем сохранённый индекс
    const size_t index = dogs_.size();  // Получаем незанятый индекс
    // Пробуем добавить
    if (auto [it, inserted] = dog_id_to_index_.emplace(id, index); !inserted) {
        throw std::invalid_argument("Dog with id "s + std::to_string(index) + " already exists"s);
    } else {
        if ( index >= dogs_.size() ) {
            dogs_.resize(index + 1);
        }
        dogs_[index] = std::make_shared<Dog>(dog);
    }
    ++next_dog_index_;
}

void GameSession::AddItem(Position pos, Item::Type& type) {
    const size_t index = items_.size();  // Получаем незанятый индекс
    // Пробуем добавить
    if (auto [it, inserted] = item_id_to_index_.emplace(next_item_index_, index); !inserted) {
        throw std::invalid_argument("Item with id "s + std::to_string(index) + " already exists"s);
    } else {
        // Создаём на основе индекса и имени экземпляр предмета
        try {
            items_.emplace_back(std::make_shared<Item>(Item::Id(index), type, pos));
        } catch (...) {
            item_id_to_index_.erase(it);
            throw;
        }
    }
    ++next_item_index_;
}

void GameSession::AddItem(std::shared_ptr<Item> item) {
    const size_t id = *item->GetId();  // Получаем сохранённый индекс
    const size_t index = items_.size();  // Получаем незанятый индекс
    // Пробуем добавить
    if (auto [it, inserted] = item_id_to_index_.emplace(id, index); !inserted) {
        throw std::invalid_argument("Item with id "s + std::to_string(index) + " already exists"s);
    } else {
        if ( index >= items_.size() ) {
            items_.resize(index + 1);
        }
        items_[index] = item;
    }
    ++next_item_index_;
}

void GameSession::Tick(TimeType dt) noexcept {
    // Список предметов для обработки коллизий
    std::vector<collision_detector::Item> col_items;
    for (const auto& [item_id, item_insex] : item_id_to_index_) {
        col_items.push_back(
            { *item_id, 
            {items_[item_insex]->GetPosition().x, items_[item_insex]->GetPosition().y},
            items_[item_insex]->GetWidth() }
            );
    }

    // Список офисов для обработки коллизий
    std::vector<collision_detector::Rect> col_offices;
    for (auto office : map_->GetOffices()) {
        collision_detector::Rect rect(  office.GetPosition().x, office.GetPosition().y, 
                                        office.GetOffset().dx, office.GetOffset().dy);
        col_offices.push_back(rect);
    }

    // Список сборщиков для обработки коллизий
    std::vector<collision_detector::Gatherer> col_gatherers;

    // Перемещаем собак
    for (auto dog : dogs_) {
        // Запоминаем старые координаты для сбора предметов
        auto old_pos = dog->GetPosition();
        // Находим новые координаты
        auto pos = MoveDog(*dog, dt);
        // и устанавливаем их
        dog->SetPosition(pos);
        // Сохраняем "собирателя" 
        col_gatherers.push_back({ {old_pos.x, old_pos.y}, {pos.x, pos.y}, dog->GetWidth() });
    }

    // Обрабатываем коллизии собак и предметов
    collision_detector::VectorItemGathererProvider item_provider(col_items, col_gatherers);
    auto item_collisions = collision_detector::FindGatherEvents(item_provider);
    // Обрабатываем коллизии собак и баз(офисов)
    collision_detector::VectorOfficeSaveProvider office_provider(col_offices, col_gatherers);
    auto office_collisions = collision_detector::FindOfficeSaveEvents(office_provider);

    std::set<collision_detector::AllIvents> all_events;
    for (auto item_event : item_collisions) {
        all_events.insert(item_event);
    }
    for (auto office_event : office_collisions) {
        all_events.insert(office_event);
    }

    std::set<Item::Id> collected_items;

    // События уже отсортированы по времени, потому что оператор сравнения переопределён
    for (auto event : all_events) {
        std::visit([this, &col_items, &collected_items](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, collision_detector::OfficeSaveEvent>) {
                // Если встретили событие офиса
                auto dog = dogs_.at(arg.gatherer_id);
                // Сдаём предметы (начисляем очки + очищаем рюкзак)
                dog->SaveBag();
            } else if constexpr (std::is_same_v<T, collision_detector::GatheringEvent>) {
                // Если встретили событие сбора
                auto collected_item_id = Item::Id(col_items[arg.item_id].id);
                auto item_index = item_id_to_index_[collected_item_id];
                auto item = items_[item_index];
                auto dog = dogs_.at(arg.gatherer_id);
                // Пробуем поднять предмет (лезет в рюкзак и не собрали ранее)
                if (dog->GetBagSize() < map_->GetBagCapacity() && !collected_items.contains(collected_item_id)) {
                    // Запоминаем, что предмет собран
                    collected_items.insert(collected_item_id);
                    // Убираем предмет в рюкзак (создаётся копия)
                    dog->TakeItem(item);
                }
            }
        }, event);
    }

    // Очищаем список предметов от нулевых указателей
    ClearCollectedItems(collected_items);

    // Генерируем новые предметы при необходимости
    size_t n_items = item_id_to_index_.size();
    size_t n_dogs = dog_id_to_index_.size();
    unsigned n_new_items = loot_generator_->Generate(dt, n_items, n_dogs);
    for (unsigned i = 0; i < n_new_items; ++i) {
        Item::Type rand_type = std::rand() % map_->GetNLootTypes();
        Position pos = map_->GetRandomPointOnMap();
        AddItem(pos, rand_type);
    }
}

 Dog* GameSession::FindDog(const Dog::Id& id) noexcept {
    if (auto it = dog_id_to_index_.find(id); it != dog_id_to_index_.end()) {
        return dogs_.at(it->second).get();
    }
    return nullptr;
}

void GameSession::RemoveDog(const Dog::Id& id) {
    if (auto it = dog_id_to_index_.find(id); it != dog_id_to_index_.end()) {
        auto index = it->second;
        // Удаляем id собаки из таблицы
        dog_id_to_index_.erase(id);
        if (!dog_id_to_index_.empty()) {   // Остались ещё собаки
            // находим id последнего последней
            auto last_dog_id = GetDogIdByIndex(dogs_.size() - 1);
            // Перекладка требуется, если пёс не из конца списка
            if (last_dog_id) {
                // На это место удалённого пса кладём указатель на собаку из конца списка
                dogs_[index] = dogs_.back();
                // Заносим новый индекс перемещённого пса в таблицу
                dog_id_to_index_[*last_dog_id] = index;
            }
        }
        // Удаляем указатель из конца списка
        dogs_.pop_back();
    }    
}

std::optional<Dog::Id> GameSession::GetDogIdByIndex(size_t index) {
    for ( auto [dog_id, dog_index] : dog_id_to_index_ ) {
        if (dog_index == index) {
            return dog_id;
        }
    }
    return std::nullopt;
}

// Вспомогательные функции
bool isPointOnRoad(Position pos, const Road& road) {
    auto ox = road.GetOxProjection();
    auto oy = road.GetOyProjection();
    return pos.x >= ox.first && pos.x <= ox.second && pos.y >= oy.first && pos.y <= oy.second;
}

struct Move {
    Position pos;
    DynamicCoord path;
};

Move GetMoveOnRoad(Position start_pos, Position end_pos, Road road) {
    StartEndCoord oxMove = {std::min(start_pos.x, end_pos.x), std::max(start_pos.x, end_pos.x)};
    StartEndCoord oyMove = {std::min(start_pos.y, end_pos.y), std::max(start_pos.y, end_pos.y)};

    DynamicCoord start, end;
    StartEndCoord roadProjection;

    bool isHorizontal = oxMove.second-oxMove.first > oyMove.second-oyMove.first;
    // Уходим на прямую
    if (isHorizontal) {  // Перемещение по X
        start = start_pos.x;
        end = end_pos.x;
        roadProjection = road.GetOxProjection();
    } else {    // Перемещение по Y
        start = start_pos.y;
        end = end_pos.y;
        roadProjection = road.GetOyProjection();
    }

    DynamicCoord intersect;
    if ( start < end ) { // Движение вправо
        intersect = std::min(end,roadProjection.second);
    } else {    // Движение влево
        intersect = std::max(end,roadProjection.first);
    }

    DynamicCoord len = std::fabs(intersect - start);

    // Начальная точка уже точно на дороге, значит пересечение проверять не нужно

    if (isHorizontal) {  // Перемещение по X
        return {.pos = {intersect, start_pos.y}, .path = len};
    } else {
        return {.pos = {start_pos.x, intersect}, .path = len};
    }
}

std::optional<Item::Id> GameSession::GetItemIdByIndex(size_t index) {
    for ( auto [item_id, item_index] : item_id_to_index_ ) {
        if (item_index == index) {
            return item_id;
        }
    }
    return std::nullopt;
}

void GameSession::ClearCollectedItems(const std::set<Item::Id>& collected_items) {
    for ( auto item_id : collected_items ) {
        if (auto it = item_id_to_index_.find(item_id); it != item_id_to_index_.end()) {
            auto item_index = it->second;
            // Удаляем id предмета из таблицы
            item_id_to_index_.erase(item_id);
            if (!item_id_to_index_.empty()) {   // Остались ещё предметы
                // находим id последнего предмета в списке предметов
                auto last_item_id = GetItemIdByIndex(items_.size() - 1);
                // Перекладка требуется, если предмет не из конца списка
                if (last_item_id) {
                    // На это место удалённого предмета кладём указатель на предмет из конца списка
                    items_[item_index] = items_.back();
                    // Заносим новый индекс перемещённого предмета в таблицу
                    item_id_to_index_[*last_item_id] = item_index;
                } 
            }
            // Удаляем указатель из конца списка
            items_.pop_back();
        }
    }
}


Position GameSession::MoveDog(Dog& dog, TimeType dt) noexcept {
    // Время в игре прибавляем всегда
    dog.AddPlayTime(dt.count()/1000.0);
    if( dog.GetSpeed() == Speed{0.0, 0.0} ) {
        // Время простоя прибавляем при нулевой скорости
        dog.AddSleepTime(dt.count()/1000.0);
        return dog.GetPosition();
    }

    // Скорость не ноль, значит собака в движении - сбрасываем время сна
    dog.ResetSleepTime();

    // Направление движения собаки
    Direction dog_direction = dog.GetDirection();

    auto dog_start_pos = dog.GetPosition();
    // Сначала находим конечную позицию в предположении, что туда можно попасть
    auto dog_end_pos = dog.GetPosition()+dog.GetSpeed()*dt;

    // Дороги, на которых начало
    std::unordered_set<const Road*> withStartRoads;
    // Дороги, на которых конец
    std::unordered_set<const Road*> withEndRoads;
    const auto& roads = map_->GetRoads();
    for ( size_t i = 0; i != roads.size(); ++i ) {
        if (isPointOnRoad(dog_start_pos, roads[i])) {
            withStartRoads.insert(&roads[i]);
        }
        if (isPointOnRoad(dog_end_pos, roads[i])) {
            withEndRoads.insert(&roads[i]);
        }
    }

    // Находим дороги, на которых есть обе точки
    std::unordered_set<const Road*> intersect;
    for ( auto road : withStartRoads ) {
        if ( withEndRoads.find(road) != withEndRoads.end() ) {
            intersect.insert(road);
        }
    }
    
    if (!intersect.empty()) {   // Если есть общие, то вычисляем перемещение на них
        withStartRoads = intersect;
    }

    // Нужно найти максимальное перемещение, которое может сделать собака
    Move res(dog_start_pos, 0.0);
    for ( auto road : withStartRoads ) {
        auto move = GetMoveOnRoad(dog_start_pos, dog_end_pos, *road);
        if (move.path > res.path) {
            res = move;
        }
    }

    if( dog_end_pos != res.pos ) {
        dog.SetSpeed(0.0, std::nullopt);
    }

    return res.pos;
}

}  // namespace model
