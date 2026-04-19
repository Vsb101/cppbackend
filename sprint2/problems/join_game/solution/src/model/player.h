#pragma once

/**
 * @file player.h
 * @brief Модель игрока
 */

#include <string>
#include <memory>

#include "../model/game_session.h"
#include "../other/tagged.h"

namespace model {

/**
 * @brief Игрок в сессии
 */
class Player {
 public:
    using Id = util::Tagged<size_t, Player>;

    explicit Player(std::string name);
    Player(Id id, std::string name);

    Player(const Player& other) = default;
    Player(Player&& other) = default;
    Player& operator=(const Player& other) = default;
    Player& operator=(Player&& other) = default;
    virtual ~Player() = default;

    void SetGameSession(std::weak_ptr<GameSession> session);
    void SetDog(std::weak_ptr<Dog> dog);

    [[nodiscard]] const Id& GetId() const;
    [[nodiscard]] const std::string& GetName() const;
    [[nodiscard]] const model::GameSession::Id& GetSessionId() const;
    [[nodiscard]] std::weak_ptr<Dog> GetDog();

 private:
    Id id_;
    std::string name_;
    std::weak_ptr<GameSession> session_;
    std::weak_ptr<Dog> dog_;

    inline static size_t cnt_max_id_ = 0;
};

}  // namespace model