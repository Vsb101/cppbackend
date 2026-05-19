#include "request_handler.h"

#include "boost/beast/http/status.hpp"
#include <boost/beast/http/file_body.hpp>
#include <boost/json.hpp>
#include <boost/url.hpp>
#include <string_view>

namespace http_handler {

StringResponse RequestHandler::ReportServerError(unsigned version, bool keep_alive) const {
    auto body = "Internal Server Error"s;
    return MakeStringResponse(http::status::internal_server_error, body, body.size(), version, keep_alive, ContentType::TEXT_PLAIN);
}

// Создаёт StringResponse с заданными параметрами
StringResponse RequestHandler::MakeStringResponse(http::status status, std::string_view body, size_t size, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type) const {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    if (status == http::status::method_not_allowed) {
        response.set(http::field::allow, "GET, HEAD");
    }
    response.body() = body;
    response.content_length(size);
    response.keep_alive(keep_alive);
    return response;
}

}  // namespace http_handler
