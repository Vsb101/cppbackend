#include "use_cases_impl.h"

#include "../domain/author.h"
#include <algorithm>
#include <sstream>
#include <set>
#include <boost/algorithm/string.hpp>

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
    
    // Превентивно проверяем уникальность, чтобы тесты full_db падали корректно
    if (authors_->LoadByName(name).has_value()) {
        throw std::runtime_error("Author already exists");
    }
    
    authors_->Save({AuthorId::New(), name});
}

static std::vector<std::string> NormalizeTags(const std::vector<std::string>& raw_tags) {
    std::vector<std::string> result;
    std::set<std::string> seen;

    for (const auto& raw_tag : raw_tags) {
        std::string tag = raw_tag;
        // Удаляем пробелы в начале и в конце
        boost::algorithm::trim(tag);
        // Заменяем множественные пробелы на один
        while (tag.find("  ") != std::string::npos) {
            boost::algorithm::replace_all(tag, "  ", " ");
        }

        // Пропускаем пустые теги
        if (tag.empty()) {
            continue;
        }

        // Пропускаем дубликаты
        if (seen.find(tag) == seen.end()) {
            seen.insert(tag);
            result.push_back(tag);
        }
    }

    // Сортируем теги по алфавиту для предсказуемого порядка (согласно ТЗ)
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

    AuthorId author_id;
    bool author_added = false;

    // Защита: очистим имя от случайных пробелов по краям
    std::string clean_author_name = author_input;
    boost::algorithm::trim(clean_author_name);

    // Больше никаких проверок на число! Работаем строго с именем автора.
    auto author_opt = authors_->LoadByName(clean_author_name);
    if (author_opt) {
        author_id = author_opt->GetId();
    } else {
        author_id = AuthorId::New();
        authors_->Save({author_id, clean_author_name});
        author_added = true;
    }

    auto normalized_tags = NormalizeTags(raw_tags);
    Book book{BookId::New(), author_id, title, publication_year, normalized_tags};
    books_->Save(book);
    
    if (!normalized_tags.empty()) {
        books_->SetBookTags(book.GetId(), normalized_tags);
    }

    return {book, author_added};
}

std::vector<domain::Author> UseCasesImpl::GetAuthors() const {
    return authors_->GetAll();
}

std::vector<domain::Book> UseCasesImpl::GetBooks() const {
    // Оптимизированный вызов: теперь GetAll() делает LEFT JOIN и возвращает книги сразу с тегами
    return books_->GetAll();
}

bool UseCasesImpl::DeleteAuthor(const std::string& author_input) {
    AuthorId author_id;
    bool id_found = false;

    bool is_number = !author_input.empty();
    for (char c : author_input) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            is_number = false;
            break;
        }
    }

    if (is_number) {
        auto authors = authors_->GetAll();
        std::sort(authors.begin(), authors.end(), [](const auto& a, const auto& b) {
            return a.GetName() < b.GetName();
        });
        
        int author_idx = std::stoi(author_input) - 1;
        if (author_idx >= 0 && author_idx < static_cast<int>(authors.size())) {
            author_id = authors[author_idx].GetId();
            id_found = true;
        }
    }

    if (!id_found) {
        if (author_input.length() == 36 && author_input.find('-') != std::string::npos) {
            try {
                auto target_id = domain::AuthorId::FromString(author_input);
                if (authors_->LoadById(target_id).has_value()) {
                    author_id = target_id;
                    id_found = true;
                }
            } catch (...) {}
        }
        
        if (!id_found) {
            auto author_opt = authors_->LoadByName(author_input);
            if (!author_opt) {
                return false; 
            }
            author_id = author_opt->GetId();
        }
    }

    if (!authors_->LoadById(author_id).has_value()) {
        return false;
    }

    // Ручной каскад: сначала очищаем все книги автора и их теги
    auto author_books = books_->GetByAuthorId(author_id);
    for (const auto& book : author_books) {
        books_->DeleteBookTags(book.GetId());
        books_->Delete(book.GetId());
    }

    // После этого удаление автора безопасно
    authors_->Delete(author_id);
    return true;
}

bool UseCasesImpl::EditAuthor(const std::string& author_input, const std::string& new_name) {
    if (new_name.empty()) {
        return false;
    }

    AuthorId author_id;
    bool id_found = false;

    bool is_number = !author_input.empty();
    for (char c : author_input) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            is_number = false;
            break;
        }
    }

    if (is_number) {
        auto authors = authors_->GetAll();
        std::sort(authors.begin(), authors.end(), [](const auto& a, const auto& b) {
            return a.GetName() < b.GetName();
        });
        int author_idx = std::stoi(author_input) - 1;
        if (author_idx >= 0 && author_idx < static_cast<int>(authors.size())) {
            author_id = authors[author_idx].GetId();
            id_found = true;
        }
    }

    if (!id_found) {
        auto author_opt = authors_->LoadByName(author_input);
        if (!author_opt) {
            return false;
        }
        author_id = author_opt->GetId();
    }

    auto author_opt = authors_->LoadById(author_id);
    if (!author_opt) {
        return false;
    }

    authors_->Update({author_id, new_name});
    return true;
}

bool UseCasesImpl::DeleteBook(const std::string& book_id_str) {
    try {
        auto book_id = BookId::FromString(book_id_str);
        if (!books_->LoadById(book_id)) {
            return false;
        }

        books_->DeleteBookTags(book_id);
        books_->Delete(book_id);
        return true;
    } catch (...) {
        return false;
    }
}

bool UseCasesImpl::EditBook(
    const std::string& book_id_str,
    const std::string& new_title,
    int publication_year,
    const std::vector<std::string>& raw_tags) {
    
    try {
        auto book_id = BookId::FromString(book_id_str);
        auto book_opt = books_->LoadById(book_id);
        if (!book_opt) {
            return false;
        }

        std::string title_to_use = new_title.empty() ? book_opt->GetTitle() : new_title;
        int year_to_use = publication_year == 0 ? book_opt->GetPublicationYear() : publication_year;

        auto normalized_tags = NormalizeTags(raw_tags);
        books_->SetBookTags(book_id, normalized_tags);

        Book updated_book{book_id, book_opt->GetAuthorId(), std::move(title_to_use), year_to_use, std::move(normalized_tags)};
        books_->Update(updated_book);
        
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<domain::Book> UseCasesImpl::GetBookByTitle(const std::string& title) const {
    auto books = books_->GetByTitle(title);
    if (books.empty()) {
        return std::nullopt;
    }
    auto book = books[0];
    auto tags = books_->GetBookTags(book.GetId());
    return Book{book.GetId(), book.GetAuthorId(), book.GetTitle(), book.GetPublicationYear(), tags};
}

std::vector<domain::Book> UseCasesImpl::GetBooksByTitle(const std::string& title) const {
    auto books = books_->GetByTitle(title);
    std::vector<domain::Book> result;
    for (auto& book : books) {
        auto tags = books_->GetBookTags(book.GetId());
        result.emplace_back(
            book.GetId(),
            book.GetAuthorId(),
            book.GetTitle(),
            book.GetPublicationYear(),
            std::move(tags)
        );
    }
    return result;
}

std::vector<domain::Book> UseCasesImpl::GetAuthorBooks(const std::string& author_id_str) const {
    auto author_books = books_->GetByAuthorId(AuthorId::FromString(author_id_str));
    std::vector<domain::Book> result;
    for (auto& book : author_books) {
        auto tags = books_->GetBookTags(book.GetId());
        result.emplace_back(
            book.GetId(),
            book.GetAuthorId(),
            book.GetTitle(),
            book.GetPublicationYear(),
            std::move(tags)
        );
    }
    return result;
}

}  // namespace app
