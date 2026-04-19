#include "logger.h"
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/expressions.hpp>
#include <iostream>

namespace logger {

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

void StringFormatter(logging::record_view const& rec,
                     logging::formatting_ostream& strm) {
    // Просто выводим сообщение, так как CreateLogMessage уже сделал из него JSON
    strm << rec[expr::smessage];
}

void InitLogger() {
    logging::add_common_attributes();
    logging::add_console_log(
        std::clog, // Тесты практикума часто ждут логи именно в clog (stderr)
        keywords::auto_flush = true,
        keywords::format = &StringFormatter
    );
}

}  // namespace logger
