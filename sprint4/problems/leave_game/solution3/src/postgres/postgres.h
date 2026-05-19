#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../app/players.h"

namespace postgres {

class PlayerRepositoryImpl : public app::PlayerRepository {
public:
    explicit PlayerRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const app::PlayerRecordInfo& player) override;
    std::vector<app::PlayerStatInfo> GetRecords(size_t start, size_t limit) override;

private:
    pqxx::connection& connection_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    PlayerRepositoryImpl& GetPlayers() & {
        return players_;
    }

private:
    pqxx::connection connection_;
    PlayerRepositoryImpl players_{connection_};
};

}  // namespace postgres