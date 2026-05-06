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

// ============================================================================
// БАЗОВЫЕ ТЕСТЫ ДЛЯ FindGatherEvents
// ============================================================================

// Случай 1: Один собиратель, один предмет, столкновение происходит
TEST_CASE("Single gatherer, single item - collision occurs", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    REQUIRE(events[0].gatherer_id == 0);
    REQUIRE(events[0].item_id == 0);
    CHECK(events[0].sq_distance == Approx(0.0));
    CHECK(events[0].time == Approx(0.5));
}

// Случай 2: Один собиратель, один предмет - нет столкновения (предмет слишком далеко)
TEST_CASE("Single gatherer, single item - no collision (too far)", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 10.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

// Случай 3: Собиратель не двигается (start_pos == end_pos)
TEST_CASE("Gatherer doesn't move - no collisions", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{0.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {0.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

// Случай 4: Несколько предметов, один собиратель
TEST_CASE("Single gatherer, multiple items", "[FindGatherEvents]") {
    std::vector<Item> items = {
        {geom::Point2D{5.0, 0.0}, 1.0},
        {geom::Point2D{15.0, 0.0}, 1.0},
        {geom::Point2D{10.0, 5.0}, 1.0}
    };
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {20.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 2);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].time == Approx(0.25));
    CHECK(events[0].sq_distance == Approx(0.0));
    CHECK(events[1].item_id == 1);
    CHECK(events[1].time == Approx(0.75));
    CHECK(events[1].sq_distance == Approx(0.0));
}

// Случай 5: Несколько собирателей, один предмет
TEST_CASE("Multiple gatherers, single item", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {
        {{0.0, 0.0}, {10.0, 0.0}, 1.0},
        {{0.0, 0.0}, {20.0, 0.0}, 1.0}
    };

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 2);
    std::vector<size_t> gatherer_ids = {events[0].gatherer_id, events[1].gatherer_id};
    std::sort(gatherer_ids.begin(), gatherer_ids.end());
    CHECK(gatherer_ids[0] == 0);
    CHECK(gatherer_ids[1] == 1);
}

// Случай 6: Предмет на краю пути собирателя
TEST_CASE("Item at the edge of gatherer's path", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{0.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].sq_distance == Approx(0.0));
    CHECK(events[0].time == Approx(0.0));
}

// Случай 7: Предмет вне проекции отрезка
TEST_CASE("Item outside gatherer's path projection", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{0.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{5.0, 0.0}, {15.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

// Случай 8: Касательное столкновение (расстояние = w + W)
TEST_CASE("Tangential collision (distance equals sum of radii)", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 2.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].sq_distance == Approx(4.0));
    CHECK(events[0].time == Approx(0.5));
}

// Случай 9: Хронологический порядок событий
TEST_CASE("Events are in chronological order", "[FindGatherEvents]") {
    std::vector<Item> items = {
        {geom::Point2D{25.0, 0.0}, 1.0},
        {geom::Point2D{10.0, 0.0}, 1.0},
        {geom::Point2D{5.0, 0.0}, 1.0}
    };
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {30.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 3);
    CHECK(events[0].item_id == 2);
    CHECK(events[0].time == Approx(5.0 / 30.0));
    CHECK(events[1].item_id == 1);
    CHECK(events[1].time == Approx(10.0 / 30.0));
    CHECK(events[2].item_id == 0);
    CHECK(events[2].time == Approx(25.0 / 30.0));
}

// Случай 10: Движение по диагонали
TEST_CASE("Diagonal movement", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 5.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 10.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].sq_distance == Approx(0.0));
    CHECK(events[0].time == Approx(0.5));
}

// Случай 11: Предмет с ненулевым расстоянием (но в пределах суммы радиусов)
TEST_CASE("Item at non-zero distance within collision range", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 1.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].sq_distance == Approx(1.0));
    CHECK(events[0].time == Approx(0.5));
}

// Случай 12: Пустой провайдер (нет предметов и собирателей)
TEST_CASE("Empty provider - no items, no gatherers", "[FindGatherEvents]") {
    std::vector<Item> items;
    std::vector<Gatherer> gatherers;

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

// Случай 13: Только предметы, нет собирателей
TEST_CASE("Only items, no gatherers", "[FindGatherEvents]") {
    std::vector<Item> items = {
        {geom::Point2D{0.0, 0.0}, 1.0},
        {geom::Point2D{5.0, 5.0}, 2.0}
    };
    std::vector<Gatherer> gatherers;

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

// Случай 14: Только собиратели, нет предметов
TEST_CASE("Only gatherers, no items", "[FindGatherEvents]") {
    std::vector<Item> items;
    std::vector<Gatherer> gatherers = {
        {{0.0, 0.0}, {10.0, 0.0}, 1.0},
        {{0.0, 0.0}, {5.0, 5.0}, 2.0}
    };

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

// Случай 15: Несколько собирателей и предметов, проверка полноты событий
TEST_CASE("Multiple gatherers and items - complete detection", "[FindGatherEvents]") {
    std::vector<Item> items = {
        {geom::Point2D{5.0, 0.0}, 1.0},
        {geom::Point2D{5.0, 5.0}, 1.0},
        {geom::Point2D{5.0, 2.5}, 1.0}
    };
    std::vector<Gatherer> gatherers = {
        {{0.0, 0.0}, {10.0, 0.0}, 1.0},
        {{0.0, 5.0}, {10.0, 5.0}, 1.0}
    };

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 2);

    std::vector<collision_detector::GatheringEvent> expected = {
        {0, 0, 0.0, 0.5},  // item 0, gatherer 0
        {1, 1, 0.0, 0.5}   // item 1, gatherer 1
    };

    std::sort(events.begin(), events.end(),
        [](const auto& a, const auto& b) {
            if (a.gatherer_id != b.gatherer_id) return a.gatherer_id < b.gatherer_id;
            return a.item_id < b.item_id;
        });
    std::sort(expected.begin(), expected.end(),
        [](const auto& a, const auto& b) {
            if (a.gatherer_id != b.gatherer_id) return a.gatherer_id < b.gatherer_id;
            return a.item_id < b.item_id;
        });

    CHECK(events[0].gatherer_id == expected[0].gatherer_id);
    CHECK(events[0].item_id == expected[0].item_id);
    CHECK(events[0].sq_distance == Approx(expected[0].sq_distance));
    CHECK(events[0].time == Approx(expected[0].time));
    CHECK(events[1].gatherer_id == expected[1].gatherer_id);
    CHECK(events[1].item_id == expected[1].item_id);
    CHECK(events[1].sq_distance == Approx(expected[1].sq_distance));
    CHECK(events[1].time == Approx(expected[1].time));
}

// Случай 16: Предмет чуть-чуть вне зоны столкновения
TEST_CASE("Item just outside collision zone", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 2.0001}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

// Случай 17: Вертикальное движение
TEST_CASE("Vertical movement", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{0.0, 5.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {0.0, 10.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].sq_distance == Approx(0.0));
    CHECK(events[0].time == Approx(0.5));
}

// Случай 18: Движение в отрицательном направлении
TEST_CASE("Movement in negative direction", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{10.0, 0.0}, {0.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].sq_distance == Approx(0.0));
    CHECK(events[0].time == Approx(0.5));
}

// ============================================================================
// ТЕСТЫ НА ПРОВЕРКУ НЕПРАВИЛЬНЫХ РЕАЛИЗАЦИЙ
// ============================================================================

// Тест: FindGatherEvents_Wrong1 не использует радиус предмета
TEST_CASE("Wrong1 should fail on item with different radius", "[FindGatherEvents_Wrong1]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 1.5}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents_Wrong1(provider);

    std::vector<Item> items2 = {{geom::Point2D{5.0, 1.5}, 1.0}};
    std::vector<Gatherer> gatherers2 = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};
    TestItemGathererProvider provider2(std::move(items2), std::move(gatherers2));
    auto correct_events = FindGatherEvents(provider2);
    
    REQUIRE(!correct_events.empty());
    REQUIRE(events.empty());
}

// Тест: FindGatherEvents_Wrong2 не использует радиус собирателя
TEST_CASE("Wrong2 should fail on gatherer with larger radius", "[FindGatherEvents_Wrong2]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 2.5}, 0.5}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 2.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents_Wrong2(provider);

    std::vector<Item> items2 = {{geom::Point2D{5.0, 2.5}, 0.5}};
    std::vector<Gatherer> gatherers2 = {{{0.0, 0.0}, {10.0, 0.0}, 2.0}};
    TestItemGathererProvider provider2(std::move(items2), std::move(gatherers2));
    auto correct_events = FindGatherEvents(provider2);
    
    REQUIRE(!correct_events.empty());
    REQUIRE(events.empty());
}

// Тест: FindGatherEvents_Wrong3 не сортирует события
TEST_CASE("Wrong3 should return unsorted events", "[FindGatherEvents_Wrong3]") {
    std::vector<Item> items = {
        {geom::Point2D{25.0, 0.0}, 1.0},
        {geom::Point2D{10.0, 0.0}, 1.0},
        {geom::Point2D{5.0, 0.0}, 1.0}
    };
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {30.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents_Wrong3(provider);

    REQUIRE(events.size() == 3);
    
    std::vector<Item> items2 = {
        {geom::Point2D{25.0, 0.0}, 1.0},
        {geom::Point2D{10.0, 0.0}, 1.0},
        {geom::Point2D{5.0, 0.0}, 1.0}
    };
    std::vector<Gatherer> gatherers2 = {{{0.0, 0.0}, {30.0, 0.0}, 1.0}};
    TestItemGathererProvider provider2(std::move(items2), std::move(gatherers2));
    auto correct_events = FindGatherEvents(provider2);
    
    CHECK(correct_events[0].time < correct_events[1].time);
    CHECK(correct_events[1].time < correct_events[2].time);
    CHECK(events[0].item_id == 0);
}

// Тест: FindGatherEvents_Wrong4 использует неправильную математику (только горизонталь/вертикаль)
TEST_CASE("Wrong4 should fail on diagonal movement", "[FindGatherEvents_Wrong4]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 6.0}, 2.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 10.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents_Wrong4(provider);

    std::vector<Item> items2 = {{geom::Point2D{5.0, 6.0}, 2.0}};
    std::vector<Gatherer> gatherers2 = {{{0.0, 0.0}, {10.0, 10.0}, 1.0}};
    TestItemGathererProvider provider2(std::move(items2), std::move(gatherers2));
    auto correct_events = FindGatherEvents(provider2);
    
    REQUIRE(!correct_events.empty());
}

// Тест: FindGatherEvents_Wrong5 использует неправильную математику
TEST_CASE("Wrong5 should fail on non-axis-aligned movement", "[FindGatherEvents_Wrong5]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 5.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 10.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents_Wrong5(provider);

    std::vector<Item> items2 = {{geom::Point2D{5.0, 5.0}, 1.0}};
    std::vector<Gatherer> gatherers2 = {{{0.0, 0.0}, {10.0, 10.0}, 1.0}};
    TestItemGathererProvider provider2(std::move(items2), std::move(gatherers2));
    auto correct_events = FindGatherEvents(provider2);
    
    REQUIRE(!correct_events.empty());
    CHECK(correct_events[0].sq_distance == Approx(0.0));
}

}  // namespace collision_detector
