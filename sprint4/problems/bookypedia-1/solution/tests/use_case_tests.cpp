#include <catch2/catch_test_macros.hpp>

#include "../src/app/use_cases_impl.h"
#include "../src/domain/author.h"

namespace {

struct MockAuthorRepository : domain::AuthorRepository {
    std::vector<domain::Author> saved_authors;

    void Save(const domain::Author& author) override {
        saved_authors.emplace_back(author);
    }

    std::vector<domain::Author> GetAll() const override {
        return saved_authors;
    }

    std::optional<domain::Author> LoadById(const domain::AuthorId&) const override {
        return std::nullopt;
    }
};

struct MockBookRepository : domain::BookRepository {
    void Save(const domain::Book&) override {}

    std::vector<domain::Book> GetAll() const override {
        return {};
    }

    std::vector<domain::Book> GetByAuthorId(const domain::AuthorId&) const override {
        return {};
    }
};

struct Fixture {
    std::shared_ptr<MockAuthorRepository> authors = std::make_shared<MockAuthorRepository>();
    std::shared_ptr<MockBookRepository> books = std::make_shared<MockBookRepository>();
};

}  // namespace

SCENARIO_METHOD(Fixture, "Book Adding") {
    GIVEN("Use cases") {
        app::UseCasesImpl use_cases{authors, books};

        WHEN("Adding an author") {
            const auto author_name = "Joanne Rowling";
            use_cases.AddAuthor(author_name);

            THEN("author with the specified name is saved to repository") {
                REQUIRE(authors->saved_authors.size() == 1);
                CHECK(authors->saved_authors.at(0).GetName() == author_name);
                CHECK(authors->saved_authors.at(0).GetId() != domain::AuthorId{});
            }
        }
    }
}