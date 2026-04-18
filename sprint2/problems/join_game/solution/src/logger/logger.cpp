#include "logger.h"

namespace logger {

void StringFormatter(logging::record_view const& rec,
                     logging::formatting_ostream& strm) {
    // Сообщение уже в JSON-формате из CreateLogMessage
    strm << rec[expr::smessage];
}

void InitLogger() {
    boost::log::add_console_log(
        std::cout,
        keywords::auto_flush = true,
        boost::log::keywords::format = &StringFormatter);
}

}  // namespace logger