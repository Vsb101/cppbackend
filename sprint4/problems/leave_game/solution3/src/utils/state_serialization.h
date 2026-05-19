#pragma once

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <cstdio>
#include <fstream>

#include <boost/serialization/vector.hpp>
#include <vector>

#include "app.h"
#include "game_session.h"
#include "players.h"
#include "game.h"
#include "app_serialization.h"

namespace serialization {

class StateSerializer {
public:
    StateSerializer(model::Game& game, app::Application& app)
    : game_(&game)
    , app_(&app)
    {}

    void Serialize(const std::filesystem::path& path) {
        auto tmp_file = std::string(path) + ".tmp";
        std::ofstream out{tmp_file, std::ios_base::binary};
        //boost::archive::binary_oarchive ar{out};
        boost::archive::text_oarchive ar{out};

        // сохраняем игру
        ar << game_->GetSessions().size();  // Надо сохранить количество сессий
        for ( const auto& session : game_->GetSessions() ) {
            ar << GameSessionRepr(*session);
        }

        // Сохраняем токены авторизации игроков
        const auto& player_tokens = app_->GetTokens();
        ar << TokensRepr(player_tokens.GetPlayersTokens());

        // Сохраняем данные игроков
        const auto& players = app_->GetPlayers();
        ar << players.GetPlayers().size();
        for ( const auto& player : players.GetPlayers() ) {
            ar << PlayerRepr(*player);
        }

        std::rename(tmp_file.c_str(), path.c_str());
    }

    void Deserialize(const std::filesystem::path& path) {
        std::ifstream in{std::string(path), std::ios_base::binary};
        //boost::archive::binary_oarchive ar{out};
        boost::archive::text_iarchive ar{in};

        // Сначала загружаем игру
        size_t sessions_size;
        ar >> sessions_size;  // Надо загрузить количество сессий
        for ( size_t i = 0; i < sessions_size; ++i ) {
            // Создаём заготовку под представление сессии
            GameSessionRepr session_repr;
            ar >> session_repr;
            auto session = game_->CreateSession(session_repr.GetMapId());
            for ( const auto& dog_repr : session_repr.GetDogs() ) {
                session->AddDog(dog_repr.Restore());
            }
            for ( const auto& item_repr : session_repr.GetItems() ) {
                session->AddItem(item_repr.Restore());
            }
        }

        // Загружаем токены авторизации игроков
        TokensRepr token_repr;
        ar >> token_repr;
        auto tokens_table = token_repr.GetPlayerIdToToken();

        // Загружаем данные игроков
        size_t players_size;
        ar >> players_size;
        for ( size_t i = 0; i < players_size; ++i ) {
            // Создаём заготовку под представление игрока
            PlayerRepr player_repr;
            ar >> player_repr;
            app_->AddPlayer(player_repr, tokens_table[player_repr.GetId()]);
        }
    }

public:
    model::Game* game_;
    app::Application* app_;
};

}  // namespace serialization
