#pragma once

#include <string>
#include <string_view>
#include "book_repository.hpp"

// Общий интерфейс для всех команд системы
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual std::string Execute(std::string_view line) = 0;
};

// Команда добавления книги
class AddBookCommand : public ICommand {
public:
    explicit AddBookCommand(BookRepository& repo);
    std::string Execute(std::string_view line) override;

private:
    BookRepository& repo_;
};

// Команда получения списка всех книг
class AllBooksCommand : public ICommand {
public:
    explicit AllBooksCommand(BookRepository& repo);
    std::string Execute(std::string_view line) override;

private:
    BookRepository& repo_;
};
