#pragma once

#include "use_cases.h"
#include "../domain/author_fwd.h"
#include <memory>

namespace app {

class UseCasesImpl : public UseCases {
public:
    UseCasesImpl(
        std::shared_ptr<domain::AuthorRepository> authors,
        std::shared_ptr<domain::BookRepository> books);

    void AddAuthor(const std::string& name) override;
    void AddBook(const domain::Book& book) override;
    std::vector<domain::Author> GetAuthors() const;
    std::vector<domain::Book> GetBooks() const;
    std::vector<domain::Book> GetAuthorBooks(const std::string& author_id) const;

private:
    std::shared_ptr<domain::AuthorRepository> authors_;
    std::shared_ptr<domain::BookRepository> books_;
};

}  // namespace app
