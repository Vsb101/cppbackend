#include <iostream>

#include "../app/app.h"

namespace app {

const model::Game::Maps& Application::ListMap() const noexcept {
  return game_.GetMaps();
};

const std::shared_ptr<model::Map> Application::FindMap(
    const model::Map::Id& id) const noexcept {
  return game_.FindMap(id);
};

std::tuple<model::Token, model::Player::Id> Application::JoinGame(
    const std::string& name, const model::Map::Id& id) {
  auto player = CreatePlayer(name);
  auto token = playerTokens_.AddPlayer(player);

  std::shared_ptr<model::GameSession> session = game_.FindGameSessionBy(id);
  if (!session) {
    session = std::make_shared<model::GameSession>(game_.FindMap(id), ioc_);
    game_.AddGameSession(session);
  }
  BindPlayerInSession(player, session);
  return std::make_tuple(token, player->GetId());
};

std::shared_ptr<model::Player> Application::CreatePlayer(const std::string& player_name) {
  auto player = std::make_shared<model::Player>(player_name);
  players_.push_back(player);
  return player;
};

void Application::BindPlayerInSession(std::shared_ptr<model::Player> player,
                                      std::shared_ptr<model::GameSession> session) {
  sessionID_[session->GetId()].push_back(player);
  player->SetGameSession(session);
  player->SetDog(session->CreateDog(player->GetName()));
};

const std::vector<std::weak_ptr<model::Player> >& Application::GetPlayersFromSession(
    model::Token token) {
  static const std::vector<std::weak_ptr<model::Player> > emptyPlayerList;
  auto player = playerTokens_.FindPlayerByToken(token).lock();
  if (!player) {
    return emptyPlayerList;
  }
  auto session_id = player->GetSessionId();
  if (!sessionID_.contains(session_id)) {
    return emptyPlayerList;
  }
  return sessionID_[session_id];
};

bool Application::CheckPlayerByToken(model::Token token) {
  return !playerTokens_.FindPlayerByToken(token).expired();
};

std::shared_ptr<Application::StrandApp> Application::GetStrand() { return strand_; };

}  // namespace app
