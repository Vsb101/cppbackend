#include <cmath>
#include <catch2/catch_test_macros.hpp>

#include "../src/model/loot_generator.h"

using namespace std::literals;

SCENARIO("Loot generation") {
    using loot_gen::LootGenerator;
    using TimeInterval = LootGenerator::TimeInterval;

    GIVEN("a loot generator") {
        LootGenerator gen{1s, 1.0};

        constexpr TimeInterval TIME_INTERVAL = 1s;

        WHEN("loot count is enough for every looter") {
            THEN("no loot is generated") {
                for (unsigned looters = 0; looters < 10; ++looters) {
                    for (unsigned loot = looters; loot < looters + 10; ++loot) {
                        INFO("loot count: " << loot << ", looters: " << looters);
                        REQUIRE(gen.Generate(TIME_INTERVAL, loot, looters) == 0);
                    }
                }
            }
        }

        WHEN("number of looters exceeds loot count") {
            THEN("number of loot is proportional to loot difference") {
                for (unsigned loot = 0; loot < 10; ++loot) {
                    for (unsigned looters = loot; looters < loot + 10; ++looters) {
                        INFO("loot count: " << loot << ", looters: " << looters);
                        REQUIRE(gen.Generate(TIME_INTERVAL, loot, looters) == looters - loot);
                    }
                }
            }
        }
    }

    GIVEN("a loot generator with some probability") {
        constexpr TimeInterval BASE_INTERVAL = 1s;
        LootGenerator gen{BASE_INTERVAL, 0.5};

        WHEN("time is greater than base interval") {
            THEN("number of generated loot is increased") {
                // При t=2s, p = 1 - (1-0.5)^2 = 0.75
                // Случайное число <= 0.75 с высокой вероятностью, генерируются все 4 предмета
                CHECK(gen.Generate(BASE_INTERVAL * 2, 0, 4) == 4);
            }
        }

        WHEN("time is less than base interval") {
            THEN("number of generated loot is decreased") {
                // Создаём генератор с фиксированным random=0.1, который гарантированно <= p
                LootGenerator gen_det{BASE_INTERVAL, 0.5, [] { return 0.1; }};
                // 415ms даёт p ≈ 0.25, 0.1 < 0.25, генерируется 4 предмета
                CHECK(gen_det.Generate(std::chrono::milliseconds(415), 0, 4) == 4);
            }
        }
    }

    GIVEN("a loot generator with custom random generator") {
        LootGenerator gen{1s, 0.5, [] {
                              return 0.5;
                          }};
        WHEN("loot is generated") {
            THEN("number of loot is proportional to random generated values") {
                // random_generator_() возвращает 0.5
                // При 415ms вероятность ≈ 0.25, 0.5 > 0.25, поэтому ничего не генерируется
                // time_without_loot_ накапливается
                CHECK(gen.Generate(std::chrono::milliseconds(415), 0, 4) == 0);
                
                // После первого вызова time_without_loot_ = 415ms
                // Добавляем ещё 585ms, чтобы получить 1000ms = 1s
                // При t=1s, p = 1 - (1-0.5)^(1/1) = 0.5
                // random_generator_() = 0.5 <= 0.5, поэтому генерируются ВСЕ предметы: 4 - 0 = 4
                CHECK(gen.Generate(std::chrono::milliseconds(585), 0, 4) == 4);
            }
        }
    }
}
