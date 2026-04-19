#pragma once

/**
 * @file app.h
 * @brief Прикладной слой игрового сервера
 */

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "../model/game.h"
#include "../model/player.h"
#include "../model/player_tokens.h"
#include "../other/sdk.h"
#include "../other/tagged.h"

namespace app {

namespace net = boost::asio;

/**
 * @brief Прикладной слой, связывающий модель и HTTP-обработчик
 */
class Application {
 public:
    using StrandApp = net::strand<net::io_context::executor_type>;

    Application(model::Game game, net::io_context& ioc)
        : game_(std::move(game))
        , ioc_(ioc)
        , strand_(std::make_shared<StrandApp>(net::make_strand(ioc))) {}

    Application(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = delete;
    virtual ~Application() = default;

    [[nodiscard]] std::shared_ptr<StrandApp> GetStrand();
    [[nodiscard]] bool CheckPlayerByToken(model::Token token);
    [[nodiscard]] const model::Game::Maps& ListMap() const noexcept;
    [[nodiscard]] const std::shared_ptr<model::Map> FindMap(
        const model::Map::Id& id) const noexcept;

    std::tuple<model::Token, model::Player::Id> JoinGame(
        const std::string& player_name, const model::Map::Id& id);
    [[nodiscard]] std::optional<std::vector<std::weak_ptr<model::Player>>>
    GetPlayersFromSession(model::Token token);

 private:
    using IdHasher = util::TaggedHasher<model::GameSession::Id>;
    using IdToIndex = std::unordered_map<
        model::GameSession::Id,
        std::vector<std::weak_ptr<model::Player>>,
        IdHasher>;

    [[nodiscard]] std::shared_ptr<model::Player> CreatePlayer(
        std::string_view player_name);
    void BindPlayerInSession(std::shared_ptr<model::Player> player,
                             std::shared_ptr<model::GameSession> session);
    [[nodiscard]] std::shared_ptr<model::GameSession> GetOrCreateSession(
        const model::Map::Id& id);

    model::Game game_;
    std::vector<std::shared_ptr<model::Player>> players_;
    IdToIndex session_id_;
    model::PlayerTokens player_tokens_;
    net::io_context& ioc_;
    std::shared_ptr<StrandApp> strand_;
};

}  // namespace app
