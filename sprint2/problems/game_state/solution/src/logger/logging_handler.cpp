#include "logging_handler.h"

namespace logger {

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                const RequestLog& request) {
    boost::json::object obj;
    obj["ip"] = request.ip;
    obj["URI"] = request.url; // Исправили uri -> url
    obj["method"] = request.method;
    json_value = std::move(obj);
}

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                const ServerAddrPortLog& server_address) {
    boost::json::object obj;
    obj["address"] = server_address.address;
    obj["port"] = server_address.port;
    json_value = std::move(obj);
}

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                const ExceptionLog& exception) {
    boost::json::object obj;
    obj["code"] = exception.code;
    obj["text"] = exception.text;
    obj["where"] = exception.where;
    json_value = std::move(obj);
}

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                const ExitCodeLog& exit_code) {
    boost::json::object obj;
    obj["code"] = exit_code.code;
    json_value = std::move(obj);
}

}  // namespace logger
