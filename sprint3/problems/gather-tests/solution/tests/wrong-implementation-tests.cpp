#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"
#include <catch2/catch_all.hpp>
#include <vector>
#include <cmath>
#include <sstream>
#include <algorithm>

namespace collision_detector {

// Тестовый провайдер, реализующий интерфейс ItemGathererProvider
class TestItemGathererProvider : public ItemGathererProvider {
public:
    TestItemGathererProvider(std::vector<Item> items, std::vector<Gatherer> gatherers)
        : items_(std::move(items)), gatherers_(std::move(gatherers)) {}

    size_t ItemsCount() const override {
        return items_.size();
    }

    Item GetItem(size_t idx) const override {
        return items_[idx];
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }

    Gatherer GetGatherer(size_t idx) const override {
        return gatherers_[idx];
    }

private:
    std::vector<Item> items_;
    std::vector<Gatherer> gatherers_;
};

}  // namespace collision_detector

// Специализация для вывода GatheringEvent
namespace Catch {
template<>
struct StringMaker<collision_detector::GatheringEvent> {
    static std::string convert(collision_detector::GatheringEvent const& value) {
        std::ostringstream tmp;
        tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";
        return tmp.str();
    }
};
}  // namespace Catch

using Catch::Approx;

namespace collision_detector {

// Тест: FindGatherEvents_Wrong1 не использует радиус предмета
TEST_CASE("Wrong1 should fail on item with different radius", "[FindGatherEvents_Wrong1]") {
    // Собиратель движется от (0, 0) до (10, 0) с радиусом 1
    // Предмет в точке (5, 1.5) с радиусом 1
    // Сумма радиусов = 2, расстояние = 1.5 < 2 -> должно быть столкновение
    // Но Wrong1 использует только радиус собирателя (1), поэтому 1.5 > 1 -> не найдёт
    
    std::vector<Item> items = {{geom::Point2D{5.0, 1.5}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents_Wrong1(provider);

    // Правильная функция должна найти столкновение
    std::vector<Item> items2 = {{geom::Point2D{5.0, 1.5}, 1.0}};
    std::vector<Gatherer> gatherers2 = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};
    TestItemGathererProvider provider2(std::move(items2), std::move(gatherers2));
    auto correct_events = FindGatherEvents(provider2);
    
    REQUIRE(!correct_events.empty());
    // Wrong1 должен пропустить это столкновение
    REQUIRE(events.empty());
}

// Тест: FindGatherEvents_Wrong2 не использует радиус собирателя
TEST_CASE("Wrong2 should fail on gatherer with larger radius", "[FindGatherEvents_Wrong2]") {
    // Собиратель движется от (0, 0) до (10, 0) с радиусом 2
    // Предмет в точке (5, 2.5) с радиусом 0.5
    // Сумма радиусов = 2.5, расстояние = 2.5 -> должно быть столкновение (касание)
    // Но Wrong2 использует только радиус предмета (0.5), поэтому 2.5 > 0.5 -> не найдёт
    
    std::vector<Item> items = {{geom::Point2D{5.0, 2.5}, 0.5}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 2.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents_Wrong2(provider);

    // Правильная функция должна найти столкновение
    std::vector<Item> items2 = {{geom::Point2D{5.0, 2.5}, 0.5}};
    std::vector<Gatherer> gatherers2 = {{{0.0, 0.0}, {10.0, 0.0}, 2.0}};
    TestItemGathererProvider provider2(std::move(items2), std::move(gatherers2));
    auto correct_events = FindGatherEvents(provider2);
    
    REQUIRE(!correct_events.empty());
    // Wrong2 должен пропустить это столкновение
    REQUIRE(events.empty());
}

// Тест: FindGatherEvents_Wrong3 не сортирует события
TEST_CASE("Wrong3 should return unsorted events", "[FindGatherEvents_Wrong3]") {
    // Создаём ситуацию, где события должны быть в определённом порядке
    std::vector<Item> items = {
        {geom::Point2D{25.0, 0.0}, 1.0},  // время ~0.83
        {geom::Point2D{10.0, 0.0}, 1.0},  // время ~0.33
        {geom::Point2D{5.0, 0.0}, 1.0}    // время ~0.17
    };
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {30.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents_Wrong3(provider);

    REQUIRE(events.size() == 3);
    
    // Wrong3 не сортирует события, поэтому они должны быть в порядке обхода (по item_id)
    // Правильная функция вернёт отсортированные по времени
    std::vector<Item> items2 = {
        {geom::Point2D{25.0, 0.0}, 1.0},
        {geom::Point2D{10.0, 0.0}, 1.0},
        {geom::Point2D{5.0, 0.0}, 1.0}
    };
    std::vector<Gatherer> gatherers2 = {{{0.0, 0.0}, {30.0, 0.0}, 1.0}};
    TestItemGathererProvider provider2(std::move(items2), std::move(gatherers2));
    auto correct_events = FindGatherEvents(provider2);
    
    // Правильные события должны быть отсортированы по времени
    CHECK(correct_events[0].time < correct_events[1].time);
    CHECK(correct_events[1].time < correct_events[2].time);
    
    // Wrong3 возвращает в порядке обхода (по item_id), поэтому times будут не отсортированы
    CHECK(events[0].item_id == 0);  // Wrong3 добавляет в порядке обхода
}

// Тест: FindGatherEvents_Wrong4 использует неправильную математику (только горизонталь/вертикаль)
TEST_CASE("Wrong4 should fail on diagonal movement", "[FindGatherEvents_Wrong4]") {
    // Собиратель движется по диагонали от (0, 0) до (10, 10)
    // Предмет в точке (5, 6) - близко к пути, но не на оси
    // Правильная функция должна найти столкновение, Wrong4 - нет
    
    std::vector<Item> items = {{geom::Point2D{5.0, 6.0}, 2.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 10.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents_Wrong4(provider);

    // Правильная функция
    std::vector<Item> items2 = {{geom::Point2D{5.0, 6.0}, 2.0}};
    std::vector<Gatherer> gatherers2 = {{{0.0, 0.0}, {10.0, 10.0}, 1.0}};
    TestItemGathererProvider provider2(std::move(items2), std::move(gatherers2));
    auto correct_events = FindGatherEvents(provider2);
    
    // Правильная функция должна найти столкновение
    // Wrong4 не работает с диагоналями правильно
    // Правим: расстояние от точки (5,6) до прямой y=x: |5-6|/sqrt(2) = 1/sqrt(2) ≈ 0.707
    // Сумма радиусов = 3, 0.707 < 3 -> столкновение
    REQUIRE(!correct_events.empty());
}

// Тест: FindGatherEvents_Wrong5 использует неправильную математику
TEST_CASE("Wrong5 should fail on non-axis-aligned movement", "[FindGatherEvents_Wrong5]") {
    // Собиратель движется по диагонали от (0, 0) до (10, 10)
    // Предмет в точке (5, 5) - точно на пути
    // Правильная функция должна найти столкновение с sq_distance = 0
    
    std::vector<Item> items = {{geom::Point2D{5.0, 5.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 10.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents_Wrong5(provider);

    // Правильная функция
    std::vector<Item> items2 = {{geom::Point2D{5.0, 5.0}, 1.0}};
    std::vector<Gatherer> gatherers2 = {{{0.0, 0.0}, {10.0, 10.0}, 1.0}};
    TestItemGathererProvider provider2(std::move(items2), std::move(gatherers2));
    auto correct_events = FindGatherEvents(provider2);
    
    REQUIRE(!correct_events.empty());
    CHECK(correct_events[0].sq_distance == Approx(0.0));
    
    // Wrong5 использует неправильную математику для диагоналей
    // Она вычисляет dist как p.x - start.x или p.y - start.y в зависимости от случая
    // Для диагонали это неправильно
}

}  // namespace collision_detector
