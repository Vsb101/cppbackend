#pragma once
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

#include "../util/tagged_uuid.h"

namespace domain {

namespace detail {
struct AuthorTag {};
struct BookTag {};
struct TagTag {};
}  // namespace detail

using AuthorId = util::TaggedUUID<detail::AuthorTag>;
using BookId = util::TaggedUUID<detail::BookTag>;
using TagId = util::TaggedUUID<detail::TagTag>;

class Author {
public:
    Author(AuthorId id, std::string name)
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const AuthorId& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

private:
    AuthorId id_;
    std::string name_;
};

class Book {
public:
    Book(BookId id, AuthorId author_id, std::string title, int publication_year)
        : id_(std::move(id))
        , author_id_(std::move(author_id))
        , title_(std::move(title))
        , publication_year_(publication_year) {
    }

    const BookId& GetId() const noexcept {
        return id_;
    }

    const AuthorId& GetAuthorId() const noexcept {
        return author_id_;
    }

    const std::string& GetTitle() const noexcept {
        return title_;
    }

    int GetPublicationYear() const noexcept {
        return publication_year_;
    }

private:
    BookId id_;
    AuthorId author_id_;
    std::string title_;
    int publication_year_;
};

class AuthorRepository {
public:
    virtual ~AuthorRepository() = default; 

    virtual void Save(const Author& author) = 0;
    virtual std::vector<Author> GetAll() const = 0;
    virtual std::optional<Author> LoadById(const AuthorId& id) const = 0;
    virtual std::optional<Author> LoadByName(const std::string& name) const = 0;
    virtual void Delete(const AuthorId& id) = 0;
    virtual void Update(const Author& author) = 0;
    virtual std::vector<Book> GetAuthorBooks(const AuthorId& author_id) const = 0;
};

class BookRepository {
public:
    virtual ~BookRepository() = default;

    virtual void Save(const Book& book) = 0;
    virtual std::vector<Book> GetAll() const = 0;
    virtual std::vector<Book> GetByAuthorId(const AuthorId& author_id) const = 0;
    virtual std::optional<Book> LoadById(const BookId& id) const = 0;
    virtual std::vector<Book> GetByTitle(const std::string& title) const = 0;
    virtual void Delete(const BookId& id) = 0;
    virtual void Update(const Book& book) = 0;
    virtual std::vector<std::string> GetBookTags(const BookId& book_id) const = 0;
    virtual void SetBookTags(const BookId& book_id, const std::vector<std::string>& tags) = 0;
    virtual void DeleteBookTags(const BookId& book_id) = 0;
    virtual std::unordered_map<std::string, std::vector<std::string>> GetBooksTagsBatch(
        const std::vector<std::string>& book_ids) const = 0;
};

}  // namespace domain