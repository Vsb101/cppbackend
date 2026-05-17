#include "postgres.h"

#include <pqxx/result>
#include <pqxx/row>
#include <pqxx/zview.hxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator""_zv;

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
    book_id UUID NOT NULL REFERENCES books(id),
    tag varchar(30) NOT NULL,
    CONSTRAINT book_tag_pkey PRIMARY KEY (book_id, tag)
);
)"_zv);

    work.exec(R"(
CREATE INDEX IF NOT EXISTS books_author_id_idx ON books(author_id);
CREATE INDEX IF NOT EXISTS books_title_idx ON books(title);
)"_zv);

    work.commit();
}

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    auto execute = [&](pqxx::transaction_base& tx) {
        tx.exec_params(
            "INSERT INTO authors (id, name) VALUES ($1, $2);"_zv,
            author.GetId().ToString(), author.GetName()
        );
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::work work{connection_}; 
        execute(work); 
        work.commit(); 
    }
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAll() const {
    std::vector<domain::Author> result;
    auto execute = [&](pqxx::transaction_base& tx) {
        pqxx::result rows = tx.exec("SELECT id, name FROM authors ORDER BY name;"_zv);
        result.reserve(rows.size());
        for (size_t i = 0; i < rows.size(); ++i) {
            result.emplace_back(
                domain::AuthorId::FromString(rows[i][0].as<std::string>()),
                rows[i][1].as<std::string>()
            );
        }
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::read_transaction r{connection_}; 
        execute(r); 
    }
    return result;
}

std::optional<domain::Author> AuthorRepositoryImpl::LoadById(const domain::AuthorId& id) const {
    std::optional<domain::Author> result;
    auto execute = [&](pqxx::transaction_base& tx) {
        pqxx::result rows = tx.exec_params("SELECT name FROM authors WHERE id = $1;"_zv, id.ToString());
        if (!rows.empty()) {
            result = domain::Author{id, rows[0][0].as<std::string>()};
        }
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::read_transaction r{connection_}; 
        execute(r); 
    }
    return result;
}

std::optional<domain::Author> AuthorRepositoryImpl::LoadByName(const std::string& name) const {
    std::optional<domain::Author> result;
    auto execute = [&](pqxx::transaction_base& tx) {
        pqxx::result rows = tx.exec_params("SELECT id FROM authors WHERE name = $1;"_zv, name);
        if (!rows.empty()) {
            result = domain::Author{
                domain::AuthorId::FromString(rows[0][0].as<std::string>()),
                name
            };
        }
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::read_transaction r{connection_}; 
        execute(r); 
    }
    return result;
}

void AuthorRepositoryImpl::Delete(const domain::AuthorId& id) {
    auto execute = [&](pqxx::transaction_base& tx) {
        tx.exec_params("DELETE FROM authors WHERE id = $1;"_zv, id.ToString());
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::work work{connection_}; 
        execute(work); 
        work.commit(); 
    }
}

void AuthorRepositoryImpl::Update(const domain::Author& author) {
    auto execute = [&](pqxx::transaction_base& tx) {
        tx.exec_params(
            "UPDATE authors SET name = $1 WHERE id = $2;"_zv,
            author.GetName(), author.GetId().ToString()
        );
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::work work{connection_}; 
        execute(work); 
        work.commit(); 
    }
}

std::vector<domain::Book> AuthorRepositoryImpl::GetAuthorBooks(const domain::AuthorId& author_id) const {
    std::vector<domain::Book> result;
    auto execute = [&](pqxx::transaction_base& tx) {
        pqxx::result rows = tx.exec_params(
            "SELECT id, title, publication_year FROM books WHERE author_id = $1 ORDER BY publication_year, title;"_zv,
            author_id.ToString()
        );
        result.reserve(rows.size());
        for (size_t i = 0; i < rows.size(); ++i) {
            result.emplace_back(
                domain::BookId::FromString(rows[i][0].as<std::string>()),
                author_id,
                rows[i][1].as<std::string>(),
                rows[i][2].as<int>()
            );
        }
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::read_transaction r{connection_}; 
        execute(r); 
    }
    return result;
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    auto execute = [&](pqxx::transaction_base& tx) {
        tx.exec_params(
            "INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4);"_zv,
            book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetPublicationYear()
        );
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::work work{connection_}; 
        execute(work); 
        work.commit(); 
    }
}

std::vector<domain::Book> BookRepositoryImpl::GetAll() const {
    std::vector<domain::Book> result;
    auto execute = [&](pqxx::transaction_base& tx) {
        pqxx::result rows = tx.exec(
            R"(
SELECT b.id, b.author_id, b.title, b.publication_year
FROM books b 
JOIN authors a ON b.author_id = a.id
ORDER BY b.title, a.name, b.publication_year, b.id;
)"_zv
        );

        result.reserve(rows.size());
        for (size_t i = 0; i < rows.size(); ++i) {
            result.emplace_back(
                domain::BookId::FromString(rows[i][0].as<std::string>()),
                domain::AuthorId::FromString(rows[i][1].as<std::string>()),
                rows[i][2].as<std::string>(),
                rows[i][3].as<int>()
            );
        }
    };

    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::read_transaction r{connection_}; 
        execute(r); 
    }
    return result;
}

std::vector<domain::Book> BookRepositoryImpl::GetByAuthorId(const domain::AuthorId& author_id) const {
    std::vector<domain::Book> result;
    auto execute = [&](pqxx::transaction_base& tx) {
        pqxx::result rows = tx.exec_params(
            "SELECT id, title, publication_year FROM books WHERE author_id = $1 ORDER BY publication_year, title;"_zv,
            author_id.ToString()
        );
        result.reserve(rows.size());
        for (size_t i = 0; i < rows.size(); ++i) {
            result.emplace_back(
                domain::BookId::FromString(rows[i][0].as<std::string>()),
                author_id,
                rows[i][1].as<std::string>(),
                rows[i][2].as<int>()
            );
        }
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::read_transaction r{connection_}; 
        execute(r); 
    }
    return result;
}

std::optional<domain::Book> BookRepositoryImpl::LoadById(const domain::BookId& id) const {
    std::optional<domain::Book> result;
    auto execute = [&](pqxx::transaction_base& tx) {
        pqxx::result rows = tx.exec_params("SELECT author_id, title, publication_year FROM books WHERE id = $1;"_zv, id.ToString());
        if (!rows.empty()) {
            result = domain::Book{
                id,
                domain::AuthorId::FromString(rows[0][0].as<std::string>()),
                rows[0][1].as<std::string>(),
                rows[0][2].as<int>()
            };
        }
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::read_transaction r{connection_}; 
        execute(r); 
    }
    return result;
}

std::vector<domain::Book> BookRepositoryImpl::GetByTitle(const std::string& title) const {
    std::vector<domain::Book> result;
    auto execute = [&](pqxx::transaction_base& tx) {
        pqxx::result rows = tx.exec_params(
            "SELECT id, author_id, publication_year FROM books WHERE title = $1 ORDER BY author_id, publication_year;"_zv,
            title
        );
        result.reserve(rows.size());
        for (size_t i = 0; i < rows.size(); ++i) {
            result.emplace_back(
                domain::BookId::FromString(rows[i][0].as<std::string>()),
                domain::AuthorId::FromString(rows[i][1].as<std::string>()),
                title,
                rows[i][2].as<int>()
            );
        }
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::read_transaction r{connection_}; 
        execute(r); 
    }
    return result;
}

void BookRepositoryImpl::Delete(const domain::BookId& id) {
    auto execute = [&](pqxx::transaction_base& tx) {
        tx.exec_params("DELETE FROM books WHERE id = $1;"_zv, id.ToString());
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::work work{connection_}; 
        execute(work); 
        work.commit(); 
    }
}

void BookRepositoryImpl::Update(const domain::Book& book) {
    auto execute = [&](pqxx::transaction_base& tx) {
        tx.exec_params(
            "UPDATE books SET title = $1, publication_year = $2, author_id = $3 WHERE id = $4;"_zv,
            book.GetTitle(), book.GetPublicationYear(), book.GetAuthorId().ToString(), book.GetId().ToString()
        );
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::work work{connection_}; 
        execute(work); 
        work.commit(); 
    }
}

std::vector<std::string> BookRepositoryImpl::GetBookTags(const domain::BookId& book_id) const {
    std::vector<std::string> result;
    auto execute = [&](pqxx::transaction_base& tx) {
        pqxx::result rows = tx.exec_params("SELECT tag FROM book_tags WHERE book_id = $1 ORDER BY tag;"_zv, book_id.ToString());
        result.reserve(rows.size());
        for (size_t i = 0; i < rows.size(); ++i) {
            result.push_back(rows[i][0].as<std::string>());
        }
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::read_transaction r{connection_}; 
        execute(r); 
    }
    return result;
}

void BookRepositoryImpl::SetBookTags(const domain::BookId& book_id, const std::vector<std::string>& tags) {
    auto execute = [&](pqxx::transaction_base& tx) {
        tx.exec_params("DELETE FROM book_tags WHERE book_id = $1;"_zv, book_id.ToString());
        for (const auto& tag : tags) {
            tx.exec_params("INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);"_zv, book_id.ToString(), tag);
        }
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::work work{connection_}; 
        execute(work); 
        work.commit(); 
    }
}

void BookRepositoryImpl::DeleteBookTags(const domain::BookId& book_id) {
    auto execute = [&](pqxx::transaction_base& tx) {
        tx.exec_params("DELETE FROM book_tags WHERE book_id = $1;"_zv, book_id.ToString());
    };
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::work work{connection_}; 
        execute(work); 
        work.commit(); 
    }
}

std::unordered_map<std::string, std::vector<std::string>> BookRepositoryImpl::GetBooksTagsBatch(
    const std::vector<std::string>& book_ids) const {
    
    std::unordered_map<std::string, std::vector<std::string>> result;
    if (book_ids.empty()) return result;
    
    auto execute = [&](pqxx::transaction_base& tx) {
        std::string query = "SELECT book_id, tag FROM book_tags WHERE book_id = ANY($1) ORDER BY book_id, tag";
        
        pqxx::result rows = tx.exec_params(query, book_ids);
        
        for (const auto& row : rows) {
            std::string book_id = row[0].as<std::string>();
            std::string tag = row[1].as<std::string>();
            result[book_id].push_back(std::move(tag));
        }
    };
    
    if (current_tx_) { 
        execute(*current_tx_);
    } else { 
        pqxx::read_transaction r{connection_}; 
        execute(r); 
    }
    
    return result;
}

}  // namespace postgres