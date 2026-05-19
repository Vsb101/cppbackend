#include "postgres.h"
#include "players.h"

#include <pqxx/pqxx>
#include <pqxx/zview.hxx>
#include <vector>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void PlayerRepositoryImpl::Save(const app::PlayerRecordInfo& player) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO records (id, name, score, play_time) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET name=$2, score=$3, play_time=$4;
)"_zv,
        player.id.ToString(), player.name, player.score, player.play_time);
    work.commit();
}

std::vector<app::PlayerStatInfo> PlayerRepositoryImpl::GetRecords(size_t start, size_t limit) {
    pqxx::read_transaction r(connection_);
    auto query_text = R"(
SELECT name, score, play_time 
FROM records 
ORDER BY score DESC, play_time, name
OFFSET )" + std::to_string(start) + R"( 
LIMIT )" + std::to_string(limit) + R"(
;)";

    std::vector<app::PlayerStatInfo> res;

    for (auto [name, score, play_time] : r.query<std::string, int, double>(query_text)) {
        res.emplace_back(app::PlayerStatInfo{name, score, play_time});
    }

    return res;
}


Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS records (
    id UUID CONSTRAINT player_id_constraint PRIMARY KEY,
    name varchar(100) NOT NULL,
    score INTEGER NOT NULL,
    play_time DOUBLE PRECISION NOT NULL
);
)"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres