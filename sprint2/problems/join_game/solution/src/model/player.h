#pragma once
#include <string>

#include "../model/game_session.h"
#include "../other/tagged.h"

namespace model {

class Player {
  inline static size_t cntMaxId = 0;

 public:
  using Id = util::Tagged<size_t, Player>;
  Player(std::string name) : id_(Id{Player::cntMaxId++}), name_(name) {};
  Player(Id id, std::string name) : id_(id), name_(name) {};
  Player(const Player& other) = default;
  Player(Player&& other) = default;
  Player& operator=(const Player& other) = default;
  Player& operator=(Player&& other) = default;
  virtual ~Player() = default;

  void SetGameSession(std::weak_ptr<GameSession> session);
  void SetDog(std::weak_ptr<Dog> dog);

  const Id& GetId() const;
  const std::string& GetName() const;
  const model::GameSession::Id& GetSessionId() const;
  std::weak_ptr<Dog> GetDog();

 private:
  Id id_;
  std::string name_;
  std::weak_ptr<GameSession> session_;
  std::weak_ptr<Dog> dog_;
};

}  // namespace model