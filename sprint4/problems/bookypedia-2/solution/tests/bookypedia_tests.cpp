#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "../src/app/use_cases.h"
#include "../src/domain/author.h"
#include "../src/domain/book.h"
#include "../src/domain/tag.h"
#include "../src/postgres/postgres.h"
#include "../src/ui/struct_info.h"

#include <sstream>
#include <vector>

namespace {

// ============================================================================
// Mock-репозитории для unit-тестов
// Хранят данные в памяти, позволяют тестировать бизнес-логику без БД
// ============================================================================

// Mock репозиторий авторов
struct MockAuthorRepository : domain::AuthorRepository {
    std::vector<domain::Author> saved_authors;
    std::vector<std::string> deleted_ids;
    std::vector<info::AuthorInfo> edited_authors;

    void Save(const domain::Author& author) override {
        auto it = std::find_if(saved_authors.begin(), saved_authors.end(),
            [&author](const domain::Author& a){ return a.GetId() == author.GetId(); });
        if (it != saved_authors.end()) {
            *it = author;  // Обновление
        } else {
            saved_authors.emplace_back(author);  // Добавление
        }
    }

    void Delete(const std::string& author_id) override {
        deleted_ids.push_back(author_id);
        saved_authors.erase(
            std::remove_if(saved_authors.begin(), saved_authors.end(),
                [&author_id](const domain::Author& a){ return a.GetId().ToString() == author_id; }),
            saved_authors.end());
    }

    void Edit(const info::AuthorInfo& author) override {
        edited_authors.push_back(author);
        for (auto& a : saved_authors) {
            if (a.GetId().ToString() == author.id) {
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

    std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const override {
        auto it = std::find_if(saved_authors.begin(), saved_authors.end(),
            [&author_name](const domain::Author& a){ return a.GetName() == author_name; });
        if (it == saved_authors.end()) {
            return std::nullopt;
        }
        return {{.id = it->GetId().ToString(), .name = it->GetName()}};
    }
};

// Mock репозиторий книг
struct MockBookRepository : domain::BookRepository {
    std::vector<domain::Book> saved_books;
    std::vector<std::string> deleted_book_ids;
    std::vector<std::string> deleted_books_by_author;
    std::vector<info::BookInfo> edited_books;

    void Save(const domain::Book& book) override {
        auto it = std::find_if(saved_books.begin(), saved_books.end(),
            [&book](const domain::Book& b){ return b.GetId() == book.GetId(); });
        if (it != saved_books.end()) {
            *it = book;
        } else {
            saved_books.emplace_back(book);
        }
    }

    void DeleteBooks(const std::string& author_id) override {
        deleted_books_by_author.push_back(author_id);
        saved_books.erase(
            std::remove_if(saved_books.begin(), saved_books.end(),
                [&author_id](const domain::Book& b){ return b.GetAuthorId().ToString() == author_id; }),
            saved_books.end());
    }

    void DeleteBook(const std::string& book_id) override {
        deleted_book_ids.push_back(book_id);
        saved_books.erase(
            std::remove_if(saved_books.begin(), saved_books.end(),
                [&book_id](const domain::Book& b){ return b.GetId().ToString() == book_id; }),
            saved_books.end());
    }

    void EditBook(const info::BookInfo& book) override {
        edited_books.push_back(book);
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
            [&book_id](const domain::Book& b){ return b.GetId().ToString() == book_id; });
        if (it == saved_books.end()) {
            return info::BookInfo{};
        }
        return {it->GetTitle(), it->GetPublicationYear(), "", {}, book_id};
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
        res.reserve(saved_books.size());
        for (const auto& book : saved_books) {
            if (book.GetTitle() == book_title) {
                res.emplace_back(book.GetTitle(), book.GetPublicationYear());
            }
        }
        return res;
    }
};

// Mock репозиторий тегов
struct MockTagRepository : domain::TagRepository {
    std::vector<domain::Tag> saved_tags;
    std::vector<std::string> deleted_tags_for_book;
    std::vector<std::string> deleted_tags_for_author;

    void Save(const std::vector<domain::Tag>& tags) override {
        for (const auto& tag : tags) {
            auto it = std::find_if(saved_tags.begin(), saved_tags.end(),
                [&tag](const domain::Tag& t){ 
                    return t.GetBookId() == tag.GetBookId() && t.GetTag() == tag.GetTag(); 
                });
            if (it == saved_tags.end()) {
                saved_tags.emplace_back(tag);
            }
        }
    }

    void Save(const std::vector<std::string>& tags, const std::string& book_id) override {
        for (const auto& tag : tags) {
            saved_tags.emplace_back(domain::BookId::FromString(book_id), tag);
        }
    }

    void DeleteTagsForAuthor(const std::string& author_id) override {
        deleted_tags_for_author.push_back(author_id);
    }

    void DeleteTagsForBook(const std::string& book_id) override {
        deleted_tags_for_book.push_back(book_id);
        saved_tags.erase(
            std::remove_if(saved_tags.begin(), saved_tags.end(),
                [&book_id](const domain::Tag& t){ return t.GetBookId().ToString() == book_id; }),
            saved_tags.end());
    }

    std::vector<std::string> GetTags(const std::string& book_id) const override {
        std::vector<std::string> res;
        for (const auto& tag : saved_tags) {
            if (tag.GetBookId().ToString() == book_id) {
                res.push_back(tag.GetTag());
            }
        }
        std::sort(res.begin(), res.end());
        return res;
    }
};

// ============================================================================
// Mock UnitOfWork и UnitOfWorkFactory для тестирования UseCasesImpl
// ============================================================================

// Mock UnitOfWork - имитирует транзакцию
class MockUnitOfWork : public app::UnitOfWork {
public:
    MockUnitOfWork(MockAuthorRepository* authors, MockBookRepository* books, MockTagRepository* tags)
        : authors_(authors), books_(books), tags_(tags) {}

    void Commit() override {}  // Имитация commit

    domain::AuthorRepository& Authors() override { return *authors_; }
    domain::BookRepository& Books() override { return *books_; }
    domain::TagRepository& Tags() override { return *tags_; }

private:
    MockAuthorRepository* authors_;
    MockBookRepository* books_;
    MockTagRepository* tags_;
};

// Mock фабрика UnitOfWork
class MockUnitOfWorkFactory : public app::UnitOfWorkFactory {
public:
    MockUnitOfWorkFactory(MockAuthorRepository* authors, MockBookRepository* books, MockTagRepository* tags)
        : authors_(authors), books_(books), tags_(tags) {}

    std::shared_ptr<app::UnitOfWork> CreateUnitOfWork(app::TypeOfTransaction) override {
        return std::make_shared<MockUnitOfWork>(authors_, books_, tags_);
    }

private:
    MockAuthorRepository* authors_;
    MockBookRepository* books_;
    MockTagRepository* tags_;
};

}  // namespace

// ============================================================================
// Unit-тесты для UseCases
// Тестируют бизнес-логику без обращения к БД
// ============================================================================

// Тест: успешное добавление автора
TEST_CASE("AddAuthor - successful author creation") {
    MockAuthorRepository mock_authors;
    MockBookRepository mock_books;
    MockTagRepository mock_tags;
    
    MockUnitOfWorkFactory factory(&mock_authors, &mock_books, &mock_tags);
    app::UseCasesImpl use_cases(&factory);

    const auto author_name = "J.K. Rowling";
    use_cases.AddAuthor(author_name);

    REQUIRE(mock_authors.saved_authors.size() == 1);
    CHECK(mock_authors.saved_authors.at(0).GetName() == author_name);
    CHECK(mock_authors.saved_authors.at(0).GetId() != domain::AuthorId{});
}

// Тест: добавление нескольких авторов
TEST_CASE("AddAuthor - multiple authors") {
    MockAuthorRepository mock_authors;
    MockBookRepository mock_books;
    MockTagRepository mock_tags;
    
    MockUnitOfWorkFactory factory(&mock_authors, &mock_books, &mock_tags);
    app::UseCasesImpl use_cases(&factory);

    use_cases.AddAuthor("Author One");
    use_cases.AddAuthor("Author Two");
    use_cases.AddAuthor("Author Three");

    REQUIRE(mock_authors.saved_authors.size() == 3);
    
    auto authors = use_cases.GetAuthors();
    REQUIRE(authors.size() == 3);
}

// Тест: поиск существующего автора
TEST_CASE("GetAuthorByName - find existing author") {
    MockAuthorRepository mock_authors;
    MockBookRepository mock_books;
    MockTagRepository mock_tags;
    
    MockUnitOfWorkFactory factory(&mock_authors, &mock_books, &mock_tags);
    app::UseCasesImpl use_cases(&factory);

    use_cases.AddAuthor("George Orwell");
    
    auto author = use_cases.GetAuthorByName("George Orwell");
    REQUIRE(author.has_value());
    CHECK(author->name == "George Orwell");
    CHECK(!author->id.empty());
}

// Тест: поиск несуществующего автора
TEST_CASE("GetAuthorByName - author not found") {
    MockAuthorRepository mock_authors;
    MockBookRepository mock_books;
    MockTagRepository mock_tags;
    
    MockUnitOfWorkFactory factory(&mock_authors, &mock_books, &mock_tags);
    app::UseCasesImpl use_cases(&factory);

    use_cases.AddAuthor("George Orwell");
    
    auto author = use_cases.GetAuthorByName("Nonexistent Author");
    CHECK(!author.has_value());
}

// Тест: добавление книги с существующим автором и тегами
TEST_CASE("AddBook - book with existing author") {
    MockAuthorRepository mock_authors;
    MockBookRepository mock_books;
    MockTagRepository mock_tags;
    
    MockUnitOfWorkFactory factory(&mock_authors, &mock_books, &mock_tags);
    app::UseCasesImpl use_cases(&factory);

    const auto author_name = "George Orwell";
    use_cases.AddAuthor(author_name);
    auto author = use_cases.GetAuthorByName(author_name);
    REQUIRE(author.has_value());

    use_cases.AddBook(author->id, author->name, "1984", 1949, {"dystopia", "politics"});

    auto books = use_cases.GetBooks();
    REQUIRE(books.size() == 1);
    CHECK(books[0].title == "1984");
    CHECK(books[0].publication_year == 1949);
    CHECK(books[0].author_name == author_name);

    auto book = use_cases.GetBook(books[0].id);
    REQUIRE(book.tags.size() == 2);
    CHECK(std::find(book.tags.begin(), book.tags.end(), "dystopia") != book.tags.end());
    CHECK(std::find(book.tags.begin(), book.tags.end(), "politics") != book.tags.end());
}

// Тест: добавление книги без тегов
TEST_CASE("AddBook - book with empty tags") {
    MockAuthorRepository mock_authors;
    MockBookRepository mock_books;
    MockTagRepository mock_tags;
    
    MockUnitOfWorkFactory factory(&mock_authors, &mock_books, &mock_tags);
    app::UseCasesImpl use_cases(&factory);

    use_cases.AddAuthor("Some Author");
    auto author = use_cases.GetAuthorByName("Some Author");
    REQUIRE(author.has_value());

    use_cases.AddBook(author->id, author->name, "Some Book", 2020, {});

    auto books = use_cases.GetBooks();
    REQUIRE(books.size() == 1);

    auto book = use_cases.GetBook(books[0].id);
    CHECK(book.tags.empty());
}

// Тест: добавление нескольких книг одному автору
TEST_CASE("AddBook - multiple books by same author") {
    MockAuthorRepository mock_authors;
    MockBookRepository mock_books;
    MockTagRepository mock_tags;
    
    MockUnitOfWorkFactory factory(&mock_authors, &mock_books, &mock_tags);
    app::UseCasesImpl use_cases(&factory);

    use_cases.AddAuthor("Agatha Christie");
    auto author = use_cases.GetAuthorByName("Agatha Christie");
    REQUIRE(author.has_value());

    use_cases.AddBook(author->id, author->name, "Murder on the Orient Express", 1934, {"mystery", "detective"});
    use_cases.AddBook(author->id, author->name, "And Then There Were None", 1939, {"mystery", "thriller"});

    auto author_books = use_cases.GetAuthorBooks(author->id);
    REQUIRE(author_books.size() == 2);
}

// Тест: успешное удаление книги
TEST_CASE("DeleteBook - successful deletion") {
    MockAuthorRepository mock_authors;
    MockBookRepository mock_books;
    MockTagRepository mock_tags;
    
    MockUnitOfWorkFactory factory(&mock_authors, &mock_books, &mock_tags);
    app::UseCasesImpl use_cases(&factory);

    use_cases.AddAuthor("Test Author");
    auto author = use_cases.GetAuthorByName("Test Author");
    REQUIRE(author.has_value());

    use_cases.AddBook(author->id, author->name, "Test Book", 2020, {"tag1"});
    auto books = use_cases.GetBooks();
    REQUIRE(books.size() == 1);
    std::string book_id = books[0].id;

    use_cases.DeleteBook(book_id);

    books = use_cases.GetBooks();
    CHECK(books.empty());

    auto deleted_tags = mock_tags.deleted_tags_for_book;
    REQUIRE(deleted_tags.size() == 1);
    CHECK(deleted_tags[0] == book_id);
}

// Тест: редактирование тегов книги
TEST_CASE("EditBook - update tags") {
    MockAuthorRepository mock_authors;
    MockBookRepository mock_books;
    MockTagRepository mock_tags;
    
    MockUnitOfWorkFactory factory(&mock_authors, &mock_books, &mock_tags);
    app::UseCasesImpl use_cases(&factory);

    use_cases.AddAuthor("Test Author");
    auto author = use_cases.GetAuthorByName("Test Author");
    REQUIRE(author.has_value());

    use_cases.AddBook(author->id, author->name, "Test Book", 2020, {"old_tag"});
    auto books = use_cases.GetBooks();
    REQUIRE(books.size() == 1);

    info::BookInfo book_to_edit = use_cases.GetBook(books[0].id);
    book_to_edit.tags = {"new_tag1", "new_tag2"};
    
    use_cases.EditBook(book_to_edit);

    auto updated_book = use_cases.GetBook(book_to_edit.id);
    REQUIRE(updated_book.tags.size() == 2);
    CHECK(std::find(updated_book.tags.begin(), updated_book.tags.end(), "new_tag1") != updated_book.tags.end());
    CHECK(std::find(updated_book.tags.begin(), updated_book.tags.end(), "new_tag2") != updated_book.tags.end());
}

// Тест: каскадное удаление автора, книг и тегов
TEST_CASE("DeleteAuthor - cascade deletion") {
    MockAuthorRepository mock_authors;
    MockBookRepository mock_books;
    MockTagRepository mock_tags;
    
    MockUnitOfWorkFactory factory(&mock_authors, &mock_books, &mock_tags);
    app::UseCasesImpl use_cases(&factory);

    use_cases.AddAuthor("Test Author");
    auto author = use_cases.GetAuthorByName("Test Author");
    REQUIRE(author.has_value());

    use_cases.AddBook(author->id, author->name, "Book 1", 2020, {"tag1"});
    use_cases.AddBook(author->id, author->name, "Book 2", 2021, {"tag2"});

    auto books = use_cases.GetBooks();
    REQUIRE(books.size() == 2);

    use_cases.DeleteAuthor(author->id);

    CHECK(mock_authors.deleted_ids.size() == 1);
    CHECK(mock_books.deleted_books_by_author.size() == 1);
    CHECK(mock_tags.deleted_tags_for_author.size() == 1);
    
    books = use_cases.GetBooks();
    CHECK(books.empty());
}

// Тест: сортировка книг
TEST_CASE("GetBooks - sorting") {
    MockAuthorRepository mock_authors;
    MockBookRepository mock_books;
    MockTagRepository mock_tags;
    
    MockUnitOfWorkFactory factory(&mock_authors, &mock_books, &mock_tags);
    app::UseCasesImpl use_cases(&factory);

    use_cases.AddAuthor("Author B");
    use_cases.AddAuthor("Author A");
    
    auto author_a = use_cases.GetAuthorByName("Author A");
    auto author_b = use_cases.GetAuthorByName("Author B");
    
    use_cases.AddBook(author_a->id, author_a->name, "Same Title", 2020, {});
    use_cases.AddBook(author_b->id, author_b->name, "Same Title", 2019, {});
    use_cases.AddBook(author_a->id, author_a->name, "Another Title", 2021, {});

    auto books = use_cases.GetBooks();
    REQUIRE(books.size() == 3);
    
    // Книги сортируются: по названию, затем по автору, затем по году
    CHECK(books[0].title == "Another Title");
    CHECK(books[1].title == "Same Title");
    CHECK(books[1].author_name == "Author A");
    CHECK(books[2].title == "Same Title");
    CHECK(books[2].author_name == "Author B");
}

// Тест: редактирование имени автора
TEST_CASE("EditAuthor - update name") {
    MockAuthorRepository mock_authors;
    MockBookRepository mock_books;
    MockTagRepository mock_tags;
    
    MockUnitOfWorkFactory factory(&mock_authors, &mock_books, &mock_tags);
    app::UseCasesImpl use_cases(&factory);

    use_cases.AddAuthor("Original Name");
    auto author = use_cases.GetAuthorByName("Original Name");
    REQUIRE(author.has_value());

    info::AuthorInfo author_to_edit = *author;
    author_to_edit.name = "New Name";
    
    use_cases.EditAuthor(author_to_edit);

    auto updated_author = use_cases.GetAuthorByName("New Name");
    REQUIRE(updated_author.has_value());
    CHECK(updated_author->id == author->id);
}

// Тест: поиск книг по названию
TEST_CASE("GetBooksByTitle - find books by title") {
    MockAuthorRepository mock_authors;
    MockBookRepository mock_books;
    MockTagRepository mock_tags;
    
    MockUnitOfWorkFactory factory(&mock_authors, &mock_books, &mock_tags);
    app::UseCasesImpl use_cases(&factory);

    use_cases.AddAuthor("Author 1");
    use_cases.AddAuthor("Author 2");
    
    auto author_1 = use_cases.GetAuthorByName("Author 1");
    auto author_2 = use_cases.GetAuthorByName("Author 2");
    
    use_cases.AddBook(author_1->id, author_1->name, "Common Title", 2020, {});
    use_cases.AddBook(author_2->id, author_2->name, "Common Title", 2021, {});
    use_cases.AddBook(author_1->id, author_1->name, "Unique Title", 2022, {});

    auto common_books = use_cases.GetBooksByTitle("Common Title");
    REQUIRE(common_books.size() == 2);

    auto unique_books = use_cases.GetBooksByTitle("Unique Title");
    REQUIRE(unique_books.size() == 1);

    auto nonexistent_books = use_cases.GetBooksByTitle("Nonexistent");
    CHECK(nonexistent_books.empty());
}
