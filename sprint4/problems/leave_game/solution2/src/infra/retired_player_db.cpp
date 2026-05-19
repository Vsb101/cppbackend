#include "retired_player_db.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <stdexcept>

namespace infra {

RetiredPlayerDb::RetiredPlayerDb(std::shared_ptr<ConnectionPool> pool) : pool_(std::move(pool)) {
    if (!pool_) {
        throw std::runtime_error("ConnectionPool is null");
    }
    auto conn = pool_->GetConnection();
    pqxx::work work{*conn};

    work.exec(R"(
        CREATE TABLE IF NOT EXISTS retired_players (
            id UUID PRIMARY KEY,
            name TEXT NOT NULL,
            score INT NOT NULL,
            play_time_ms BIGINT NOT NULL
        )
    )");

    work.exec(R"(
        CREATE INDEX IF NOT EXISTS idx_retired_players_sort
        ON retired_players (score DESC, play_time_ms ASC, name ASC)
    )");

    work.commit();
}

void RetiredPlayerDb::Save(const std::string& name, int score, double play_time_sec) {
    try {
        auto conn = pool_->GetConnection();
        pqxx::work work{*conn};

        boost::uuids::random_generator gen;
        std::string id = boost::uuids::to_string(gen());
        long long play_time_ms = static_cast<long long>(play_time_sec * 1000.0);

        work.exec_params(
            "INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4)",
            id,
            name,
            score,
            play_time_ms
        );

        work.commit();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save retired player: ") + e.what());
    }
}

std::vector<app::RetiredPlayer> RetiredPlayerDb::GetRecords(size_t offset, size_t limit) {
    try {
        auto conn = pool_->GetConnection();
        pqxx::read_transaction tx{*conn};

        auto result = tx.exec_params(
            "SELECT name, score, play_time_ms FROM retired_players "
            "ORDER BY score DESC, play_time_ms ASC, name ASC "
            "LIMIT $1 OFFSET $2",
            static_cast<long long>(limit),
            static_cast<long long>(offset)
        );

        std::vector<app::RetiredPlayer> records;
        records.reserve(result.size());

        for (const auto& row : result) {
            app::RetiredPlayer rec;
            rec.name = row[0].as<std::string>();
            rec.score = row[1].as<int>();
            rec.play_time = row[2].as<long long>() / 1000.0;
            records.push_back(std::move(rec));
        }

        return records;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to get records: ") + e.what());
    }
}

} // namespace infra
