#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {
        while (!IsGameEnded()) {
            PrintFields();
            if (my_initiative) {
                std::string input;
                std::cout << "Enter your move (e.g. A1): ";
                std::cin >> input;
                auto move = ParseMove(input);
                if (!move) {
                    std::cout << "Invalid move. Try again.\n";
                    continue;
                }
                
                if (!WriteExact(socket, MoveToString(*move))) {
                    std::cout << "Failed to send move.\n";
                    return;
                }
                
                auto result_data = ReadExact<1>(socket);
                if (!result_data) {
                    std::cout << "Failed to read result.\n";
                    return;
                }
                
                auto result = static_cast<SeabattleField::ShotResult>(result_data->at(0));
                switch (result) {
                    case SeabattleField::ShotResult::MISS:
                        std::cout << "Miss!\n";
                        my_initiative = false;
                        break;
                    case SeabattleField::ShotResult::HIT:
                        std::cout << "Hit!\n";
                        other_field_.MarkHit(move->first, move->second);
                        break;
                    case SeabattleField::ShotResult::KILL:
                        std::cout << "Kill!\n";
                        other_field_.MarkKill(move->first, move->second);
                        break;
                }
            } else {
                std::cout << "Waiting for opponent's move...\n";
                auto move_data = ReadExact<2>(socket);
                if (!move_data) {
                    std::cout << "Failed to read move.\n";
                    return;
                }
                
                auto move = ParseMove(*move_data);
                if (!move) {
                    std::cout << "Invalid move received.\n";
                    return;
                }
                
                auto result = my_field_.Shoot(move->first, move->second);
                if (!WriteExact(socket, std::string(1, static_cast<char>(result)))) {
                    std::cout << "Failed to send result.\n";
                    return;
                }
                
                if (result == SeabattleField::ShotResult::MISS) {
                    my_initiative = true;
                } else if (result == SeabattleField::ShotResult::KILL) {
                    std::cout << "Your ship has been sunk!\n";
                } else {
                    std::cout << "Hit on your ship!\n";
                }
            }
        }
        
        PrintFields();
        if (my_field_.IsLoser()) {
            std::cout << "You lost!\n";
        } else {
            std::cout << "You won!\n";
        }
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[1] - '1', p2 = sv[0] - 'A';

        if (p1 < 0 || p1 > 7) return std::nullopt;
        if (p2 < 0 || p2 > 7) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.second) + 'A', static_cast<char>(move.first) + '1'};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    // TODO: добавьте методы по вашему желанию

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);
    
    net::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    tcp::socket socket(io_context);
    acceptor.accept(socket);
    
    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);
    
    net::io_context io_context;
    tcp::socket socket(io_context);
    tcp::endpoint endpoint(net::ip::make_address(ip_str), port);
    socket.connect(endpoint);
    
    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
