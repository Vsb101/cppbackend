#pragma once
#include <chrono>
#include <cmath>
#include <functional>

namespace loot_gen {

class LootGeneratorInfo {
public:
    LootGeneratorInfo(double period, double probability) 
        : period_(period), probability_(probability) {
    }

    double GetPeriod() const noexcept {
        return period_;
    }

    std::chrono::milliseconds GetPeriodInMilliseconds() const noexcept {
        auto rounded = std::round(period_*1000);
        return std::chrono::milliseconds{int(rounded)};
    }

    double GetProbability() const noexcept {
        return probability_;
    }

private:
    double period_;     // в секундах
    double probability_;
};

/*
 *  Генератор трофеев
 */
class LootGenerator {
public:
    using RandomGenerator = std::function<double()>;
    using TimeInterval = std::chrono::milliseconds;

    /*
     * base_interval - базовый отрезок времени > 0
     * probability - вероятность появления трофея в течение базового интервала времени
     * random_generator - генератор псевдослучайных чисел в диапазоне от [0 до 1]
     */
    LootGenerator(TimeInterval base_interval, double probability,
                  RandomGenerator random_gen = DefaultGenerator)
        : base_interval_{base_interval}
        , probability_{probability}
        , random_generator_{std::move(random_gen)} {
    }

    /*
     * Возвращает количество трофеев, которые должны появиться на карте спустя
     * заданный промежуток времени.
     * Количество трофеев, появляющихся на карте не превышает количество мародёров.
     *
     * time_delta - отрезок времени, прошедший с момента предыдущего вызова Generate
     * loot_count - количество трофеев на карте до вызова Generate
     * looter_count - количество мародёров на карте
     */
    unsigned Generate(TimeInterval time_delta, unsigned loot_count, unsigned looter_count);

private:
    static double DefaultGenerator() noexcept {
        return 1.0;
    };
    TimeInterval base_interval_;
    double probability_;
    TimeInterval time_without_loot_{};
    RandomGenerator random_generator_;
};

}  // namespace loot_gen
