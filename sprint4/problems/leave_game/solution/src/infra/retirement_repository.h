#pragma once

#include <string>
#include <vector>
#include "record.h"
#include "connection_pool.h"

namespace infra {

class RetirementRepository {
public:
    explicit RetirementRepository(const std::string& conn_str);

    void AddRecord(const std::string& name, int score, double play_time);
    std::vector<Record> GetRecords(size_t start, size_t max_items);

private:
    void CreateTableIfNotExists();
    
    ConnectionPool pool_;
};

} // namespace infra
