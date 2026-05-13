#include "book_repository.hpp"
#include <string_view>

using pqxx::operator""_zv;
using namespace std::literals; // Для использования "ISBN"sv и т.д.

BookRepository::BookRepository(pqxx::connection& conn) : conn_(conn) {
    {
        pqxx::work w(conn_);
        w.exec(
            "CREATE TABLE IF NOT EXISTS books ("
            "id SERIAL PRIMARY KEY, "
            "title varchar(100) NOT NULL, "
            "author varchar(100) NOT NULL, "
            "year integer NOT NULL, "
            "ISBN char(13));"_zv);
        w.exec(
            "CREATE UNIQUE INDEX IF NOT EXISTS idx_books_isbn ON books (ISBN) WHERE ISBN IS NOT NULL;"_zv);
        w.commit();
    }

    conn_.prepare(kAddBook, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4);"_zv);
    conn_.prepare(kAddBookNullIsbn, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, NULL);"_zv);
    conn_.prepare(kAllBooks, "SELECT id, title, author, year, ISBN FROM books ORDER BY year DESC, title ASC, author ASC, ISBN ASC;"_zv);
}

bool BookRepository::Add(const Book& book) {
    try {
        pqxx::work w(conn_);
        if (!book.isbn) {
            w.exec_prepared(kAddBookNullIsbn, book.title, book.author, book.year);
        } else {
            w.exec_prepared(kAddBook, book.title, book.author, book.year, *book.isbn);
        }
        w.commit();
        return true;
    } catch (const pqxx::sql_error&) {
        return false;
    }
}

std::vector<Book> BookRepository::GetAll() {
    pqxx::read_transaction r(conn_);
    pqxx::result rows = r.exec_prepared(kAllBooks);
    
    std::vector<Book> books;
    books.reserve(rows.size());

    for (const auto& row : rows) {
        // C++20: Используем назначенные инициализаторы. 
        // Порядок полей обязан строго соответствовать их объявлению в struct Book!
        books.push_back(Book{
            .id = row["id"].as<int>(),
            .title = row["title"].as<std::string>(),
            .author = row["author"].as<std::string>(),
            .year = row["year"].as<int>(),
            .isbn = row["ISBN"].as<std::optional<std::string>>()
        });
    }
    return books;
}
