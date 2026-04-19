#include "../logger/logging_handler.h"

namespace logger {

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                const RequestLog& request) {
    json_value = {{kIp, json::value_from(request.ip)},
                  {kUrl, json::value_from(request.url)},
                  {kMethod, json::value_from(request.method)}};
}

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                const ServerAddrPortLog& server_address) {
    json_value = {{kPort, json::value_from(server_address.port)},
                  {kAddress, json::value_from(server_address.address)}};
}

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                const ExceptionLog& exception) {
    json_value = {{kCode, json::value_from(exception.code)},
                  {kText, json::value_from(exception.text)},
                  {kWhere, json::value_from(exception.where)}};
}

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                const ExitCodeLog& exit_code) {
    json_value = {{kCode, json::value_from(exit_code.code)}};
}

}  // namespace logger