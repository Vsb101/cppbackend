#pragma once

#include <string>
#include <vector>
#include <optional>
#include <pqxx/pqxx>

struct Book {
    int id{0};
    std::string title;
    std::string author;
    int year{0};
    std::optional<std::string> isbn;
};

class BookRepository {
public:
    explicit BookRepository(pqxx::connection& conn);

    bool Add(const Book& book);
    std::vector<Book> GetAll();

private:
    pqxx::connection& conn_;
    
    static inline const std::string kAddBook = "add_book";
    static inline const std::string kAddBookNullIsbn = "add_book_null_isbn";
    static inline const std::string kAllBooks = "all_books";
};
