#pragma once

#include <string>
#include <vector>
#include <optional> // <-- Обязательно добавить для std::optional
#include "../domain/author.h"

namespace app {

struct AddBookResult {
    domain::Book book;
    bool author_added = false;
};

class UseCases {
public:
    // Деструктор обязательно должен быть public и virtual
    virtual ~UseCases() = default; 

    virtual void AddAuthor(const std::string& name) = 0;
    virtual AddBookResult AddBookWithAuthorSelection(
        const std::string& title,
        int publication_year,
        const std::string& author_input,
        const std::vector<std::string>& tags) = 0;
    virtual std::vector<domain::Author> GetAuthors() const = 0;
    virtual std::vector<domain::Book> GetBooks() const = 0;
    virtual bool DeleteAuthor(const std::string& author_input) = 0;
    virtual bool EditAuthor(const std::string& author_input, const std::string& new_name) = 0;
    virtual bool DeleteBook(const std::string& book_input) = 0;
    virtual bool EditBook(
        const std::string& book_input,
        const std::string& new_title,
        int publication_year,
        const std::vector<std::string>& tags) = 0;
    virtual std::optional<domain::Book> GetBookByTitle(const std::string& title) const = 0;
    virtual std::vector<domain::Book> GetBooksByTitle(const std::string& title) const = 0;
    virtual std::vector<domain::Book> GetAuthorBooks(const std::string& author_id) const = 0;
};

}  // namespace app
