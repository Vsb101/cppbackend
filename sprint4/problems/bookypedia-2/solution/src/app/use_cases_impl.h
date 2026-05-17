#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "unit_of_work.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(UnitOfWorkFactory* uow_factory)
        : uow_factory_{uow_factory} 
        {
    }

    void AddAuthor(const std::string& name) override;
    void AddBook(const std::string& autor_id, const std::string& autor_name
        , const std::string& title, const int publication_year, const std::vector<std::string>& tags) override;
    void EditAuthor(const info::AuthorInfo& author) const;
    info::Authors GetAuthors() const override;
    info::Books GetBooks() const override;
    info::BookInfo GetBook(const std::string& book_id) const override;
    void DeleteBook(const std::string& book_id) const override;
    void EditBook(const info::BookInfo& book) const override;
    info::Books GetAuthorBooks(const std::string& author_id) const override;
    void DeleteAuthor(const std::string& author_id) const override;
    std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const override;
    info::Books GetBooksByTitle(const std::string& book_title) const;
private:
    // domain::AuthorRepository& authors_;
    // domain::BookRepository& books_;
    UnitOfWorkFactory* uow_factory_;
};

}  // namespace app
