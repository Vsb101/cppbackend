#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../app/retired_player.h"
#include "db_connection_pool.h"

namespace infra {

class RetiredPlayerDb {
public:
    explicit RetiredPlayerDb(std::shared_ptr<ConnectionPool> pool);

    void Save(const std::string& name, int score, double play_time_sec);
    std::vector<app::RetiredPlayer> GetRecords(size_t offset, size_t limit);

private:
    std::shared_ptr<ConnectionPool> pool_;
};

} // namespace infra
