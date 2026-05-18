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
#include <unordered_map>
#include <unordered_set>
#include <set>

namespace {

// ============================================================================
// Mock-репозитории для unit-тестов
// Хранят данные в памяти, позволяют тестировать бизнес-логику без БД
// ============================================================================

// Mock репозиторий авторов с оптимизированным поиском
// Используем unordered_map для O(1) поиска по ID и имени
struct MockAuthorRepository : domain::AuthorRepository {
    // Храним авторов по ID для быстрого поиска (O(1))
    std::unordered_map<std::string, domain::Author> authors_by_id_;
    // Индекс по имени для быстрого поиска (O(1))
    std::unordered_map<std::string, std::string> name_to_id_;
    std::vector<std::string> deleted_ids_;
    std::vector<info::AuthorInfo> edited_authors_;

    void Save(const domain::Author& author) override {
        const std::string id = author.GetId().ToString();
        const std::string name = author.GetName();
        
        auto it = authors_by_id_.find(id);
        if (it != authors_by_id_.end()) {
            // Обновление существующего автора
            // Удаляем старое имя из индекса
            name_to_id_.erase(it->second.GetName());
            it->second = author;
        } else {
            // Добавление нового автора
            authors_by_id_.emplace(id, author);
        }
        // Обновляем индекс по имени
        name_to_id_[name] = id;
    }

    void Delete(const std::string& author_id) override {
        deleted_ids_.push_back(author_id);
        
        auto it = authors_by_id_.find(author_id);
        if (it != authors_by_id_.end()) {
            // Удаляем из индекса по имени
            name_to_id_.erase(it->second.GetName());
            authors_by_id_.erase(it);
        }
    }

    void Edit(const info::AuthorInfo& author) override {
        edited_authors_.push_back(author);
        
        auto it = authors_by_id_.find(author.id);
        if (it != authors_by_id_.end()) {
            // Обновляем имя в индексе
            name_to_id_.erase(it->second.GetName());
            name_to_id_[author.name] = author.id;
        }
    }

    info::Authors GetAuthors() const override {
        info::Authors res;
        res.reserve(authors_by_id_.size());
        for (const auto& [id, author] : authors_by_id_) {
            res.emplace_back(id, author.GetName());
        }
        return res;
    }

    std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const override {
        // O(1) поиск через индекс
        auto it = name_to_id_.find(author_name);
        if (it == name_to_id_.end()) {
            return std::nullopt;
        }
        
        auto author_it = authors_by_id_.find(it->second);
        if (author_it != authors_by_id_.end()) {
            return {{.id = it->second, .name = author_name}};
        }
        return std::nullopt;
    }
};

// Mock репозиторий книг с оптимизированным поиском
// Используем unordered_map для O(1) поиска по ID и индексы по автору/названию
struct MockBookRepository : domain::BookRepository {
    // Храним книги по ID для O(1) поиска
    std::unordered_map<std::string, domain::Book> books_by_id_;
    // Индекс по автору: author_id -> список book_id
    std::unordered_map<std::string, std::vector<std::string>> books_by_author_;
    // Индекс по названию: title -> список book_id
    std::unordered_map<std::string, std::vector<std::string>> books_by_title_;
    std::vector<std::string> deleted_book_ids_;
    std::vector<std::string> deleted_books_by_author_;
    std::vector<info::BookInfo> edited_books_;

    void Save(const domain::Book& book) override {
        const std::string book_id = book.GetId().ToString();
        const std::string author_id = book.GetAuthorId().ToString();
        const std::string title = book.GetTitle();
        
        auto it = books_by_id_.find(book_id);
        if (it != books_by_id_.end()) {
            // Обновление: удаляем старую книгу из индексов
            const std::string old_author = it->second.GetAuthorId().ToString();
            const std::string old_title = it->second.GetTitle();
            
            auto& old_author_books = books_by_author_[old_author];
            old_author_books.erase(
                std::remove(old_author_books.begin(), old_author_books.end(), book_id),
                old_author_books.end());
            
            auto& old_title_books = books_by_title_[old_title];
            old_title_books.erase(
                std::remove(old_title_books.begin(), old_title_books.end(), book_id),
                old_title_books.end());
            
            it->second = book;
        } else {
            books_by_id_.emplace(book_id, book);
        }
        
        // Обновляем индексы
        books_by_author_[author_id].push_back(book_id);
        books_by_title_[title].push_back(book_id);
    }

    void DeleteBooks(const std::string& author_id) override {
        deleted_books_by_author_.push_back(author_id);
        
        auto it = books_by_author_.find(author_id);
        if (it == books_by_author_.end()) {
            return;
        }
        
        for (const auto& book_id : it->second) {
            auto book_it = books_by_id_.find(book_id);
            if (book_it != books_by_id_.end()) {
                // Удаляем из индекса по названию
                auto& title_books = books_by_title_[book_it->second.GetTitle()];
                title_books.erase(
                    std::remove(title_books.begin(), title_books.end(), book_id),
                    title_books.end());
                books_by_id_.erase(book_it);
            }
        }
        books_by_author_.erase(it);
    }

    void DeleteBook(const std::string& book_id) override {
        deleted_book_ids_.push_back(book_id);
        
        auto it = books_by_id_.find(book_id);
        if (it == books_by_id_.end()) {
            return;
        }
        
        // Удаляем из индекса по автору
        const std::string author_id = it->second.GetAuthorId().ToString();
        auto& author_books = books_by_author_[author_id];
        author_books.erase(
            std::remove(author_books.begin(), author_books.end(), book_id),
            author_books.end());
        
        // Удаляем из индекса по названию
        const std::string title = it->second.GetTitle();
        auto& title_books = books_by_title_[title];
        title_books.erase(
            std::remove(title_books.begin(), title_books.end(), book_id),
            title_books.end());
        
        books_by_id_.erase(it);
    }

    void EditBook(const info::BookInfo& book) override {
        edited_books_.push_back(book);
    }

    info::Books GetBooks() const override {
        info::Books res;
        res.reserve(books_by_id_.size());
        for (const auto& [id, book] : books_by_id_) {
            res.emplace_back(book.GetTitle(), book.GetPublicationYear());
        }
        std::sort(res.begin(), res.end(), [](const info::BookInfo& left, const info::BookInfo& right){
            return left.title < right.title;
        });
        return res;
    }

    info::BookInfo GetBook(const std::string& book_id) const override {
        auto it = books_by_id_.find(book_id);
        if (it == books_by_id_.end()) {
            return info::BookInfo{};
        }
        return {it->second.GetTitle(), it->second.GetPublicationYear(), "", {}, book_id};
    }

    info::Books GetBooksByAuthor(const std::string& author_id) const override {
        info::Books res;
        
        auto it = books_by_author_.find(author_id);
        if (it == books_by_author_.end()) {
            return res;
        }
        
        res.reserve(it->second.size());
        for (const auto& book_id : it->second) {
            auto book_it = books_by_id_.find(book_id);
            if (book_it != books_by_id_.end()) {
                res.emplace_back(book_it->second.GetTitle(), book_it->second.GetPublicationYear());
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
        
        auto it = books_by_title_.find(book_title);
        if (it == books_by_title_.end()) {
            return res;
        }
        
        res.reserve(it->second.size());
        for (const auto& book_id : it->second) {
            auto book_it = books_by_id_.find(book_id);
            if (book_it != books_by_id_.end()) {
                res.emplace_back(book_it->second.GetTitle(), book_it->second.GetPublicationYear());
            }
        }
        return res;
    }
};

// Mock репозиторий тегов с оптимизированным поиском
// Используем unordered_map + set для O(1) доступа и автоматической сортировки
struct MockTagRepository : domain::TagRepository {
    // Храним теги по book_id: book_id -> set<tag> (set для уникальности и сортировки)
    std::unordered_map<std::string, std::set<std::string>> tags_by_book_;
    std::vector<std::string> deleted_tags_for_book_;
    std::vector<std::string> deleted_tags_for_author_;

    void Save(const std::vector<domain::Tag>& tags) override {
        for (const auto& tag : tags) {
            tags_by_book_[tag.GetBookId().ToString()].insert(tag.GetTag());
        }
    }

    void Save(const std::vector<std::string>& tags, const std::string& book_id) override {
        auto& book_tags = tags_by_book_[book_id];
        for (const auto& tag : tags) {
            book_tags.insert(tag);
        }
    }

    void DeleteTagsForAuthor(const std::string& author_id) override {
        deleted_tags_for_author_.push_back(author_id);
    }

    void DeleteTagsForBook(const std::string& book_id) override {
        deleted_tags_for_book_.push_back(book_id);
        tags_by_book_.erase(book_id);
    }

    std::vector<std::string> GetTags(const std::string& book_id) const override {
        auto it = tags_by_book_.find(book_id);
        if (it == tags_by_book_.end()) {
            return {};
        }
        // set уже отсортирован, копируем в vector
        return {it->second.begin(), it->second.end()};
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

    REQUIRE(mock_authors.authors_by_id_.size() == 1);
    CHECK(mock_authors.authors_by_id_.begin()->second.GetName() == author_name);
    CHECK(mock_authors.authors_by_id_.begin()->second.GetId() != domain::AuthorId{});
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

    REQUIRE(mock_authors.authors_by_id_.size() == 3);
    
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

    auto deleted_tags = mock_tags.deleted_tags_for_book_;
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

    CHECK(mock_authors.deleted_ids_.size() == 1);
    CHECK(mock_books.deleted_books_by_author_.size() == 1);
    CHECK(mock_tags.deleted_tags_for_author_.size() == 1);
    
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
