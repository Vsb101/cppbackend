#pragma once

namespace db::queries {

inline constexpr const char* INSERT_RETIRE_RECORD = 
    "INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3);";

inline constexpr const char* SELECT_RETIRE_RECORDS = 
    "SELECT name, score, play_time FROM retired_players "
    "ORDER BY score DESC, play_time ASC, name ASC LIMIT $1 OFFSET $2;";

inline constexpr const char* CREATE_RETIRE_TABLE = 
    "CREATE TABLE IF NOT EXISTS retired_players ("
    "    id SERIAL PRIMARY KEY,"
    "    name VARCHAR(100) NOT NULL,"
    "    score INT NOT NULL,"
    "    play_time DOUBLE PRECISION NOT NULL"
    ");";

inline constexpr const char* CREATE_RETIRE_INDEX = 
    "CREATE INDEX IF NOT EXISTS idx_retired_players_score_time_name "
    "ON retired_players (score DESC, play_time ASC, name ASC);";

} // namespace db::queries
