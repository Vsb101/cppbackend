#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"
#include <catch2/catch.hpp>
#include <vector>
#include <cmath>
#include <sstream>

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

namespace collision_detector {

// Случай 1: Один собиратель, один предмет, столкновение происходит
TEST_CASE("Single gatherer, single item - collision occurs", "[FindGatherEvents]") {
    // Собиратель движется от (0, 0) до (10, 0)
    // Предмет находится на пути в точке (5, 0)
    // Радиусы: собиратель 1, предмет 1 -> сумма радиусов 2
    // Расстояние от предмета до прямой = 0, что < 2 -> столкновение

    std::vector<Item> items = {{geom::Point2D{5.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    REQUIRE(events[0].gatherer_id == 0);
    REQUIRE(events[0].item_id == 0);
    // Расстояние от предмета до отрезка = 0, sq_distance = 0
    CHECK(events[0].sq_distance == Approx(0.0));
    // Предмет находится посередине пути собирателя
    CHECK(events[0].time == Approx(0.5));
}

// Случай 2: Один собиратель, один предмет - нет столкновения (предмет слишком далеко)
TEST_CASE("Single gatherer, single item - no collision (too far)", "[FindGatherEvents]") {
    // Собиратель движется от (0, 0) до (10, 0)
    // Предмет находится в точке (5, 10) - слишком далеко
    // Радиусы: собиратель 1, предмет 1 -> сумма радиусов 2
    // Расстояние от предмета до прямой = 10, что > 2 -> нет столкновения

    std::vector<Item> items = {{geom::Point2D{5.0, 10.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

// Случай 3: Собиратель не двигается (start_pos == end_pos)
TEST_CASE("Gatherer doesn't move - no collisions", "[FindGatherEvents]") {
    // Собиратель стоит на месте
    // Предмет рядом, но так как движения нет, столкновений быть не должно

    std::vector<Item> items = {{geom::Point2D{0.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {0.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

// Случай 4: Несколько предметов, один собиратель
TEST_CASE("Single gatherer, multiple items", "[FindGatherEvents]") {
    // Собиратель движется по оси X от 0 до 20
    // Предмет 1: в точке (5, 0) - столкновение
    // Предмет 2: в точке (15, 0) - столкновение
    // Предмет 3: в точке (10, 5) - расстояние 5 > 2, нет столкновения

    std::vector<Item> items = {
        {geom::Point2D{5.0, 0.0}, 1.0},
        {geom::Point2D{15.0, 0.0}, 1.0},
        {geom::Point2D{10.0, 5.0}, 1.0}
    };
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {20.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 2);

    // События должны быть в хронологическом порядке
    CHECK(events[0].item_id == 0);
    CHECK(events[0].time == Approx(0.25));
    CHECK(events[0].sq_distance == Approx(0.0));

    CHECK(events[1].item_id == 1);
    CHECK(events[1].time == Approx(0.75));
    CHECK(events[1].sq_distance == Approx(0.0));
}

// Случай 5: Несколько собирателей, один предмет
TEST_CASE("Multiple gatherers, single item", "[FindGatherEvents]") {
    // Предмет в точке (5, 0)
    // Собиратель 0: движется от (0, 0) до (10, 0) - столкновение
    // Собиратель 1: движется от (0, 0) до (20, 0) - столкновение

    std::vector<Item> items = {{geom::Point2D{5.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {
        {{0.0, 0.0}, {10.0, 0.0}, 1.0},
        {{0.0, 0.0}, {20.0, 0.0}, 1.0}
    };

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 2);

    // Оба события должны присутствовать
    std::vector<size_t> gatherer_ids = {events[0].gatherer_id, events[1].gatherer_id};
    std::sort(gatherer_ids.begin(), gatherer_ids.end());
    CHECK(gatherer_ids[0] == 0);
    CHECK(gatherer_ids[1] == 1);
}

// Случай 6: Предмет на краю пути собирателя
TEST_CASE("Item at the edge of gatherer's path", "[FindGatherEvents]") {
    // Собиратель движется от (0, 0) до (10, 0)
    // Предмет в точке (0, 0) - на старте пути
    // Расстояние = 0, время = 0

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
    // Собиратель движется от (5, 0) до (15, 0)
    // Предмет в точке (0, 0) - вне отрезка перемещения
    // Проекция попадает за начало отрезка

    std::vector<Item> items = {{geom::Point2D{0.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{5.0, 0.0}, {15.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

// Случай 8: Касательное столкновение (расстояние = w + W)
TEST_CASE("Tangential collision (distance equals sum of radii)", "[FindGatherEvents]") {
    // Собиратель движется от (0, 0) до (10, 0)
    // Радиус собирателя = 1
    // Предмет в точке (5, 2) с радиусом 1
    // Сумма радиусов = 2, расстояние = 2 -> касательное столкновение

    std::vector<Item> items = {{geom::Point2D{5.0, 2.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK(events[0].item_id == 0);
    // Квадрат расстояния = 4
    CHECK(events[0].sq_distance == Approx(4.0));
    CHECK(events[0].time == Approx(0.5));
}

// Случай 9: Хронологический порядок событий
TEST_CASE("Events are in chronological order", "[FindGatherEvents]") {
    // Собиратель движется от (0, 0) до (30, 0)
    // Предмет 1: в точке (25, 0) - время 25/30 = 0.833...
    // Предмет 2: в точке (10, 0) - время 10/30 = 0.333...
    // Предмет 3: в точке (5, 0) - время 5/30 = 0.166...

    std::vector<Item> items = {
        {geom::Point2D{25.0, 0.0}, 1.0},
        {geom::Point2D{10.0, 0.0}, 1.0},
        {geom::Point2D{5.0, 0.0}, 1.0}
    };
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {30.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 3);

    // События должны быть отсортированы по времени
    CHECK(events[0].item_id == 2);
    CHECK(events[0].time == Approx(5.0 / 30.0));

    CHECK(events[1].item_id == 1);
    CHECK(events[1].time == Approx(10.0 / 30.0));

    CHECK(events[2].item_id == 0);
    CHECK(events[2].time == Approx(25.0 / 30.0));
}

// Случай 10: Движение по диагонали
TEST_CASE("Diagonal movement", "[FindGatherEvents]") {
    // Собиратель движется от (0, 0) до (10, 10)
    // Предмет в точке (5, 5) - на пути
    // Расстояние = 0, время = 0.5

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
    // Собиратель движется от (0, 0) до (10, 0)
    // Предмет в точке (5, 1) с радиусом 1
    // Расстояние до прямой = 1, сумма радиусов = 2 -> столкновение

    std::vector<Item> items = {{geom::Point2D{5.0, 1.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK(events[0].item_id == 0);
    // Квадрат расстояния = 1
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
    // Собиратель 0: движется от (0, 0) до (10, 0)
    // Собиратель 1: движется от (0, 5) до (10, 5)

    // Предмет 0: в точке (5, 0) - столкновение с собирателем 0
    // Предмет 1: в точке (5, 5) - столкновение с собирателем 1
    // Предмет 2: в точке (5, 2.5) - расстояние 2.5 > 2, нет столкновений

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

    // Проверяем, что события соответствуют ожидаемым
    std::vector<collision_detector::GatheringEvent> expected = {
        {0, 0, 0.0, 0.5},  // item 0, gatherer 0
        {1, 1, 0.0, 0.5}   // item 1, gatherer 1
    };

    // Сортируем по (gatherer_id, item_id) для сравнения
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
    // Собиратель движется от (0, 0) до (10, 0)
    // Радиусы: собиратель 1, предмет 1 -> сумма радиусов 2
    // Предмет в точке (5, 2.0001) - чуть дальше зоны столкновения

    std::vector<Item> items = {{geom::Point2D{5.0, 2.0001}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

// Случай 17: Вертикальное движение
TEST_CASE("Vertical movement", "[FindGatherEvents]") {
    // Собиратель движется от (0, 0) до (0, 10)
    // Предмет в точке (0, 5) - на пути

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
    // Собиратель движется от (10, 0) до (0, 0)
    // Предмет в точке (5, 0) - на пути

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

}  // namespace collision_detector