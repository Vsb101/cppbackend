#pragma once

/**
 * @file game_session.h
 * @brief Игровая сессия для конкретной карты
 */

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <vector>

#include "../other/tagged.h"
#include "dog.h"
#include "map.h"

namespace model {

namespace net = boost::asio;

/**
 * @brief Сессия игры на конкретной карте
 */
class GameSession {
 public:
    using SessionStrand = net::strand<net::io_context::executor_type>;
    using Id = util::Tagged<std::string, GameSession>;

    GameSession(std::shared_ptr<Map> map, net::io_context& ioc);

    std::shared_ptr<Dog> CreateDog(const std::string& name);

    [[nodiscard]] const Id& GetId() const noexcept;
    [[nodiscard]] std::shared_ptr<Map> GetMap();
    [[nodiscard]] std::shared_ptr<SessionStrand> GetStrand();

 private:
    std::shared_ptr<Map> map_;
    std::shared_ptr<SessionStrand> strand_;
    Id id_;
    std::vector<std::shared_ptr<Dog>> dogs_;

    void PutDogInRndPosition(std::shared_ptr<Dog> dog);
};

}  // namespace model