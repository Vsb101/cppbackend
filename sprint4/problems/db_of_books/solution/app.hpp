#pragma once

#include <pqxx/pqxx>
#include <unordered_map>
#include <memory>
#include <string>
#include "book_repository.hpp"
#include "commands.hpp"

class BookManagerApp {
public:
    explicit BookManagerApp(const char* conn_string);
    void Run();

private:
    void RegisterCommands();

private:
    pqxx::connection conn_;
    BookRepository repo_;
    std::unordered_map<std::string, std::unique_ptr<ICommand>> commands_;
};
