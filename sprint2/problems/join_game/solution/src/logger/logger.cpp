#include "logger.h"

namespace logger {

    void StringFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
        
        strm << rec[expr::smessage];            //Выводим месседж
    }

    void InitLogger() {
        boost::log::add_console_log(
            std::cout,
            keywords::auto_flush = true,
            boost::log::keywords::format = &StringFormatter
        );
    };

}