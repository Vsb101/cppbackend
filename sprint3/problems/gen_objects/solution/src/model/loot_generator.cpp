#include "loot_generator.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace loot_gen {

std::random_device s_random_device;
std::mt19937 s_rng(s_random_device());
std::uniform_real_distribution<double> s_distribution(0.0, 1.0);

unsigned LootGenerator::Generate(TimeInterval time_delta, unsigned loot_count, unsigned looter_count) {
    time_without_loot_ += time_delta;

    // Если предметов уже столько же, сколько собак, или больше — ничего не делаем
    if (loot_count >= looter_count || looter_count == 0) {
        return 0;
    }

    // Формула вероятности: p = 1 - (1 - P)^(t/T)
    const double ratio = std::chrono::duration<double>{time_without_loot_} / base_interval_;
    const double p = 1.0 - std::pow(1.0 - probability_, ratio);

    // Сбрасываем время каждый раз, когда проводим "розыгрыш"
    if (random_generator_() <= p) {
        unsigned generated_loot = looter_count - loot_count;
        time_without_loot_ = {}; 
        return generated_loot;
    }

    return 0;
}

} // namespace loot_gen
