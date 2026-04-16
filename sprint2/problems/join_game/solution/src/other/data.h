#pragma pnce

#include <boost/beast/http.hpp>
#include <optional>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;

namespace response_data {
template <typename Body, typename Fields>
struct Data {
  Data(std::string ip_addr, long res_time, const http::response<Body, Fields>& res)
      : ip(ip_addr),
        res_time(res_time),
        code(res.result_int()),
        content_type(res[http::field::content_type]) {};

  long res_time;
  std::string ip;
  int code;
  std::string content_type;
};
}  // namespace response_data

namespace request_data {
struct Data {
  Data(std::string ip_addr,
       const http::request<http::string_body> req)
      : ip(ip_addr), url(req.target()), method(req.method_string()){};

  std::string ip;
  std::string url;
  std::string method;
};
}  // namespace request_data