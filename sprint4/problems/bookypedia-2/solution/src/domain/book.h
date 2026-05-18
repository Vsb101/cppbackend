#pragma once
#include <string>

#include "../util/tagged_uuid.h"
#include "author.h"

namespace domain {

namespace detail {

// Маркер для типобезопасного BookId
struct BookTag {};

}  // namespace detail

// Типобезопасный идентификатор книги
using BookId = util::TaggedUUID<detail::BookTag>;

// Сущность "Книга"
class Book {
public:
    Book(BookId id, AuthorId author_id, std::string title, int publication_year)
        : id_(std::move(id))
        , author_id_(std::move(author_id))
        , title_(std::move(title)) 
        , publication_year_(publication_year) {
    }

    // Получение идентификатора книги
    const BookId& GetId() const noexcept {
        return id_;
    }

    // Получение идентификатора автора
    const AuthorId& GetAuthorId() const noexcept {
        return author_id_;
    }

    // Получение названия книги
    const std::string& GetTitle() const noexcept {
        return title_;
    }

    // Получение года публикации
    const int GetPublicationYear() const noexcept {
        return publication_year_;
    }

private:
    BookId id_;
    AuthorId author_id_;
    std::string title_;
    int publication_year_;
};

// Репозиторий для работы с книгами
class BookRepository {
public:
    // Сохранение книги (добавление или обновление)
    virtual void Save(const Book& book) = 0;
    // Удаление всех книг автора (при удалении автора)
    virtual void DeleteBooks(const std::string& author_id) = 0;
    // Удаление одной книги
    virtual void DeleteBook(const std::string& book_id) = 0;
    // Редактирование информации о книге
    virtual void EditBook(const info::BookInfo& book) = 0; 
    // Получение списка всех книг
    virtual info::Books GetBooks() const = 0;
    // Получение книги по ID
    virtual info::BookInfo GetBook(const std::string& book_id) const = 0;
    // Получение книг автора
    virtual info::Books GetBooksByAuthor(const std::string& author_id) const = 0;
    // Поиск книг по названию
    virtual info::Books GetBooksByTitle(const std::string& book_title) const = 0;

protected:
    ~BookRepository() = default;
};

}  // namespace domain
