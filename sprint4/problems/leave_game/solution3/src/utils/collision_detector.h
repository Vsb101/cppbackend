#pragma once

#include "geom.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <optional>
#include <variant>

namespace collision_detector {

struct CollectionResult {
    bool IsCollected(double collect_radius) const {
        return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
    }

    // Квадрат расстояния до точки
    double sq_distance;
    // Доля пройденного отрезка
    double proj_ratio;
};

// Движемся из точки a в точку b и пытаемся подобрать точку c
CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c);

struct Item {
    unsigned int id;
    geom::Point2D position;
    double width;
};

struct Gatherer {
    geom::Point2D start_pos;
    geom::Point2D end_pos;
    double width;
};

class ItemGathererProvider {
protected:
    ~ItemGathererProvider() = default;

public:
    virtual size_t ItemsCount() const = 0;
    virtual Item GetItem(size_t idx) const = 0;
    virtual size_t GatherersCount() const = 0;
    virtual Gatherer GetGatherer(size_t idx) const = 0;
};

struct GatheringEvent {
    size_t item_id;
    size_t gatherer_id;
    double sq_distance;
    double time;
};

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider);

class VectorItemGathererProvider : public ItemGathererProvider {
public:
    VectorItemGathererProvider(std::vector<Item> items,
                               std::vector<Gatherer> gatherers)
        : items_(items)
        , gatherers_(gatherers) {
    }

    
    size_t ItemsCount() const override {
        return items_.size();
    }
    Item GetItem(size_t idx) const override {
        return items_.at(idx);
    }
    size_t GatherersCount() const override {
        return gatherers_.size();
    }
    Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx);
    }

private:
    std::vector<Item> items_;
    std::vector<Gatherer> gatherers_;
};

class CompareEvents {
public:
    bool operator()(const GatheringEvent& l,
                    const GatheringEvent& r) {
        if (l.gatherer_id != r.gatherer_id || l.item_id != r.item_id) 
            return false;

        static const double eps = std::numeric_limits<double>::epsilon();

        if (std::abs(l.sq_distance - r.sq_distance) > eps) {
            return false;
        }

        if (std::abs(l.time - r.time) > eps) {
            return false;
        }
        return true;
    }
};

struct LineSegment {
    // Предполагаем, что x1 <= x2
    double x1, x2;
};

struct Rect {
    double x, y;
    double w, h;

    Rect() = delete;
    Rect(double x, double y, double w, double h) : x(x), y(y), w(w), h(h) {};

    Rect(geom::Point2D start, geom::Point2D end, double width) {
        x = std::min(start.x, end.x) - width;
        y = std::min(start.y, end.y) - width;
        w = std::fabs(end.x - start.x) + width*2;
        h = std::fabs(end.y - start.y) + width*2;
    }

    std::vector<geom::Point2D> GetVertices() const {
        std::vector<geom::Point2D> res;
        res.push_back({x, y});
        res.push_back({x + w, y});
        res.push_back({x + w, y + h});
        res.push_back({x, y + h});
        return res;
    }
};

std::optional<LineSegment> Intersect(LineSegment s1, LineSegment s2);
LineSegment ProjectX(Rect r);
LineSegment ProjectY(Rect r);
std::optional<Rect> Intersect(Rect r1, Rect r2);

class OfficeSaveProvider {
protected:
    ~OfficeSaveProvider() = default;

public:
    virtual size_t RectsCount() const = 0;
    virtual Rect GetRect(size_t idx) const = 0;
    virtual size_t GatherersCount() const = 0;
    virtual Gatherer GetGatherer(size_t idx) const = 0;
};

struct OfficeSaveEvent {
    size_t office_id;
    size_t gatherer_id;
    double sq_distance;
    double time;
};

std::vector<OfficeSaveEvent> FindOfficeSaveEvents(const OfficeSaveProvider& provider) ;

class VectorOfficeSaveProvider : public OfficeSaveProvider {
public:
    VectorOfficeSaveProvider(std::vector<Rect> rects,
                               std::vector<Gatherer> gatherers)
        : rects_(rects)
        , gatherers_(gatherers) {
    }

    
    size_t RectsCount() const override {
        return rects_.size();
    }
    Rect GetRect(size_t idx) const override {
        return rects_.at(idx);
    }
    size_t GatherersCount() const override {
        return gatherers_.size();
    }
    Gatherer GetGatherer(size_t idx) const override {
        return gatherers_[idx];
    }

private:
    std::vector<Rect> rects_;
    std::vector<Gatherer> gatherers_;
};

using AllIvents = std::variant<OfficeSaveEvent, GatheringEvent>;

bool operator<(const AllIvents& a, const AllIvents& b);

}  // namespace collision_detector