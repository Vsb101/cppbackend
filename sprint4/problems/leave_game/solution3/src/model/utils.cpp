#include "utils.h"
#include <random>

namespace utils {

namespace my_random {
    // generate random double number in [min, max] range
    double GetRandomDouble(double min, double max) {
        std::random_device rd; // used to generate a seed
        std::mt19937 generator(rd());
        double nextafter_max = std::nextafter(max, std::numeric_limits<decltype(max)>::max());
        std::uniform_real_distribution<> distr(min, nextafter_max);
        return  distr(generator);
    }

    // generate random size_t number in [min, max] range
    size_t GetRandomIndex(size_t min, size_t max) {
        std::random_device rd; // used to generate a seed
        std::mt19937 generator(rd());
        double nextafter_max = std::nextafter(max, std::numeric_limits<decltype(max)>::max());
        std::uniform_int_distribution<unsigned long> distr(min, nextafter_max);
        return  distr(generator);
    }
}

namespace validators {
    bool IsValidToken(const std::string_view& token) {
        return token.size() == 32;
    }
}

}
