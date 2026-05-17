#pragma once
#include "../domain/author_fwd.h"
#include "../ui/struct_info.h"
#include <vector>
#include <optional>

#include <string>

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(const std::string& autor_id, const std::string& autor_name
        , const std::string& title, const int publication_year, const std::vector<std::string>& tags) = 0;
    virtual void EditAuthor(const info::AuthorInfo& author) const = 0;
    virtual info::Authors GetAuthors() const = 0;
    virtual info::Books GetBooks() const = 0;
    virtual info::BookInfo GetBook(const std::string& book_id) const = 0;
    virtual void DeleteBook(const std::string& book_id) const = 0;
    virtual void EditBook(const info::BookInfo& book) const = 0;
    virtual info::Books GetAuthorBooks(const std::string& author_id) const = 0;
    virtual void DeleteAuthor(const std::string& author_id) const = 0;
    virtual std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const = 0;
    virtual info::Books GetBooksByTitle(const std::string& book_title) const = 0;
protected:
    ~UseCases() = default;
};

}  // namespace app
