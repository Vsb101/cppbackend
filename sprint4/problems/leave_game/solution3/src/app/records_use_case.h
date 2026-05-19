#pragma once

#include "use_cases_impl.h"

namespace app {

struct RecordsParams {
    size_t start;
    size_t limit;
};

struct RecordsResult {
    std::vector<PlayerStatInfo> records;
};
   

class RecordsUseCase {
public:
    RecordsUseCase(UseCasesImpl& db_use_cases);
    RecordsResult GetRecords(RecordsParams params);

private:
    UseCasesImpl* db_use_cases_;
};

}  // namespace app
