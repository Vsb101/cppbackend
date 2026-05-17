#include <catch2/catch_test_macros.hpp>
#include <algorithm>

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

    info::Authors GetAuthors() const {
        info::Authors res;
        res.reserve(saved_authors.size());
        for (const auto& author : saved_authors) {
            res.emplace_back(author.GetId().ToString(), author.GetName());
        }
        return res;
    }

    std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const {
        // saved_authors.fi
        // const auto it = actions_.find(cmd); it != actions_.cend()
        auto it = std::find_if(saved_authors.begin(), saved_authors.end()
        , [author_name](const domain::Author& author){return author.GetName() == author_name;});
        if (it == saved_authors.end()) {
            return std::nullopt;
        }
        // it->GetId().ToString()
        return {{.id = it->GetId().ToString(), .name = it->GetName()}};
    }
};

struct MockBookRepository : domain::BookRepository {
    std::vector<domain::Book> saved_books;

    void Save(const domain::Book& book) override {
        saved_books.emplace_back(book);
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