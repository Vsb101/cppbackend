#include "../logger/logging_handler.h"

namespace logger {

    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const RequestLog& request) {
        jv = { {IP, json::value_from(request.ip)},
                {URL, json::value_from(request.url)},
                {METHOD, json::value_from(request.method)} };
    };

    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const ServerAddrPortLog& server_address) {
        jv = { {PORT, json::value_from(server_address.port)},
                {ADDRESS, json::value_from(server_address.address)} };
    };

    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const ExceptionLog& exception) {
        jv = { {CODE, json::value_from(exception.code)},
                {TEXT, json::value_from(exception.text)},
                {WHERE, json::value_from(exception.where)} };
    };

    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const ExitCodeLog& exit_code) {
        jv = { {CODE, json::value_from(exit_code.code)} };
    };

}