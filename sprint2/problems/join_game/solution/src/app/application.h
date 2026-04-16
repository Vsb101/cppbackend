#pragma once

#include "app/player.h"
#include "app/player_tokens.h"
#include "model/model.h"
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace app {

/**
 * @brief Фасад приложения - единая точка доступа к сценариям использования
 * 
 * Скрывает сложность системы за простым интерфейсом.
 * Уменьшает связность между http_handler и model.
 */
class Application {
public:
    explicit Application(model::Game& game);

    // ========================================================================
    // Сценарий: Вход в игру (JoinGame)
    // ========================================================================
    struct JoinGameResult {
        bool success;
        Player::Id player_id;
        Player::Token auth_token;
        std::string error_code;
        std::string message;
    };

    /**
     * @brief Вход игрока в игру
     * @param user_name Имя пользователя (пса)
     * @param map_id Идентификатор карты
     * @return Результат операции
     */
    JoinGameResult JoinGame(std::string user_name, std::string map_id);

    // ========================================================================
    // Сценарий: Получение списка игроков
    // ========================================================================

    /**
     * @brief Получить список игроков
     * @param token Токен игрока
     * @return Список игроков или nullptr если токен невалиден
     */
    std::vector<Player> GetPlayers(const Player::Token& token) const;

    /**
     * @brief Найти игрока по токену
     * @param token Токен игрока
     * @return Указатель на игрока или nullptr
     */
    const Player* FindPlayerByToken(const Player::Token& token) noexcept;

    // ========================================================================
    // Доступ к доменной модели (для чтения)
    // ========================================================================

    const model::Game& GetGame() const noexcept { return game_; }
    model::Game& GetGame() noexcept { return game_; }

    const model::Dog* GetDog(size_t index) const noexcept {
        return game_.GetDog(index);
    }

private:
    model::Game& game_;
    PlayerTokens tokens_;
    std::vector<Player> players_;
};

}  // namespace app
