#include "app.hpp"
#include "json_parser.hpp"
#include <iostream>

using namespace std::literals;

BookManagerApp::BookManagerApp(const char* conn_string) 
    : conn_(conn_string), repo_(conn_) {
    RegisterCommands();
}

void BookManagerApp::RegisterCommands() {
    commands_["add_book"] = std::make_unique<AddBookCommand>(repo_);
    commands_["all_books"] = std::make_unique<AllBooksCommand>(repo_);
}

void BookManagerApp::Run() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        auto action_opt = JsonParser::ExtractStringValue(line, "action");
        if (!action_opt) {
            std::cout << "{\"result\":false}\n"sv;
            continue;
        }

        if (*action_opt == "exit") {
            break;
        }

        auto it = commands_.find(*action_opt);
        if (it != commands_.end()) {
            try {
                std::cout << it->second->Execute(line) << "\n";
            } catch (const std::exception& e) {
                std::cerr << "Command execution error: " << e.what() << std::endl;
                std::cout << "{\"result\":false}\n"sv;
            }
        } else {
            std::cout << "{\"result\":false}\n"sv;
        }
    }
}
