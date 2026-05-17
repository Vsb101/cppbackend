#pragma once
#include <chrono>
#include <functional>
#include <random>

namespace loot_gen {

// Глобальный RNG для использования в конструкторах по умолчанию
extern std::random_device s_random_device;
extern std::mt19937 s_rng;
extern std::uniform_real_distribution<double> s_distribution;

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
                  RandomGenerator random_gen)
        : base_interval_{base_interval}
        , probability_{probability}
        , random_generator_{std::move(random_gen)} {
    }

    LootGenerator(TimeInterval base_interval, double probability)
        : base_interval_{base_interval}
        , probability_{probability}
        , random_generator_{std::function<double()>{[]() { return s_distribution(s_rng); }}} {
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
    TimeInterval base_interval_;
    double probability_;
    TimeInterval time_without_loot_{};
    RandomGenerator random_generator_;
};

}  // namespace loot_gen
