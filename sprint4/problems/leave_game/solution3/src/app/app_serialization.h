#pragma once

#include <boost/serialization/vector.hpp>
#include <cstddef>
#include <vector>

#include "players.h"
#include "model_serialization.h"

namespace serialization {

// GameRepr (GameRepresentation) - сериализованное представление класса Game
class GameRepr {
public:
    GameRepr() = default;

    explicit GameRepr(const model::Game& game) {
        for (const auto& session : game.GetSessions()) {
            sessions_.push_back(GameSessionRepr(*session));
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & sessions_;
    }

private:
    std::vector<GameSessionRepr> sessions_;
};

class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(const app::Player& player)
    : id_{player.GetId()}
    , name_{player.GetName()}
    , session_{player.GetSession()->GetMap().GetId()}
    , dog_{player.GetDog().GetId()} {
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & *id_;
        ar & *name_;
        ar & *session_;
        ar & *dog_;
    }

    const app::Player::Id& GetId() const {
        return id_;
    }

    const app::Player::Name& GetName() const {
        return name_;
    }

    const model::Map::Id& GetSession() const {
        return session_;
    }

    const model::Dog::Id& GetDog() const {
        return dog_;
    }

private:
    app::Player::Id id_ = app::Player::Id{0};
    app::Player::Name name_ = app::Player::Name{""};
    model::Map::Id session_ = model::Map::Id{""};   // В данный момент игровая сессия соответсвует одной карте
    model::Dog::Id dog_ = model::Dog::Id{0};
};

class TokenRepr {
public:
    TokenRepr() = default;

    explicit TokenRepr(const app::Token& token) {
        token_ = token;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & *token_;
    }

    const app::Token& GetToken() const {
        return token_;
    }

private:
    app::Token token_;
};

class PlayerIdRepr {
public:
    PlayerIdRepr() = default;

    explicit PlayerIdRepr(const app::Player::Id& id) {
        id_ = id;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & *id_;
    }

    const app::Player::Id& GetId() const {
        return id_;
    }

private:
    app::Player::Id id_;
};

class TokensRepr {
public:
    TokensRepr() = default;

    explicit TokensRepr(const app::PlayerTokens::PlayersIdsToTokens& player_id_to_token) {
        for (const auto& [player_id, token] : player_id_to_token) {
            player_ids_.push_back(PlayerIdRepr(player_id));
            tokens_.push_back(TokenRepr(token));
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & player_ids_;
        ar & tokens_;
    }

    app::PlayerTokens::PlayersIdsToTokens GetPlayerIdToToken() const {
        app::PlayerTokens::PlayersIdsToTokens res;
        for (std::size_t i = 0; i < player_ids_.size(); ++i) {
            res[player_ids_[i].GetId()] = tokens_[i].GetToken();
        }
        return res;
    }

private:
    std::vector<TokenRepr> tokens_;
    std::vector<PlayerIdRepr> player_ids_;
};

}  // namespace serialization
