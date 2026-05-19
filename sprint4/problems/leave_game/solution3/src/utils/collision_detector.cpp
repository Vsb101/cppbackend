#include "collision_detector.h"
#include <cassert>
#include <variant>

namespace collision_detector {

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
    // Проверим, что перемещение ненулевое.
    // Тут приходится использовать строгое равенство, а не приближённое,
    // пскольку при сборе заказов придётся учитывать перемещение даже на небольшое
    // расстояние.
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return CollectionResult(sq_distance, proj_ratio);
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> detected_events;

    static auto eq_pt = [](geom::Point2D p1, geom::Point2D p2) {
        return p1.x == p2.x && p1.y == p2.y;
    };

    for (size_t g = 0; g < provider.GatherersCount(); ++g) {
        Gatherer gatherer = provider.GetGatherer(g);
        if (eq_pt(gatherer.start_pos, gatherer.end_pos)) {
            continue;
        }
        for (size_t i = 0; i < provider.ItemsCount(); ++i) {
            Item item = provider.GetItem(i);
            auto collect_result
                = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);

            if (collect_result.IsCollected(gatherer.width + item.width)) {
                GatheringEvent evt{.item_id = i,
                                   .gatherer_id = g,
                                   .sq_distance = collect_result.sq_distance,
                                   .time = collect_result.proj_ratio};
                detected_events.push_back(evt);
            }
        }
    }

    std::sort(detected_events.begin(), detected_events.end(),
              [](const GatheringEvent& e_l, const GatheringEvent& e_r) {
                  return e_l.time < e_r.time;
              });

    return detected_events;
}

std::optional<LineSegment> Intersect(LineSegment s1, LineSegment s2) {
    double left = std::max(s1.x1, s2.x1);
    double right = std::min(s1.x2, s2.x2);

    if (right < left) {
        return std::nullopt;
    }

    // Здесь использована возможность C++-20 - объявленные 
    // инициализаторы (designated initializers).
    // Узнать о ней подробнее можно на сайте cppreference:
    // https://en.cppreference.com/w/cpp/language/aggregate_initialization#Designated_initializers
    return LineSegment{.x1 = left, .x2 = right};
}

// Вычисляем проекции на оси
LineSegment ProjectX(Rect r) {
    return LineSegment{.x1 = r.x, .x2 = r.x + r.w};
}

LineSegment ProjectY(Rect r) {
    return LineSegment{.x1 = r.y, .x2 = r.y + r.h};
}

std::optional<Rect> Intersect(Rect r1, Rect r2) {
    auto px = Intersect(ProjectX(r1), ProjectX(r2));
    auto py = Intersect(ProjectY(r1), ProjectY(r2));

    if (!px || !py) {
        return std::nullopt;
    }

    // Составляем из проекций прямоугольник
    return Rect(px->x1, py->x1, 
                px->x2 - px->x1, py->x2 - py->x1);
}

std::vector<OfficeSaveEvent> FindOfficeSaveEvents(const OfficeSaveProvider& provider) {
    std::vector<OfficeSaveEvent> detected_events;

    static auto eq_pt = [](geom::Point2D p1, geom::Point2D p2) {
        return p1.x == p2.x && p1.y == p2.y;
    };

    // По всем "собирателям"
    for (size_t g = 0; g < provider.GatherersCount(); ++g) {
        Gatherer gatherer = provider.GetGatherer(g);
        // Пропускаем ситуацию, когда позиция не поменялась
        if (eq_pt(gatherer.start_pos, gatherer.end_pos)) {
            continue;
        }

        // По всем "базам" (они же офисы в виде прямоугольников)
        for (size_t i = 0; i < provider.RectsCount(); ++i) {
            // Прямоугольник, который соответсвует офису
            Rect office = provider.GetRect(i);
            // Прямогульник, который "накрывает" пройденный собирателем путь
            Rect gatherer_path(
                geom::Point2D(gatherer.start_pos.x, gatherer.start_pos.y), 
                geom::Point2D(gatherer.end_pos.x, gatherer.end_pos.y), 
                gatherer.width);

            // Находим пересечение прямоугольников
            auto intersect_result = Intersect(gatherer_path, office);

            if (!intersect_result) {    // Если пересечения нет, то и контакта нет
                continue;
            }

            // Если пересечение есть, для определения минимального времени в пути до офиса
            // ищем минимальный путь до вершины прямоугольника-пересечения
            double min_ratio = 1.01;
            for ( auto vertex : intersect_result->GetVertices()) {
                auto collect_result = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, vertex);

                if (collect_result.IsCollected(gatherer.width) && collect_result.proj_ratio < min_ratio) {
                    min_ratio = collect_result.proj_ratio;
                }
            }
            OfficeSaveEvent evt{.office_id = i,
                            .gatherer_id = g,
                            .time = min_ratio};
            detected_events.push_back(evt);

        }
    }

    std::sort(detected_events.begin(), detected_events.end(),
              [](const OfficeSaveEvent& e_l, const OfficeSaveEvent& e_r) {
                  return e_l.time < e_r.time;
              });

    return detected_events;
}

bool operator<(const AllIvents& a, const AllIvents& b) {
    double a_time, b_time;

    std::visit([&](auto&& arg) {
        a_time = arg.time;
    }, a);

    std::visit([&](auto&& arg) {
        b_time = arg.time;
    }, b);

    return a_time < b_time;
}

}  // namespace collision_detector