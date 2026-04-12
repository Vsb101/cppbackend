#pragma once

#include "model.h"
#include "tagged.h"

#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace game_model {

/// Тип для идентификатора игрока
using PlayerId = std::uint32_t;

/// Тег для токена аутентификации
struct AuthTokenTag {};

/// Токен аутентификации - строка из 32 hex-символов
using AuthToken = util::Tagged<std::string, AuthTokenTag>;

/// Хешер для токенов
struct AuthTokenHasher {
    size_t operator()(const AuthToken& token) const {
        return std::hash<std::string>{}(*token);
    }
};

/// Игрок - агент пользователя внутри игры
struct Player {
    PlayerId id;                    ///< Уникальный идентификатор игрока
    std::string name;               ///< Имя игрока (кличка пса)
    AuthToken token;                ///< Токен для аутентификации
    const model::Map* map;          ///< Указатель на карту, на которой находится игрок
};

/// Класс для управления токенами игроков
class PlayerTokens {
public:
    PlayerTokens();

    /// Генерирует новый токен и связывает его с игроком
    AuthToken GenerateToken();

    /// Находит игрока по токену
    const Player* FindPlayerByToken(const AuthToken& token) const;

    /// Добавляет связь токена с игроком
    void AddToken(const AuthToken& token, const Player* player);

private:
    std::random_device random_device_;
    std::mt19937_64 generator1_;
    std::mt19937_64 generator2_;

    /// Мапа токен -> игрок
    std::unordered_map<AuthToken, const Player*, AuthTokenHasher> token_to_player_;
};

/// Игровой сеанс - набор игроков на одной карте
class GameSession {
public:
    explicit GameSession(const model::Map* map);

    /// @return Карта, на которой происходит сеанс
    const model::Map* GetMap() const noexcept { return map_; }

    /// Добавляет игрока в сеанс
    void AddPlayer(Player player);

    /// @return Список всех игроков в сеансе
    const std::vector<Player>& GetPlayers() const noexcept { return players_; }

    /// @return Игрок по ID (или nullptr если не найден)
    const Player* FindPlayer(PlayerId id) const;

    /// @return Последний добавленный игрок
    Player& Back() { return players_.back(); }

private:
    const model::Map* map_;
    std::vector<Player> players_;
};

/// Менеджер игровых сеансов
class GameSessions {
public:
    /// Добавляет игровой сеанс для карты
    void AddSession(const model::Map* map);

    /// Находит или создаёт сеанс для карты
    GameSession* GetOrCreateSession(const model::Map* map);

    /// Находит сеанс по карте
    GameSession* FindSession(const model::Map* map);

    /// Находит игрока по токену
    const Player* FindPlayerByToken(const AuthToken& token) const;

    /// Добавляет игрока в сеанс
    void AddPlayerToSession(const model::Map* map, Player player);

    /// @return Генератор токенов
    PlayerTokens& GetTokenGenerator() { return tokens_; }

private:
    PlayerTokens tokens_;
    std::unordered_map<const model::Map*, GameSession> sessions_;
};

}  // namespace game_model
