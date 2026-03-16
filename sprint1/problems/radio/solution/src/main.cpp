#include "audio.h"
#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>

using namespace std::literals;

void StartServer(uint16_t port) {
    boost::asio::io_context io_context;
    boost::asio::ip::udp::socket socket(io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port));
    Player player(ma_format_u8, 1);

    std::vector<char> buffer(65000);

    while (true) {
        boost::asio::ip::udp::endpoint sender_endpoint;
        boost::system::error_code error;

        size_t length = socket.receive_from(boost::asio::buffer(buffer), sender_endpoint, 0, error);

        if (!error && length > 0) {
            size_t frame_size = player.GetFrameSize();
            size_t frames = length / frame_size;

            std::cout << "[СЕРВЕР] Получено " << length << " байт от "
                      << sender_endpoint.address().to_string() << ":"
                      << sender_endpoint.port()
                      << " (" << frames << " фреймов)" << std::endl;

            player.PlayBuffer(buffer.data(), frames, 1.5s);
        } else {
            std::cerr << "[СЕРВЕР] Ошибка приёма: " << error.message() << std::endl;
        }
    }
}

void StartClient(uint16_t port) {
    boost::asio::io_context io_context;
    boost::asio::ip::udp::socket socket(io_context);
    socket.open(boost::asio::ip::udp::v4());

    Recorder recorder(ma_format_u8, 1);

    std::string ip_str;
    while (true) {
        std::cout << "Enter server IP address: ";
        std::getline(std::cin, ip_str);

        if (ip_str.empty()) continue;

        auto rec_result = recorder.Record(65000, 1.5s);
        std::cout << "Recording done" << std::endl;

        try {
            boost::asio::ip::address server_ip = boost::asio::ip::make_address(ip_str);
            boost::asio::ip::udp::endpoint endpoint(server_ip, port);

            size_t frame_size = recorder.GetFrameSize();
            size_t bytes_to_send = rec_result.frames * frame_size;

            socket.send_to(boost::asio::buffer(rec_result.data, bytes_to_send), endpoint);
            std::cout << "Message sent" << std::endl;
        } catch (std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <client|server> <port>" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

    if (mode == "server") {
        StartServer(port);
    } else if (mode == "client") {
        StartClient(port);
    } else {
        std::cerr << "Mode must be 'client' or 'server'" << std::endl;
        return 1;
    }

    return 0;
}