#define _USE_MATH_DEFINES

#include <catch2/catch_all.hpp>
#include <vector>
#include <cmath>
#include <sstream>
#include <algorithm>

#include "../src/model/collision_detector.h"
#include "../src/model/geom.h"

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

TEST_CASE("Single gatherer, single item - no collision (too far)", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 10.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

TEST_CASE("Gatherer doesn't move - no collisions", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{0.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {0.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

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

TEST_CASE("Item outside gatherer's path projection", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{0.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {{{5.0, 0.0}, {15.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

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

TEST_CASE("Empty provider - no items, no gatherers", "[FindGatherEvents]") {
    std::vector<Item> items;
    std::vector<Gatherer> gatherers;

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

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

TEST_CASE("Only gatherers, no items", "[FindGatherEvents]") {
    std::vector<Item> items;
    std::vector<Gatherer> gatherers = {
        {{0.0, 0.0}, {10.0, 0.0}, 1.0},
        {{0.0, 5.0}, {5.0, 5.0}, 2.0}
    };

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

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

    std::sort(events.begin(), events.end(),
        [](const auto& a, const auto& b) {
            if (a.gatherer_id != b.gatherer_id) return a.gatherer_id < b.gatherer_id;
            return a.item_id < b.item_id;
        });

    CHECK(events[0].gatherer_id == 0);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].sq_distance == Approx(0.0));
    CHECK(events[0].time == Approx(0.5));
    CHECK(events[1].gatherer_id == 1);
    CHECK(events[1].item_id == 1);
    CHECK(events[1].sq_distance == Approx(0.0));
    CHECK(events[1].time == Approx(0.5));
}

TEST_CASE("Item just outside collision zone", "[FindGatherEvents]") {
    std::vector<Item> items = {{geom::Point2D{5.0, 2.0001}, 1.0}};
    std::vector<Gatherer> gatherers = {{{0.0, 0.0}, {10.0, 0.0}, 1.0}};

    TestItemGathererProvider provider(std::move(items), std::move(gatherers));
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

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

}  // namespace collision_detector
