#pragma once

#include <string>
#include <memory>

#include "../model/game_session.h"
#include "../model/dog.h" // Нужен для weak_ptr<Dog>
#include "../other/tagged.h"

namespace app { // Сменили namespace на app

class Player {
 public:
    using Id = util::Tagged<size_t, Player>;

    // Оставляем только конструктор с явным ID
    Player(Id id, std::string name)
        : id_(std::move(id))
        , name_(std::move(name)) {}

    void SetGameSession(std::shared_ptr<model::GameSession> session) {
        session_ = std::move(session);
    }
    
    void SetDog(std::shared_ptr<model::Dog> dog) {
        dog_ = std::move(dog);
    }

    const Id& GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    
    // Возвращаем shared_ptr через lock, чтобы безопасно работать с объектами
    std::shared_ptr<model::GameSession> GetSession() const {
        return session_.lock();
    }
    
    std::shared_ptr<model::Dog> GetDog() const {
        return dog_.lock();
    }

 private:
    Id id_;
    std::string name_;
    std::weak_ptr<model::GameSession> session_;
    std::weak_ptr<model::Dog> dog_;
};

}  // namespace app
