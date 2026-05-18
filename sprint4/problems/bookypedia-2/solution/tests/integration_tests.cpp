#include <catch2/catch_test_macros.hpp>
#include <pqxx/pqxx>

#include "../src/app/use_cases.h"
#include "../src/domain/author.h"
#include "../src/domain/book.h"
#include "../src/domain/tag.h"
#include "../src/postgres/postgres.h"

#include <cstdlib>
#include <optional>

namespace {

constexpr const char* DB_URL_ENV = "BOOKYPEDIA_DB_URL";

std::string GetTestDbUrl() {
    if (const auto* url = std::getenv(DB_URL_ENV)) {
        return std::string(url);
    }
    // Default test database URL
    return "postgres://postgres:postgres@localhost:5432/bookypedia_test";
}

void DropTestTables() {
    try {
        pqxx::connection conn{GetTestDbUrl()};
        pqxx::work tx{conn};
        tx.exec("DROP TABLE IF EXISTS book_tags CASCADE");
        tx.exec("DROP TABLE IF EXISTS books CASCADE");
        tx.exec("DROP TABLE IF EXISTS authors CASCADE");
        tx.commit();
    } catch (...) {
        // Ignore errors during cleanup
    }
}

void CreateTestTables() {
    pqxx::connection conn{GetTestDbUrl()};
    pqxx::work tx{conn};
    tx.exec(R"(
        CREATE TABLE IF NOT EXISTS authors (
            id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
            name varchar(100) UNIQUE NOT NULL 
        );
    )");
    tx.exec(R"(
        CREATE TABLE IF NOT EXISTS books (
            id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
            author_id UUID NOT NULL,
            title varchar(100) NOT NULL,
            publication_year integer
        );
    )");
    tx.exec(R"(
        CREATE TABLE IF NOT EXISTS book_tags (
            book_id UUID NOT NULL,
            tag varchar(30) NOT NULL
        );
    )");
    tx.commit();
}

class IntegrationFixture {
public:
    IntegrationFixture() {
        DropTestTables();
        CreateTestTables();
        db_ = std::make_unique<postgres::Database>(pqxx::connection{GetTestDbUrl()});
        use_cases_ = std::make_unique<app::UseCasesImpl>(db_->GetUowFactory());
    }

    ~IntegrationFixture() {
        DropTestTables();
    }

    std::unique_ptr<postgres::Database> db_;
    std::unique_ptr<app::UseCasesImpl> use_cases_;
};

}  // namespace

// ============================================================================
// Integration tests with PostgreSQL
// ============================================================================

TEST_CASE("Integration: Add and retrieve author") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("Test Author");
    
    auto authors = fixture.use_cases_->GetAuthors();
    REQUIRE(authors.size() == 1);
    CHECK(authors[0].name == "Test Author");
    CHECK(!authors[0].id.empty());
}

TEST_CASE("Integration: Add book with tags") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("George Orwell");
    auto author = fixture.use_cases_->GetAuthorByName("George Orwell");
    REQUIRE(author.has_value());

    fixture.use_cases_->AddBook(author->id, author->name, "1984", 1949, 
        {"dystopia", "classic", "politics"});
    
    auto books = fixture.use_cases_->GetBooks();
    REQUIRE(books.size() == 1);
    CHECK(books[0].title == "1984");
    CHECK(books[0].publication_year == 1949);
    CHECK(books[0].author_name == "George Orwell");
    REQUIRE(books[0].tags.size() == 3);
}

TEST_CASE("Integration: Delete book and its tags") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("Test Author");
    auto author = fixture.use_cases_->GetAuthorByName("Test Author");
    REQUIRE(author.has_value());

    fixture.use_cases_->AddBook(author->id, author->name, "Test Book", 2020, {"tag1", "tag2"});
    auto books = fixture.use_cases_->GetBooks();
    REQUIRE(books.size() == 1);
    
    std::string book_id = books[0].id;
    fixture.use_cases_->DeleteBook(book_id);
    
    books = fixture.use_cases_->GetBooks();
    CHECK(books.empty());
}

TEST_CASE("Integration: Delete author and cascade delete books and tags") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("Test Author");
    auto author = fixture.use_cases_->GetAuthorByName("Test Author");
    REQUIRE(author.has_value());

    fixture.use_cases_->AddBook(author->id, author->name, "Book 1", 2020, {"tag1"});
    fixture.use_cases_->AddBook(author->id, author->name, "Book 2", 2021, {"tag2"});
    
    auto books = fixture.use_cases_->GetBooks();
    REQUIRE(books.size() == 2);

    fixture.use_cases_->DeleteAuthor(author->id);
    
    books = fixture.use_cases_->GetBooks();
    CHECK(books.empty());
    
    auto authors = fixture.use_cases_->GetAuthors();
    CHECK(authors.empty());
}

TEST_CASE("Integration: Edit book") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("Test Author");
    auto author = fixture.use_cases_->GetAuthorByName("Test Author");
    REQUIRE(author.has_value());

    fixture.use_cases_->AddBook(author->id, author->name, "Original Title", 2020, {"old_tag"});
    auto books = fixture.use_cases_->GetBooks();
    REQUIRE(books.size() == 1);

    info::BookInfo book = fixture.use_cases_->GetBook(books[0].id);
    book.title = "New Title";
    book.publication_year = 2021;
    book.tags = {"new_tag1", "new_tag2"};
    
    fixture.use_cases_->EditBook(book);
    
    auto updated_book = fixture.use_cases_->GetBook(book.id);
    CHECK(updated_book.title == "New Title");
    CHECK(updated_book.publication_year == 2021);
    REQUIRE(updated_book.tags.size() == 2);
}

TEST_CASE("Integration: Edit author") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("Original Name");
    auto author = fixture.use_cases_->GetAuthorByName("Original Name");
    REQUIRE(author.has_value());

    info::AuthorInfo author_info = *author;
    author_info.name = "New Name";
    
    fixture.use_cases_->EditAuthor(author_info);
    
    auto updated_author = fixture.use_cases_->GetAuthorByName("New Name");
    REQUIRE(updated_author.has_value());
    CHECK(updated_author->id == author->id);
    
    auto old_author = fixture.use_cases_->GetAuthorByName("Original Name");
    CHECK(!old_author.has_value());
}

TEST_CASE("Integration: Get books by title") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("Author 1");
    fixture.use_cases_->AddAuthor("Author 2");
    
    auto author_1 = fixture.use_cases_->GetAuthorByName("Author 1");
    auto author_2 = fixture.use_cases_->GetAuthorByName("Author 2");
    
    fixture.use_cases_->AddBook(author_1->id, author_1->name, "Common Title", 2020, {});
    fixture.use_cases_->AddBook(author_2->id, author_2->name, "Common Title", 2021, {});
    fixture.use_cases_->AddBook(author_1->id, author_1->name, "Unique Title", 2022, {});

    auto common_books = fixture.use_cases_->GetBooksByTitle("Common Title");
    REQUIRE(common_books.size() == 2);

    auto unique_books = fixture.use_cases_->GetBooksByTitle("Unique Title");
    REQUIRE(unique_books.size() == 1);
}

TEST_CASE("Integration: Get author books") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("Agatha Christie");
    auto author = fixture.use_cases_->GetAuthorByName("Agatha Christie");
    REQUIRE(author.has_value());

    fixture.use_cases_->AddBook(author->id, author->name, "Book A", 1930, {});
    fixture.use_cases_->AddBook(author->id, author->name, "Book B", 1940, {});
    
    auto author_books = fixture.use_cases_->GetAuthorBooks(author->id);
    REQUIRE(author_books.size() == 2);
}

TEST_CASE("Integration: Tags are sorted alphabetically") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("Test Author");
    auto author = fixture.use_cases_->GetAuthorByName("Test Author");
    REQUIRE(author.has_value());

    fixture.use_cases_->AddBook(author->id, author->name, "Test Book", 2020, 
        {"zebra", "apple", "mango"});
    
    auto book = fixture.use_cases_->GetBook(
        fixture.use_cases_->GetBooks()[0].id);
    
    REQUIRE(book.tags.size() == 3);
    CHECK(book.tags[0] == "apple");
    CHECK(book.tags[1] == "mango");
    CHECK(book.tags[2] == "zebra");
}

TEST_CASE("Integration: Duplicate tags are not stored") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("Test Author");
    auto author = fixture.use_cases_->GetAuthorByName("Test Author");
    REQUIRE(author.has_value());

    fixture.use_cases_->AddBook(author->id, author->name, "Test Book", 2020, 
        {"tag1", "tag2", "tag1", "tag2"});
    
    auto book = fixture.use_cases_->GetBook(
        fixture.use_cases_->GetBooks()[0].id);
    
    REQUIRE(book.tags.size() == 2);
}

TEST_CASE("Integration: Multiple authors with books") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("Author A");
    fixture.use_cases_->AddAuthor("Author B");
    fixture.use_cases_->AddAuthor("Author C");

    auto author_a = fixture.use_cases_->GetAuthorByName("Author A");
    auto author_b = fixture.use_cases_->GetAuthorByName("Author B");
    
    fixture.use_cases_->AddBook(author_a->id, author_a->name, "Book from A", 2020, {});
    fixture.use_cases_->AddBook(author_b->id, author_b->name, "Book from B", 2021, {});

    auto authors = fixture.use_cases_->GetAuthors();
    REQUIRE(authors.size() == 3);

    auto books = fixture.use_cases_->GetBooks();
    REQUIRE(books.size() == 2);
}

TEST_CASE("Integration: Book with empty tags list") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("Test Author");
    auto author = fixture.use_cases_->GetAuthorByName("Test Author");
    REQUIRE(author.has_value());

    fixture.use_cases_->AddBook(author->id, author->name, "Book Without Tags", 2020, {});
    
    auto book = fixture.use_cases_->GetBook(
        fixture.use_cases_->GetBooks()[0].id);
    
    CHECK(book.tags.empty());
}

TEST_CASE("Integration: Get non-existent author") {
    IntegrationFixture fixture;

    auto author = fixture.use_cases_->GetAuthorByName("Nonexistent Author");
    CHECK(!author.has_value());
}

TEST_CASE("Integration: Get non-existent book") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("Test Author");
    auto author = fixture.use_cases_->GetAuthorByName("Test Author");
    REQUIRE(author.has_value());

    fixture.use_cases_->AddBook(author->id, author->name, "Test Book", 2020, {});
    
    auto non_existent = fixture.use_cases_->GetBook("00000000-0000-0000-0000-000000000000");
    CHECK(non_existent.title.empty());
}

TEST_CASE("Integration: Delete non-existent author") {
    IntegrationFixture fixture;

    fixture.use_cases_->AddAuthor("Test Author");
    
    auto initial_authors = fixture.use_cases_->GetAuthors();
    REQUIRE(initial_authors.size() == 1);

    // Try to delete non-existent author
    fixture.use_cases_->DeleteAuthor("00000000-0000-0000-0000-000000000000");
    
    // Should still have the original author
    auto authors = fixture.use_cases_->GetAuthors();
    REQUIRE(authors.size() == 1);
}
