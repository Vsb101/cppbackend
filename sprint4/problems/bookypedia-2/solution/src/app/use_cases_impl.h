#pragma once

#include "use_cases.h"
#include "../domain/author.h"
#include <memory>

namespace app {

class UseCasesImpl : public UseCases {
public:
    UseCasesImpl(
        std::shared_ptr<domain::AuthorRepository> authors,
        std::shared_ptr<domain::BookRepository> books);

    void AddAuthor(const std::string& name) override;
    AddBookResult AddBookWithAuthorSelection(
        const std::string& title,
        int publication_year,
        const std::string& author_input,
        const std::vector<std::string>& tags) override;
    std::vector<domain::Author> GetAuthors() const;
    std::vector<domain::Book> GetBooks() const;
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

    // Для обратной совместимости
    void AddBook(const domain::Book& book) {
        (void)book;  // Пока не используем, только для совместимости
    }

private:
    std::shared_ptr<domain::AuthorRepository> authors_;
    std::shared_ptr<domain::BookRepository> books_;
};

}  // namespace app
