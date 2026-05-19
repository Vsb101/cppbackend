#include "http_handler_types.h"
#include "json_fields.h"

namespace http_handler {

// сериализация экземпляра класса ResponseError в JSON-значение
void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, http_handler::ResponseError const& resp_err)
{
    jv = {
        {json_field::ERROR_CODE, resp_err.code},
        {json_field::ERROR_MESSAGE, resp_err.message}
    };
}

}  // namespace http_handler
