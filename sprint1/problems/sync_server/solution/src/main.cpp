
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
using beast::string_view;
namespace http = beast::http;
using beast::string_view;

http::response<http::string_body> MakeResponse(http::request<http::string_body>&& request, const std::string& target) {
    http::response<http::string_body> response;
    
    if (request.method() == http::verb::get) {
        response.result(http::status::ok);
        response.set(http::field::content_type, string_view("text/html"));
        std::string body = "Hello, " + target;
        response.body() = body;
        response.content_length(body.size());
    } else if (request.method() == http::verb::head) {
        response.result(http::status::ok);
        response.set(http::field::content_type, string_view("text/html"));
        std::string body = "Hello, " + target;
        response.content_length(body.size());
    } else {
        response.result(http::status::method_not_allowed);
        response.set(http::field::content_type, string_view("text/html"));
        response.set(http::field::allow, string_view("GET, HEAD"));
        response.body() = "Invalid method.";
        response.content_length(14);
    }
    
    response.set(http::field::server, string_view("cpp-backend-server"));
    
    
    return response;
}

std::string ExtractTarget(const std::string& target) {
    if (!target.empty() && target[0] == '/') {
        return target.substr(1);
    }
    return target;
}

int main() {
    const auto address = net::ip::make_address("127.0.0.1");
    const auto port = static_cast<unsigned short>(8080);

    net::io_context ioc;
    tcp::acceptor acceptor(ioc, {address, port});
    
    std::cout << string_view("Server has started...") << std::endl;
    
    while (true) {
        tcp::socket socket(ioc);
        acceptor.accept(socket);
        
        beast::flat_buffer buffer;
        http::request<http::string_body> request;
        
        http::read(socket, buffer, request);
        
        std::string target = ExtractTarget(std::string(request.target()));
        
        http::response<http::string_body> response = MakeResponse(std::move(request), target);
        
        http::write(socket, response);
        
        socket.shutdown(tcp::socket::shutdown_send);
    }
}