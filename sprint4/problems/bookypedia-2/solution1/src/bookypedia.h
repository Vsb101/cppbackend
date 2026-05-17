#pragma once

#include "app/use_cases_impl.h"
#include "postgres/postgres.h"
#include "ui/view.h"

namespace bookypedia {

struct AppConfig {
    std::string db_url;
    bool disable_cache = false; 
};

class Application {
public:
    explicit Application(const AppConfig& config);

    void Run();

private:
    postgres::Database db_;
    app::UseCasesImpl use_cases_;
};

}  // namespace bookypedia