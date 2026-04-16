#pragma once

#include <random>
#include <sstream>
#include <iomanip>
#include <string>

namespace app {

/**
 * @brief Генератор и хранитель токенов игроков
 */
class PlayerTokens {
public:
    PlayerTokens()
        : gen_(std::random_device{}()) {
    }

    /**
     * @brief Генерирует новый уникальный токен
     * @return Строка-токен
     */
    std::string GenerateToken() {
        std::uniform_int_distribution<> distrib(0, 0xFFFFFFFF);
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(8) << distrib(gen_)
           << std::hex << std::setfill('0') << std::setw(8) << distrib(gen_);
        return ss.str();
    }

private:
    std::mt19937 gen_;
};

}  // namespace app
