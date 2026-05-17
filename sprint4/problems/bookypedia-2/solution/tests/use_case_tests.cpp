#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <optional>

#include "../src/app/use_cases_impl.h"
#include "../src/domain/author.h"
#include "../src/domain/book.h"
#include "../src/postgres/postgres.h"

namespace {

struct MockAuthorRepository : domain::AuthorRepository {
    std::vector<domain::Author> saved_authors;

    void Save(const domain::Author& author) override {
        saved_authors.emplace_back(author);
    }

    void Delete(const std::string& author_id) override {
        saved_authors.erase(
            std::remove_if(saved_authors.begin(), saved_authors.end(),
                [&author_id](const domain::Author& author) {
                    return author.GetId().ToString() == author_id;
                }),
            saved_authors.end());
    }

    void Edit(const info::AuthorInfo& author) override {
        for (auto& a : saved_authors) {
            if (a.GetId().ToString() == author.id) {
                a = domain::Author(a.GetId(), author.name);
                break;
            }
        }
    }

    info::Authors GetAuthors() const override {
        info::Authors res;
        res.reserve(saved_authors.size());
        for (const auto& author : saved_authors) {
            res.emplace_back(author.GetId().ToString(), author.GetName());
        }
        return res;
    }

    std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const {
        auto it = std::find_if(saved_authors.begin(), saved_authors.end(),
            [author_name](const domain::Author& author){return author.GetName() == author_name;});
        if (it == saved_authors.end()) {
            return std::nullopt;
        }
        return {{.id = it->GetId().ToString(), .name = it->GetName()}};
    }
};

struct MockBookRepository : domain::BookRepository {
    std::vector<domain::Book> saved_books;

    void Save(const domain::Book& book) override {
        saved_books.emplace_back(book);
    }

    void DeleteBooks(const std::string& author_id) override {
        saved_books.erase(
            std::remove_if(saved_books.begin(), saved_books.end(),
                [&author_id](const domain::Book& book) {
                    return book.GetAuthorId().ToString() == author_id;
                }),
            saved_books.end());
    }

    void DeleteBook(const std::string& book_id) override {
        saved_books.erase(
            std::remove_if(saved_books.begin(), saved_books.end(),
                [&book_id](const domain::Book& book) {
                    return book.GetId().ToString() == book_id;
                }),
            saved_books.end());
    }

    void EditBook(const info::BookInfo& book) override {
        for (auto& b : saved_books) {
            if (b.GetId().ToString() == book.title) {
                // У Book нет метода SetTitle, создаём новый объект
                // Это упрощённая реализация
                break;
            }
        }
    }

    info::Books GetBooks() const override {
        info::Books res;
        res.reserve(saved_books.size());
        for (const auto& book : saved_books) {
            res.emplace_back(book.GetTitle(), book.GetPublicationYear());
        }
        std::sort(res.begin(), res.end(), [](const info::BookInfo& left, const info::BookInfo& right){
            return left.title < right.title;
        });
        return res;
    }

    info::BookInfo GetBook(const std::string& book_id) const override {
        auto it = std::find_if(saved_books.begin(), saved_books.end(),
            [&book_id](const domain::Book& book){return book.GetId().ToString() == book_id;});
        if (it == saved_books.end()) {
            throw std::runtime_error("Book not found");
        }
        return {{it->GetTitle(), it->GetPublicationYear()}};
    }

    info::Books GetBooksByAuthor(const std::string& author_id) const override {
        info::Books res;
        res.reserve(saved_books.size());
        for (const auto& book : saved_books) {
            if (book.GetAuthorId().ToString() == author_id) {
                res.emplace_back(book.GetTitle(), book.GetPublicationYear());
            }
        }
        std::sort(res.begin(), res.end(), [](const info::BookInfo& left, const info::BookInfo& right){
            if (left.publication_year != right.publication_year) {
                return left.publication_year < right.publication_year;
            }
            return left.title < right.title;
        });
        return res;
    }

    info::Books GetBooksByTitle(const std::string& book_title) const override {
        info::Books res;
        for (const auto& book : saved_books) {
            if (book.GetTitle() == book_title) {
                res.emplace_back(book.GetTitle(), book.GetPublicationYear());
            }
        }
        return res;
    }
};

struct Fixture {
    MockAuthorRepository authors;
    MockBookRepository books;
};

}  // namespace

/* SCENARIO_METHOD(Fixture, "Book Adding") {
    GIVEN("Use cases") {
        postgres::UnitOfWorkFactoryImpl 
        app::UseCasesImpl use_cases{authors, books};

        WHEN("Adding an author") {
            const auto author_name = "Joanne Rowling";
            use_cases.AddAuthor(author_name);

            THEN("author with the specified name is saved to repository") {
                REQUIRE(authors.saved_authors.size() == 1);
                CHECK(authors.saved_authors.at(0).GetName() == author_name);
                CHECK(authors.saved_authors.at(0).GetId() != domain::AuthorId{});
            }
        }
    }
} */