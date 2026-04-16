#pragma once

#include <string>
#include "model/model.h"
#include "tagged.h"

namespace app {

/**
 * @brief Игрок в приложении
 * 
 * Сущность уровня приложения (не доменной модели).
 * Содержит токен аутентификации и ссылку на пса.
 */
class Player {
public:
    using Id = util::Tagged<int, Player>;
    using Token = std::string;

    Player(Id id, Token token, size_t dog_index) noexcept
        : id_(id), token_(std::move(token)), dog_index_(dog_index) {}

    Id GetId() const noexcept { return id_; }
    const Token& GetToken() const noexcept { return token_; }
    size_t GetDogIndex() const noexcept { return dog_index_; }

private:
    Id id_;
    Token token_;
    size_t dog_index_;
};

}  // namespace app
