#pragma once

namespace db::queries {

inline constexpr const char* INSERT_RETIRE_RECORD = 
    "INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3);";

inline constexpr const char* SELECT_RETIRE_RECORDS = 
    "SELECT name, score, play_time FROM retired_players "
    "ORDER BY score DESC, play_time ASC, name ASC LIMIT $1 OFFSET $2;";

} // namespace db::queries
