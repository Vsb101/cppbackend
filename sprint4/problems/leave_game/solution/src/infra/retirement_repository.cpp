#include "retirement_repository.h"
#include <pqxx/transaction>
#include <pqxx/result>
#include <string>
#include <boost/asio/post.hpp> // Важно для асинхронной отправки задач
#include "../logger/logger.h"

namespace infra {

RetirementRepository::RetirementRepository(const std::string& conn_str, 
                                           std::shared_ptr<boost::asio::thread_pool> db_pool)
    : pool_(4, [conn_str] {
        return std::make_shared<pqxx::connection>(conn_str);
    })
    , db_pool_(std::move(db_pool)) {
    CreateTableAndIndices();
    logger::LogInfo("RetirementRepository initialized");
}

void RetirementRepository::CreateTableAndIndices() {
    try {
        auto conn = pool_.GetConnection();
        pqxx::work txn(*conn);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS retired_players (
                name TEXT NOT NULL,
                score INT NOT NULL,
                play_time DOUBLE PRECISION NOT NULL
            );
        )");
        txn.exec(R"(
            CREATE INDEX IF NOT EXISTS idx_retired_players_sort
            ON retired_players (score DESC, play_time ASC, name ASC);
        )");
        txn.commit();
        logger::LogInfo("Database table and indices verified/created");
    } catch (const std::exception& ex) {
        logger::LogError("Database initialization failed", std::string(ex.what()));
        throw;
    }
}

void RetirementRepository::AddRecord(const std::string& name, int score, double play_time) {
    try {
        logger::LogInfo("AddRecord execution started", name);
        auto conn = pool_.GetConnection();
        pqxx::work txn(*conn);
        
        // Используем безопасный параметризованный запрос
        txn.exec_params(
            "INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3);",
            name, score, play_time
        );
        
        txn.commit();
        logger::LogInfo("AddRecord successfully committed", name);
    } catch (const std::exception& ex) {
        logger::LogError("AddRecord database transaction failed", std::string(ex.what()));
        throw; // Прокидываем наверх, чтобы слой app залогировал проблему
    }
}


std::vector<Record> RetirementRepository::GetRecords(size_t start, size_t max_items) {
    try {
        auto conn = pool_.GetConnection();
        pqxx::work txn(*conn);
        
        auto result = txn.exec_params(
            "SELECT name, score, play_time FROM retired_players "
            "ORDER BY score DESC, play_time ASC, name ASC "
            "LIMIT $1 OFFSET $2;",
            max_items, start
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
