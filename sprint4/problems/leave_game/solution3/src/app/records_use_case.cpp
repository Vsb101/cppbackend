#include "records_use_case.h"

namespace app {
    
RecordsUseCase::RecordsUseCase(UseCasesImpl& db_use_cases) 
    : db_use_cases_{&db_use_cases} {
}

RecordsResult RecordsUseCase::GetRecords(RecordsParams params) {
    return RecordsResult(db_use_cases_->GetRecords(params.start, params.limit));
}

}