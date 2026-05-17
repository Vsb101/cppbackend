#include "retirement_repository.h"
#include <pqxx/transaction>
#include <pqxx/result>
#include <string>
#include "../logger/logger.h"

namespace infra {

RetirementRepository::RetirementRepository(const std::string& conn_str)
    : pool_(4, [conn_str] {
        return std::make_shared<pqxx::connection>(conn_str);
    }) {
    CreateTableIfNotExists();
    logger::LogInfo("RetirementRepository initialized");
}

void RetirementRepository::CreateTableIfNotExists() {
    try {
        auto conn = pool_.GetConnection();
        pqxx::work txn(*conn);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS retired_players (
                name TEXT NOT NULL,
                score INT NOT NULL,
                play_time DOUBLE PRECISION NOT NULL
            )
        )");
        txn.exec(R"(
            CREATE INDEX IF NOT EXISTS idx_retired_players_sort
            ON retired_players (score DESC, play_time ASC, name ASC)
        )");
        txn.commit();
        logger::LogInfo("Database table created");
    } catch (const std::exception& ex) {
        logger::LogError("Database initialization failed", std::string(ex.what()));
        throw;
    }
}

void RetirementRepository::AddRecord(const std::string& name, int score, double play_time) {
    try {
        logger::LogInfo("AddRecord called", name);
        auto conn = pool_.GetConnection();
        pqxx::work txn(*conn);
        txn.exec(
            "INSERT INTO retired_players (name, score, play_time) VALUES (" +
            txn.quote(name) + ", " + std::to_string(score) + ", " + std::to_string(play_time) + ")"
        );
        txn.commit();
        logger::LogInfo("AddRecord committed", name);
    } catch (const std::exception& ex) {
        logger::LogError("AddRecord failed", std::string(ex.what()));
        throw;
    }
}

std::vector<Record> RetirementRepository::GetRecords(size_t start, size_t max_items) {
    try {
        auto conn = pool_.GetConnection();
        pqxx::work txn(*conn);
        auto result = txn.exec(
            "SELECT name, score, play_time FROM retired_players "
            "ORDER BY score DESC, play_time ASC, name ASC "
            "LIMIT " + std::to_string(max_items) + " OFFSET " + std::to_string(start)
        );
        std::vector<Record> records;
        records.reserve(result.size());
        for (const auto& row : result) {
            records.push_back({
                row[0].as<std::string>(),
                row[1].as<int>(),
                row[2].as<double>()
            });
        }
        return records;
    } catch (const std::exception& ex) {
        logger::LogError("GetRecords failed", std::string(ex.what()));
        throw;
    }
}

} // namespace infra
