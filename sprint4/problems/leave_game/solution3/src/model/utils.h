#pragma once
#include <iomanip>
#include <iostream>
#include <optional>
#include <utility>

namespace utils {

using namespace std;

class FormattedOutput
{
    private:
        int width;
        char fill;
        stringstream& stream_obj;

    public:
        FormattedOutput(stringstream& obj, int w, char c = '0'): width(w), fill(c), stream_obj(obj) {}

        template<typename T>
        FormattedOutput& operator<<(const T& output)
        {
            stream_obj << hex << setfill(fill) << setw(width) << output;
            return *this;
        }

        FormattedOutput& operator<<(stringstream& (*func)(stringstream&))
        {
            func(stream_obj);
            return *this;
        }

        std::string str()
        {
            return stream_obj.str();
        }
};

namespace my_random {
    double GetRandomDouble(double min, double max);
    size_t GetRandomIndex(size_t min, size_t max);
}

namespace validators {
    bool IsValidToken(const std::string_view& token);
}

namespace geometry {
    template<typename T>
    std::optional<std::pair<T,T>> GetIntersection(std::pair<T,T> a, std::pair<T,T> b) {
        bool reverse_move = false;
        if(a.first > a.second) {
            std::swap(a.first, a.second);
            reverse_move = true;
        }

        if ( a.first > b.second || a.second < b.first ) {
            return std::nullopt;
        }

        std::pair<T,T> res{max(a.first, b.first), min(a.second, b.second)};

        if (reverse_move) {
            std::swap(res.first, res.second);
        }
        return res;
    }

    template<typename T>
    bool IsInInterval(T point, std::pair<T,T> interval) {
        return point >= interval.first && point <= interval.second;
    }

    template<typename T, typename U>
    std::optional<T> GetMaxMoveOnSegment(T move_start, T move_end, U road_beg, U road_end) {
        // Стартуем вне дороги - такого быть не может
        if (move_start < road_beg || move_start > road_end) {
            return std::nullopt;
        }

        auto res_beg = std::max(move_start, road_beg);
        auto res_end = std::min(move_end, road_end);

        // Если полученный отрезок существует, возвращаем его
        if (res_beg <= res_end) {
            return res_end;
        }

        return std::nullopt;
    }
}

}
