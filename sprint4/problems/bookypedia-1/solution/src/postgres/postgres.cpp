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
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
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

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET title=$3, publication_year=$4;
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
    work.commit();
}

}  // namespace postgres