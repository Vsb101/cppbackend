#include "postgres.h"

#include <pqxx/result.hxx>
#include <pqxx/row.hxx>
#include <pqxx/zview.hxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2);
)"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAll() const {
    pqxx::read_transaction r{connection_};
    std::vector<domain::Author> result;
    pqxx::result rows = r.exec(
        R"(
SELECT id, name FROM authors ORDER BY name;
)"
    );
    for (size_t i = 0; i < rows.size(); ++i) {
        auto const& row = rows[i];
        result.emplace_back(
            domain::AuthorId::FromString(row[0].template as<std::string>()),
            row[1].template as<std::string>()
        );
    }
    return result;
}

std::optional<domain::Author> AuthorRepositoryImpl::LoadById(const domain::AuthorId& id) const {
    pqxx::read_transaction r{connection_};
    pqxx::result rows = r.exec_params(
        R"(
SELECT name FROM authors WHERE id = $1;
)"_zv,
        id.ToString()
    );
    if (!rows.empty()) {
        auto const& row = rows[0];
        return domain::Author{id, row[0].template as<std::string>()};
    }
    return std::nullopt;
}

std::optional<domain::Author> AuthorRepositoryImpl::LoadByName(const std::string& name) const {
    pqxx::read_transaction r{connection_};
    pqxx::result rows = r.exec_params(
        R"(
SELECT id, name FROM authors WHERE name = $1;
)"_zv,
        name
    );
    if (!rows.empty()) {
        auto const& row = rows[0];
        return domain::Author{
            domain::AuthorId::FromString(row[0].template as<std::string>()),
            row[1].template as<std::string>()
        };
    }
    return std::nullopt;
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4);
)"_zv,
        book.GetId().ToString(),
        book.GetAuthorId().ToString(),
        book.GetTitle(),
        book.GetPublicationYear()
    );
    work.commit();
}

std::vector<domain::Book> BookRepositoryImpl::GetAll() const {
    pqxx::read_transaction r{connection_};
    std::vector<domain::Book> result;
    pqxx::result rows = r.exec(
        R"(
SELECT id, author_id, title, publication_year FROM books ORDER BY title;
)"
    );
    for (size_t i = 0; i < rows.size(); ++i) {
        auto const& row = rows[i];
        result.emplace_back(
            domain::BookId::FromString(row[0].template as<std::string>()),
            domain::AuthorId::FromString(row[1].template as<std::string>()),
            row[2].template as<std::string>(),
            row[3].template as<int>()
        );
    }
    return result;
}

std::vector<domain::Book> BookRepositoryImpl::GetByAuthorId(const domain::AuthorId& author_id) const {
    pqxx::read_transaction r{connection_};
    std::vector<domain::Book> result;
    pqxx::result rows = r.exec_params(
        R"(
SELECT id, author_id, title, publication_year FROM books WHERE author_id = $1 ORDER BY publication_year, title;
)"_zv,
        author_id.ToString()
    );
    for (size_t i = 0; i < rows.size(); ++i) {
        auto const& row = rows[i];
        result.emplace_back(
            domain::BookId::FromString(row[0].template as<std::string>()),
            domain::AuthorId::FromString(row[1].template as<std::string>()),
            row[2].template as<std::string>(),
            row[3].template as<int>()
        );
    }
    return result;
}

std::optional<domain::Book> BookRepositoryImpl::LoadById(const domain::BookId& id) const {
    pqxx::read_transaction r{connection_};
    pqxx::result rows = r.exec_params(
        R"(
SELECT author_id, title, publication_year FROM books WHERE id = $1;
)"_zv,
        id.ToString()
    );
    if (!rows.empty()) {
        auto const& row = rows[0];
        return domain::Book{
            id,
            domain::AuthorId::FromString(row[0].template as<std::string>()),
            row[1].template as<std::string>(),
            row[2].template as<int>()
        };
    }
    return std::nullopt;
}

std::vector<domain::Book> BookRepositoryImpl::GetByTitle(const std::string& title) const {
    pqxx::read_transaction r{connection_};
    std::vector<domain::Book> result;
    pqxx::result rows = r.exec_params(
        R"(
SELECT id, author_id, title, publication_year FROM books WHERE title = $1 ORDER BY author_id, publication_year;
)"_zv,
        title
    );
    for (size_t i = 0; i < rows.size(); ++i) {
        auto const& row = rows[i];
        result.emplace_back(
            domain::BookId::FromString(row[0].template as<std::string>()),
            domain::AuthorId::FromString(row[1].template as<std::string>()),
            row[2].template as<std::string>(),
            row[3].template as<int>()
        );
    }
    return result;
}

void AuthorRepositoryImpl::Delete(const domain::AuthorId& id) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
DELETE FROM authors WHERE id = $1;
)"_zv,
        id.ToString());
    work.commit();
}

void AuthorRepositoryImpl::Update(const domain::Author& author) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
UPDATE authors SET name = $1 WHERE id = $2;
)"_zv,
        author.GetName(), author.GetId().ToString());
    work.commit();
}

std::vector<domain::Book> AuthorRepositoryImpl::GetAuthorBooks(const domain::AuthorId& author_id) const {
    pqxx::read_transaction r{connection_};
    std::vector<domain::Book> result;
    pqxx::result rows = r.exec_params(
        R"(
SELECT id, author_id, title, publication_year FROM books WHERE author_id = $1 ORDER BY publication_year, title;
)"_zv,
        author_id.ToString()
    );
    for (size_t i = 0; i < rows.size(); ++i) {
        auto const& row = rows[i];
        result.emplace_back(
            domain::BookId::FromString(row[0].template as<std::string>()),
            domain::AuthorId::FromString(row[1].template as<std::string>()),
            row[2].template as<std::string>(),
            row[3].template as<int>()
        );
    }
    return result;
}

void BookRepositoryImpl::Delete(const domain::BookId& id) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
DELETE FROM books WHERE id = $1;
)"_zv,
        id.ToString());
    work.commit();
}

void BookRepositoryImpl::Update(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
UPDATE books SET title = $1, publication_year = $2, author_id = $3 WHERE id = $4;
)"_zv,
        book.GetTitle(), book.GetPublicationYear(), book.GetAuthorId().ToString(), book.GetId().ToString());
    work.commit();
}

std::vector<std::string> BookRepositoryImpl::GetBookTags(const domain::BookId& book_id) const {
    pqxx::read_transaction r{connection_};
    std::vector<std::string> result;
    pqxx::result rows = r.exec_params(
        R"(
SELECT tag FROM book_tags WHERE book_id = $1 ORDER BY tag;
)"_zv,
        book_id.ToString()
    );
    for (size_t i = 0; i < rows.size(); ++i) {
        result.push_back(rows[i][0].template as<std::string>());
    }
    return result;
}

void BookRepositoryImpl::SetBookTags(const domain::BookId& book_id, const std::vector<std::string>& tags) {
    pqxx::work work{connection_};
    // Сначала удаляем все существующие теги книги
    work.exec_params(
        R"(
DELETE FROM book_tags WHERE book_id = $1;
)"_zv,
        book_id.ToString());
    // Затем добавляем новые теги
    for (const auto& tag : tags) {
        work.exec_params(
            R"(
INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);
)"_zv,
            book_id.ToString(), tag);
    }
    work.commit();
}

void BookRepositoryImpl::DeleteBookTags(const domain::BookId& book_id) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
DELETE FROM book_tags WHERE book_id = $1;
)"_zv,
        book_id.ToString());
    work.commit();
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);
    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL REFERENCES authors(id),
    title varchar(100) NOT NULL,
    publication_year int NOT NULL
);
)"_zv);
    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID NOT NULL REFERENCES books(id) ON DELETE CASCADE,
    tag varchar(30) NOT NULL,
    CONSTRAINT book_tag_pkey PRIMARY KEY (book_id, tag)
);
)"_zv);
    work.commit();
}

}  // namespace postgres