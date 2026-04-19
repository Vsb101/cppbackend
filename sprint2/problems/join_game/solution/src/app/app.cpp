#include "../app/app.h"

namespace app {

const model::Game::Maps& Application::ListMap() const noexcept {
    return game_.GetMaps();
}

const std::shared_ptr<model::Map> Application::FindMap(
    const model::Map::Id& id) const noexcept {
    return game_.FindMap(id);
}

std::shared_ptr<model::GameSession> Application::GetOrCreateSession(
    const model::Map::Id& id) {
    auto session = game_.FindGameSessionBy(id);
    if (session) {
        return session;
    }
    session = std::make_shared<model::GameSession>(game_.FindMap(id), ioc_);
    game_.AddGameSession(session);
    return session;
}

std::tuple<model::Token, model::Player::Id> Application::JoinGame(
    const std::string& name, const model::Map::Id& id) {
    auto player = CreatePlayer(name);
    auto token = player_tokens_.AddPlayer(player);

    auto session = GetOrCreateSession(id);
    BindPlayerInSession(player, session);
    return std::make_tuple(token, player->GetId());
}

std::shared_ptr<model::Player> Application::CreatePlayer(
    std::string_view player_name) {
    auto player = std::make_shared<model::Player>(std::string(player_name));
    players_.push_back(player);
    return player;
}

void Application::BindPlayerInSession(
    std::shared_ptr<model::Player> player,
    std::shared_ptr<model::GameSession> session) {
    session_id_.try_emplace(session->GetId());
    session_id_.at(session->GetId()).push_back(player);
    player->SetGameSession(session);
    player->SetDog(session->CreateDog(player->GetName()));
}

std::optional<std::vector<std::weak_ptr<model::Player>>>
Application::GetPlayersFromSession(model::Token token) {
    auto player = player_tokens_.FindPlayerByToken(token).lock();
    if (!player) {
        return std::nullopt;
    }
    auto session_id = player->GetSessionId();
    auto it = session_id_.find(session_id);
    if (it == session_id_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool Application::CheckPlayerByToken(model::Token token) {
    return !player_tokens_.FindPlayerByToken(token).expired();
}

std::shared_ptr<Application::StrandApp> Application::GetStrand() {
    return strand_;
}

}  // namespace app
