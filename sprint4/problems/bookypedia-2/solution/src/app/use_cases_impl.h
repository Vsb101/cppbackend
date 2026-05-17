#pragma once

#include "use_cases.h"
#include "../domain/author.h"
#include "../postgres/postgres.h"
#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <utility>

namespace app {

class UseCasesImpl : public UseCases {
public:
    // Конструктор для основного приложения (принимает БД)
    UseCasesImpl(
        postgres::Database& db,
        std::shared_ptr<domain::AuthorRepository> authors,
        std::shared_ptr<domain::BookRepository> books);

    // Старый конструктор для совместимости с юнит-тестами
    UseCasesImpl(
        std::shared_ptr<domain::AuthorRepository> authors,
        std::shared_ptr<domain::BookRepository> books);

    void AddAuthor(const std::string& name) override;
    
    AddBookResult AddBookWithAuthorSelection(
        const std::string& title,
        int publication_year,
        const std::string& author_input,
        const std::vector<std::string>& tags) override;
        
    std::vector<domain::Author> GetAuthors() const override;
    std::vector<domain::Book> GetBooks() const override;
    
    bool DeleteAuthor(const std::string& author_input) override;
    bool EditAuthor(const std::string& author_input, const std::string& new_name) override;
    bool DeleteBook(const std::string& book_input) override;
    
    bool EditBook(
        const std::string& book_input,
        const std::string& new_title,
        int publication_year,
        const std::vector<std::string>& tags) override;
        
    std::optional<domain::Book> GetBookByTitle(const std::string& title) const override;
    std::vector<domain::Book> GetBooksByTitle(const std::string& title) const override;
    std::vector<domain::Book> GetAuthorBooks(const std::string& author_id) const override;

    void AddBook(const domain::Book& book) { (void)book; }

private:
    // Шаблонный хелпер для прозрачной работы с транзакциями (в .h файле)
    template <typename Block>
    void ExecuteInTransaction(Block&& block) const {
        if (db_) {
            db_->ExecuteTransaction(std::forward<Block>(block));
        } else {
            block(); // Если БД нет (в юнит-тестах), просто выполняем код атомарно
        }
    }

    postgres::Database* db_ = nullptr; // Изменено на указатель
    std::shared_ptr<domain::AuthorRepository> authors_;
    std::shared_ptr<domain::BookRepository> books_;
};

}  // namespace app
