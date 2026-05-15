#include "use_cases_impl.h"

#include "../domain/author.h"

namespace app {
using namespace domain;

UseCasesImpl::UseCasesImpl(
    std::shared_ptr<domain::AuthorRepository> authors,
    std::shared_ptr<domain::BookRepository> books)
    : authors_(std::move(authors))
    , books_(std::move(books)) {
}

void UseCasesImpl::AddAuthor(const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("Author name cannot be empty");
    }
    authors_->Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(const domain::Book& book) {
    if (book.GetTitle().empty()) {
        throw std::invalid_argument("Book title cannot be empty");
    }
    books_->Save(book);
}

std::vector<domain::Author> UseCasesImpl::GetAuthors() const {
    return authors_->GetAll();
}

std::vector<domain::Book> UseCasesImpl::GetBooks() const {
    return books_->GetAll();
}

std::vector<domain::Book> UseCasesImpl::GetAuthorBooks(const std::string& author_id) const {
    return books_->GetByAuthorId(AuthorId::FromString(author_id));
}

}  // namespace app
