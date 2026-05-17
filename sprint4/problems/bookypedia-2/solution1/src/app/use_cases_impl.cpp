#include "use_cases_impl.h"

#include "../domain/author.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <stdexcept>
#include <boost/algorithm/string.hpp>

namespace app {

using namespace domain;

UseCasesImpl::UseCasesImpl(
    postgres::Database& db,
    std::shared_ptr<domain::AuthorRepository> authors,
    std::shared_ptr<domain::BookRepository> books)
    : db_(&db)
    , authors_(std::move(authors))
    , books_(std::move(books)) {
}

UseCasesImpl::UseCasesImpl(
    std::shared_ptr<domain::AuthorRepository> authors,
    std::shared_ptr<domain::BookRepository> books)
    : db_(nullptr)
    , authors_(std::move(authors))
    , books_(std::move(books)) {
}

void UseCasesImpl::AddAuthor(const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("Author name cannot be empty");
    }
    
    ExecuteInTransaction([&]() {
        if (authors_->LoadByName(name).has_value()) {
            throw std::runtime_error("Author already exists");
        }
        authors_->Save({AuthorId::New(), name});
    });
}

static std::vector<std::string> NormalizeTags(const std::vector<std::string>& raw_tags) {
    std::unordered_set<std::string> seen;
    seen.reserve(raw_tags.size());

    for (const auto& raw_tag : raw_tags) {
        std::string tag = raw_tag;
        boost::algorithm::trim(tag);
        
        // Быстрая замена множественных пробелов
        size_t pos = 0;
        while ((pos = tag.find("  ", pos)) != std::string::npos) {
            tag.replace(pos, 2, " ");
        }

        if (!tag.empty()) {
            seen.insert(std::move(tag));
        }
    }

    std::vector<std::string> result(seen.begin(), seen.end());
    std::sort(result.begin(), result.end());
    return result;
}

AddBookResult UseCasesImpl::AddBookWithAuthorSelection(
    const std::string& title,
    int publication_year,
    const std::string& author_input,
    const std::vector<std::string>& raw_tags) {
    
    if (title.empty()) {
        throw std::invalid_argument("Book title cannot be empty");
    }

    std::optional<domain::Book> final_book; 
    bool author_added = false;

    ExecuteInTransaction([&]() {
        AuthorId author_id;
        
        bool is_number = !author_input.empty() && 
                         std::all_of(author_input.begin(), author_input.end(), 
                                    [](unsigned char c) { return std::isdigit(c); });
        
        bool is_uuid = author_input.length() == 36 && 
                       author_input.find('-') != std::string::npos;

        if (is_number) {
            auto authors = authors_->GetAll();
            int author_idx = std::stoi(author_input) - 1;
            if (author_idx < 0 || author_idx >= static_cast<int>(authors.size())) {
                throw std::invalid_argument("Invalid author number");
            }
            author_id = authors[author_idx].GetId();
        } 
        else if (is_uuid) {
            auto author_opt = authors_->LoadById(AuthorId::FromString(author_input));
            if (!author_opt) {
                throw std::runtime_error("Author not found by UUID");
            }
            author_id = author_opt->GetId();
        }
        else {
            std::string clean_author_name = author_input;
            boost::algorithm::trim(clean_author_name);

            auto author_opt = authors_->LoadByName(clean_author_name);
            if (author_opt) {
                author_id = author_opt->GetId();
            } else {
                author_id = AuthorId::New();
                authors_->Save({author_id, clean_author_name});
                author_added = true;
            }
        }

        auto normalized_tags = NormalizeTags(raw_tags);
        BookId book_id = BookId::New();
        
        final_book.emplace(book_id, author_id, title, publication_year);
        books_->Save(*final_book);
        
        if (!normalized_tags.empty()) {
            books_->SetBookTags(book_id, normalized_tags);
        }
    });

    return {*final_book, author_added};
}

std::vector<domain::Author> UseCasesImpl::GetAuthors() const {
    return authors_->GetAll();
}

std::vector<domain::Book> UseCasesImpl::GetBooks() const {
    return books_->GetAll();
}

bool UseCasesImpl::DeleteAuthor(const std::string& author_input) {
    bool is_deleted = false;
    ExecuteInTransaction([&]() {
        AuthorId author_id;
        
        auto author_by_name = authors_->LoadByName(author_input);
        if (author_by_name) {
            author_id = author_by_name->GetId();
        } 
        else {
            bool is_number = !author_input.empty() && 
                            std::all_of(author_input.begin(), author_input.end(),
                                       [](unsigned char c) { return std::isdigit(c); });
            
            if (is_number) {
                auto authors = authors_->GetAll();
                int author_idx = std::stoi(author_input) - 1;
                if (author_idx >= 0 && author_idx < static_cast<int>(authors.size())) {
                    author_id = authors[author_idx].GetId();
                } else {
                    return;
                }
            } else {
                return;
            }
        }

        auto author_books = books_->GetByAuthorId(author_id);
        for (const auto& book : author_books) {
            books_->DeleteBookTags(book.GetId());
            books_->Delete(book.GetId());
        }

        authors_->Delete(author_id);
        is_deleted = true;
    });
    return is_deleted;
}

bool UseCasesImpl::EditAuthor(const std::string& author_input, const std::string& new_name) {
    if (new_name.empty()) return false;
    
    bool is_edited = false;
    ExecuteInTransaction([&]() {
        AuthorId author_id;
        
        auto author_by_name = authors_->LoadByName(author_input);
        if (author_by_name) {
            author_id = author_by_name->GetId();
        } 
        else {
            bool is_number = !author_input.empty() && 
                            std::all_of(author_input.begin(), author_input.end(),
                                       [](unsigned char c) { return std::isdigit(c); });
            
            if (is_number) {
                auto authors = authors_->GetAll();
                int author_idx = std::stoi(author_input) - 1;
                if (author_idx >= 0 && author_idx < static_cast<int>(authors.size())) {
                    author_id = authors[author_idx].GetId();
                } else {
                    return;
                }
            } else {
                return;
            }
        }

        if (!authors_->LoadById(author_id)) return;

        authors_->Update({author_id, new_name});
        is_edited = true;
    });
    return is_edited;
}

bool UseCasesImpl::DeleteBook(const std::string& book_id_str) {
    bool is_deleted = false;
    ExecuteInTransaction([&]() {
        try {
            auto book_id = BookId::FromString(book_id_str);
            if (!books_->LoadById(book_id)) {
                return;
            }

            books_->DeleteBookTags(book_id);
            books_->Delete(book_id);
            is_deleted = true;
        } catch (...) {
            return;
        }
    });
    return is_deleted;
}

bool UseCasesImpl::EditBook(
    const std::string& book_id_str,
    const std::string& new_title,
    int publication_year,
    const std::vector<std::string>& raw_tags) {
    
    bool is_edited = false;
    ExecuteInTransaction([&]() {
        try {
            auto book_id = BookId::FromString(book_id_str);
            auto book_opt = books_->LoadById(book_id);
            if (!book_opt) {
                return;
            }

            std::string title_to_use = new_title.empty() ? book_opt->GetTitle() : new_title;
            int year_to_use = publication_year == 0 ? book_opt->GetPublicationYear() : publication_year;

            auto normalized_tags = NormalizeTags(raw_tags);
            books_->SetBookTags(book_id, normalized_tags);

            Book updated_book{book_id, book_opt->GetAuthorId(), std::move(title_to_use), year_to_use};
            books_->Update(updated_book);
            
            is_edited = true;
        } catch (...) {
            return;
        }
    });
    return is_edited;
}

std::optional<domain::Book> UseCasesImpl::GetBookByTitle(const std::string& title) const {
    auto books = books_->GetByTitle(title);
    if (books.empty()) {
        return std::nullopt;
    }
    return books[0];
}

std::vector<domain::Book> UseCasesImpl::GetBooksByTitle(const std::string& title) const {
    return books_->GetByTitle(title);
}

std::vector<domain::Book> UseCasesImpl::GetAuthorBooks(const std::string& author_id_str) const {
    try {
        return books_->GetByAuthorId(AuthorId::FromString(author_id_str));
    } catch (...) {
        return {};
    }
}

std::vector<std::string> UseCasesImpl::GetBookTags(const std::string& book_id_str) const {
    try {
        auto book_id = BookId::FromString(book_id_str);
        return books_->GetBookTags(book_id);
    } catch (...) {
        return {};
    }
}

std::unordered_map<std::string, std::vector<std::string>> UseCasesImpl::GetBooksTagsBatch(
    const std::vector<std::string>& book_ids) const {
    
    std::unordered_map<std::string, std::vector<std::string>> result;
    ExecuteInTransaction([&]() {
        result = books_->GetBooksTagsBatch(book_ids);
    });
    return result;
}

}  // namespace app