#pragma once

#include <string>
#include <vector>
#include <memory>
#include <boost/asio/thread_pool.hpp> // КРИТИЧЕСКИ ВАЖНО для thread_pool

#include "record.h"
#include "connection_pool.h"

namespace infra {

class RetirementRepository {
public:
    // Конструктор теперь строго принимает два аргумента
    RetirementRepository(
        const std::string& conn_str, 
        std::shared_ptr<boost::asio::thread_pool> db_pool
    );

    void AddRecord(const std::string& name, int score, double play_time);
    std::vector<Record> GetRecords(size_t start, size_t max_items);

private:
    void CreateTableAndIndices();
    
    ConnectionPool pool_;
    std::shared_ptr<boost::asio::thread_pool> db_pool_;
};

} // namespace infra
