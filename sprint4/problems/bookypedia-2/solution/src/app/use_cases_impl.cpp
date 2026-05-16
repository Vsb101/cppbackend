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
        boost::algorithm::replace_all(tag, "  ", " ");
        boost::algorithm::replace_all(tag, "  ", " ");  // На случай тройных пробелов и т.д.
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

    // Проверяем, является ли ввод номером автора
    bool is_number = true;
    for (char c : author_input) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            is_number = false;
            break;
        }
    }

    if (is_number && !author_input.empty()) {
        // Выбор автора из списка по номеру
        auto authors = authors_->GetAll();
        int author_idx = std::stoi(author_input) - 1;
        if (author_idx < 0 || author_idx >= static_cast<int>(authors.size())) {
            throw std::invalid_argument("Invalid author number");
        }
        author_id = authors[author_idx].GetId();
    } else {
        // Поиск или создание автора по имени
        auto author_opt = authors_->LoadByName(author_input);
        if (author_opt) {
            author_id = author_opt->GetId();
        } else {
            // Создаем нового автора
            author_id = AuthorId::New();
            authors_->Save({author_id, author_input});
            author_added = true;
        }
    }

    auto normalized_tags = NormalizeTags(raw_tags);
    Book book{BookId::New(), author_id, title, publication_year, normalized_tags};
    books_->Save(book);
    
    // Сохраняем теги книги
    if (!normalized_tags.empty()) {
        books_->SetBookTags(book.GetId(), normalized_tags);
    }

    return {book, author_added};
}

std::vector<domain::Author> UseCasesImpl::GetAuthors() const {
    return authors_->GetAll();
}

std::vector<domain::Book> UseCasesImpl::GetBooks() const {
    auto all_books = books_->GetAll();
    std::vector<domain::Book> result;
    for (auto& book : all_books) {
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

bool UseCasesImpl::DeleteAuthor(const std::string& author_input) {
    AuthorId author_id;

    // Проверяем, является ли ввод номером автора
    bool is_number = true;
    for (char c : author_input) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            is_number = false;
            break;
        }
    }

    if (is_number && !author_input.empty()) {
        // Выбор автора из списка по номеру
        auto authors = authors_->GetAll();
        int author_idx = std::stoi(author_input) - 1;
        if (author_idx < 0 || author_idx >= static_cast<int>(authors.size())) {
            return false;
        }
        author_id = authors[author_idx].GetId();
    } else {
        // Поиск автора по имени
        auto author_opt = authors_->LoadByName(author_input);
        if (!author_opt) {
            return false;
        }
        author_id = author_opt->GetId();
    }

    // Удаляем все книги автора (и их теги автоматически через CASCADE)
    auto author_books = authors_->GetAuthorBooks(author_id);
    for (const auto& book : author_books) {
        books_->Delete(book.GetId());
    }

    // Удаляем автора
    authors_->Delete(author_id);
    return true;
}

bool UseCasesImpl::EditAuthor(const std::string& author_input, const std::string& new_name) {
    if (new_name.empty()) {
        return false;
    }

    AuthorId author_id;

    // Проверяем, является ли ввод номером автора
    bool is_number = true;
    for (char c : author_input) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            is_number = false;
            break;
        }
    }

    if (is_number && !author_input.empty()) {
        // Выбор автора из списка по номеру
        auto authors = authors_->GetAll();
        int author_idx = std::stoi(author_input) - 1;
        if (author_idx < 0 || author_idx >= static_cast<int>(authors.size())) {
            return false;
        }
        author_id = authors[author_idx].GetId();
    } else {
        // Поиск автора по имени
        auto author_opt = authors_->LoadByName(author_input);
        if (!author_opt) {
            return false;
        }
        author_id = author_opt->GetId();
    }

    // Проверяем, что автор всё ещё существует
    auto author_opt = authors_->LoadById(author_id);
    if (!author_opt) {
        return false;
    }

    authors_->Save({author_id, new_name});
    return true;
}

bool UseCasesImpl::DeleteBook(const std::string& book_input) {
    BookId book_id;

    // Проверяем, является ли ввод номером книги
    bool is_number = true;
    for (char c : book_input) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            is_number = false;
            break;
        }
    }

    if (is_number && !book_input.empty()) {
        // Выбор книги из списка по номеру
        auto books = books_->GetAll();
        int book_idx = std::stoi(book_input) - 1;
        if (book_idx < 0 || book_idx >= static_cast<int>(books.size())) {
            return false;
        }
        book_id = books[book_idx].GetId();
    } else {
        // Поиск книги по названию
        auto books = books_->GetByTitle(book_input);
        if (books.empty()) {
            return false;
        }
        if (books.size() > 1) {
            // Если несколько книг с таким названием, берём первую (это будет обработано в UI)
            book_id = books[0].GetId();
        } else {
            book_id = books[0].GetId();
        }
    }

    // Проверяем, что книга существует
    auto book_opt = books_->LoadById(book_id);
    if (!book_opt) {
        return false;
    }

    books_->Delete(book_id);
    return true;
}

bool UseCasesImpl::EditBook(
    const std::string& book_input,
    const std::string& new_title,
    int publication_year,
    const std::vector<std::string>& raw_tags) {
    
    BookId book_id;
    std::string current_title;
    int current_year;

    // Проверяем, является ли ввод номером книги
    bool is_number = true;
    for (char c : book_input) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            is_number = false;
            break;
        }
    }

    if (is_number && !book_input.empty()) {
        // Выбор книги из списка по номеру
        auto books = books_->GetAll();
        int book_idx = std::stoi(book_input) - 1;
        if (book_idx < 0 || book_idx >= static_cast<int>(books.size())) {
            return false;
        }
        book_id = books[book_idx].GetId();
        current_title = books[book_idx].GetTitle();
        current_year = books[book_idx].GetPublicationYear();
    } else {
        // Поиск книги по названию
        auto books = books_->GetByTitle(book_input);
        if (books.empty()) {
            return false;
        }
        if (books.size() > 1) {
            book_id = books[0].GetId();
            current_title = books[0].GetTitle();
            current_year = books[0].GetPublicationYear();
        } else {
            book_id = books[0].GetId();
            current_title = books[0].GetTitle();
            current_year = books[0].GetPublicationYear();
        }
    }

    // Проверяем, что книга всё ещё существует
    auto book_opt = books_->LoadById(book_id);
    if (!book_opt) {
        return false;
    }

    // Используем текущие значения, если новые не указаны
    std::string title_to_use = new_title.empty() ? current_title : new_title;
    int year_to_use = publication_year == 0 ? current_year : publication_year;

    auto normalized_tags = NormalizeTags(raw_tags);
    Book updated_book{book_id, book_opt->GetAuthorId(), title_to_use, year_to_use, normalized_tags};
    books_->Update(updated_book);
    
    // Обновляем теги книги
    books_->SetBookTags(book_id, normalized_tags);

    return true;
}

std::optional<domain::Book> UseCasesImpl::GetBookByTitle(const std::string& title) const {
    auto books = books_->GetByTitle(title);
    if (books.empty()) {
        return std::nullopt;
    }
    // Возвращаем первую книгу с тегами
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
