#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <stdexcept>
#include <algorithm>

#include "../src/app/use_cases_impl.h"
#include "../src/domain/author.h"

namespace {

using namespace domain;

// Мок для AuthorRepository
struct MockAuthorRepository : AuthorRepository {
    std::vector<Author> authors;
    
    void Save(const Author& author) override {
        authors.push_back(author);
    }
    
    std::vector<Author> GetAll() const override {
        return authors;
    }
    
    std::optional<Author> LoadById(const AuthorId& id) const override {
        for (const auto& a : authors) {
            if (a.GetId() == id) return a;
        }
        return std::nullopt;
    }
    
    std::optional<Author> LoadByName(const std::string& name) const override {
        for (const auto& a : authors) {
            if (a.GetName() == name) return a;
        }
        return std::nullopt;
    }
    
    void Delete(const AuthorId& id) override {
        auto it = std::find_if(authors.begin(), authors.end(),
            [&id](const auto& a) { return a.GetId() == id; });
        if (it != authors.end()) authors.erase(it);
    }
    
    void Update(const Author& author) override {
        for (auto& a : authors) {
            if (a.GetId() == author.GetId()) {
                a = author;
                break;
            }
        }
    }
    
    std::vector<Book> GetAuthorBooks(const AuthorId& author_id) const override {
        return {};
    }
};

// Мок для BookRepository
struct MockBookRepository : BookRepository {
    std::vector<Book> books;
    std::unordered_map<std::string, std::vector<std::string>> tags_map;
    
    void Save(const Book& book) override {
        books.push_back(book);
    }
    
    std::vector<Book> GetAll() const override {
        return books;
    }
    
    std::vector<Book> GetByAuthorId(const AuthorId& author_id) const override {
        std::vector<Book> result;
        for (const auto& b : books) {
            if (b.GetAuthorId() == author_id) result.push_back(b);
        }
        return result;
    }
    
    std::optional<Book> LoadById(const BookId& id) const override {
        for (const auto& b : books) {
            if (b.GetId() == id) return b;
        }
        return std::nullopt;
    }
    
    std::vector<Book> GetByTitle(const std::string& title) const override {
        std::vector<Book> result;
        for (const auto& b : books) {
            if (b.GetTitle() == title) result.push_back(b);
        }
        return result;
    }
    
    void Delete(const BookId& id) override {
        auto it = std::find_if(books.begin(), books.end(),
            [&id](const auto& b) { return b.GetId() == id; });
        if (it != books.end()) books.erase(it);
        tags_map.erase(id.ToString());
    }
    
    void Update(const Book& book) override {
        for (auto& b : books) {
            if (b.GetId() == book.GetId()) {
                b = book;
                break;
            }
        }
    }
    
    std::vector<std::string> GetBookTags(const BookId& book_id) const override {
        auto it = tags_map.find(book_id.ToString());
        if (it != tags_map.end()) return it->second;
        return {};
    }
    
    void SetBookTags(const BookId& book_id, const std::vector<std::string>& tags) override {
        tags_map[book_id.ToString()] = tags;
    }
    
    void DeleteBookTags(const BookId& book_id) override {
        tags_map.erase(book_id.ToString());
    }
    
    std::unordered_map<std::string, std::vector<std::string>> GetBooksTagsBatch(
        const std::vector<std::string>& book_ids) const override {
        
        std::unordered_map<std::string, std::vector<std::string>> result;
        for (const auto& id : book_ids) {
            auto it = tags_map.find(id);
            if (it != tags_map.end()) {
                result[id] = it->second;
            }
        }
        return result;
    }
};

} // anonymous namespace

SCENARIO("Add author") {
    GIVEN("Empty repository") {
        auto authors = std::make_shared<MockAuthorRepository>();
        auto books = std::make_shared<MockBookRepository>();
        app::UseCasesImpl use_cases(authors, books);
        
        WHEN("Adding an author") {
            use_cases.AddAuthor("John Doe");
            
            THEN("Author is added") {
                auto all_authors = use_cases.GetAuthors();
                REQUIRE(all_authors.size() == 1);
                REQUIRE(all_authors[0].GetName() == "John Doe");
            }
        }
        
        WHEN("Adding duplicate author") {
            use_cases.AddAuthor("John Doe");
            
            THEN("Throws exception") {
                REQUIRE_THROWS_AS(use_cases.AddAuthor("John Doe"), std::runtime_error);
            }
        }
        
        WHEN("Adding empty name") {
            THEN("Throws exception") {
                REQUIRE_THROWS_AS(use_cases.AddAuthor(""), std::invalid_argument);
            }
        }
    }
}

SCENARIO("Add book with author selection") {
    GIVEN("Repository with existing author") {
        auto authors = std::make_shared<MockAuthorRepository>();
        auto books = std::make_shared<MockBookRepository>();
        app::UseCasesImpl use_cases(authors, books);
        
        use_cases.AddAuthor("Jane Austen");
        
        WHEN("Adding book with existing author name") {
            auto result = use_cases.AddBookWithAuthorSelection(
                "Pride and Prejudice", 1813, "Jane Austen", {"classic", "romance"});
            
            THEN("Book is added and author not added") {
                REQUIRE_FALSE(result.author_added);
                auto all_books = use_cases.GetBooks();
                REQUIRE(all_books.size() == 1);
                REQUIRE(all_books[0].GetTitle() == "Pride and Prejudice");
                
                auto tags = use_cases.GetBookTags(all_books[0].GetId().ToString());
                REQUIRE(tags.size() == 2);
                REQUIRE(std::find(tags.begin(), tags.end(), "classic") != tags.end());
                REQUIRE(std::find(tags.begin(), tags.end(), "romance") != tags.end());
            }
        }
        
        WHEN("Adding book with new author") {
            auto result = use_cases.AddBookWithAuthorSelection(
                "Emma", 1815, "Jane Austen", {});
            
            THEN("Book is added") {
                REQUIRE_FALSE(result.author_added);
                auto all_books = use_cases.GetBooks();
                REQUIRE(all_books.size() == 1);
            }
        }
    }
    
    GIVEN("Empty repository") {
        auto authors = std::make_shared<MockAuthorRepository>();
        auto books = std::make_shared<MockBookRepository>();
        app::UseCasesImpl use_cases(authors, books);
        
        WHEN("Adding book with new author") {
            auto result = use_cases.AddBookWithAuthorSelection(
                "1984", 1949, "George Orwell", {"dystopian"});
            
            THEN("Book and author are added") {
                REQUIRE(result.author_added);
                auto all_authors = use_cases.GetAuthors();
                REQUIRE(all_authors.size() == 1);
                REQUIRE(all_authors[0].GetName() == "George Orwell");
                
                auto all_books = use_cases.GetBooks();
                REQUIRE(all_books.size() == 1);
                REQUIRE(all_books[0].GetTitle() == "1984");
            }
        }
        
        WHEN("Adding book with empty title") {
            THEN("Throws exception") {
                REQUIRE_THROWS_AS(
                    use_cases.AddBookWithAuthorSelection("", 1949, "George Orwell", {}),
                    std::invalid_argument);
            }
        }
    }
}

SCENARIO("Get books") {
    GIVEN("Repository with multiple books") {
        auto authors = std::make_shared<MockAuthorRepository>();
        auto books = std::make_shared<MockBookRepository>();
        app::UseCasesImpl use_cases(authors, books);
        
        use_cases.AddAuthor("Leo Tolstoy");
        use_cases.AddAuthor("Fyodor Dostoevsky");
        
        auto author_id_tolstoy = use_cases.GetAuthors()[0].GetId();
        auto author_id_dostoevsky = use_cases.GetAuthors()[1].GetId();
        
        use_cases.AddBookWithAuthorSelection(
            "War and Peace", 1869, "Leo Tolstoy", {"epic", "historical"});
        use_cases.AddBookWithAuthorSelection(
            "Anna Karenina", 1877, "Leo Tolstoy", {"romance"});
        use_cases.AddBookWithAuthorSelection(
            "Crime and Punishment", 1866, "Fyodor Dostoevsky", {"psychological"});
        
        WHEN("Getting all books") {
            auto all_books = use_cases.GetBooks();
            
            THEN("Returns all books") {
                REQUIRE(all_books.size() == 3);
            }
        }
        
        WHEN("Getting books by author") {
            auto tolstoy_books = use_cases.GetAuthorBooks(author_id_tolstoy.ToString());
            
            THEN("Returns only author's books") {
                REQUIRE(tolstoy_books.size() == 2);
                bool has_war_and_peace = false;
                bool has_anna_karenina = false;
                for (const auto& book : tolstoy_books) {
                    if (book.GetTitle() == "War and Peace") has_war_and_peace = true;
                    if (book.GetTitle() == "Anna Karenina") has_anna_karenina = true;
                }
                REQUIRE(has_war_and_peace);
                REQUIRE(has_anna_karenina);
            }
        }
        
        WHEN("Getting books by title") {
            auto books_by_title = use_cases.GetBooksByTitle("War and Peace");
            
            THEN("Returns correct book") {
                REQUIRE(books_by_title.size() == 1);
                REQUIRE(books_by_title[0].GetTitle() == "War and Peace");
            }
        }
    }
}

SCENARIO("Batch get book tags") {
    GIVEN("Repository with books having tags") {
        auto authors = std::make_shared<MockAuthorRepository>();
        auto books = std::make_shared<MockBookRepository>();
        app::UseCasesImpl use_cases(authors, books);
        
        use_cases.AddAuthor("Test Author");
        
        auto result1 = use_cases.AddBookWithAuthorSelection(
            "Book1", 2000, "Test Author", {"tag1", "tag2"});
        auto result2 = use_cases.AddBookWithAuthorSelection(
            "Book2", 2001, "Test Author", {"tag2", "tag3"});
        
        std::vector<std::string> book_ids = {
            result1.book.GetId().ToString(),
            result2.book.GetId().ToString()
        };
        
        WHEN("Getting tags in batch") {
            auto tags_map = use_cases.GetBooksTagsBatch(book_ids);
            
            THEN("Returns tags for all books") {
                REQUIRE(tags_map.size() == 2);
                REQUIRE(tags_map[book_ids[0]].size() == 2);
                REQUIRE(tags_map[book_ids[1]].size() == 2);
            }
        }
    }
}

SCENARIO("Delete author") {
    GIVEN("Repository with author and books") {
        auto authors = std::make_shared<MockAuthorRepository>();
        auto books = std::make_shared<MockBookRepository>();
        app::UseCasesImpl use_cases(authors, books);
        
        use_cases.AddAuthor("J.K. Rowling");
        use_cases.AddBookWithAuthorSelection(
            "Harry Potter", 1997, "J.K. Rowling", {"fantasy"});
        
        WHEN("Deleting author by name") {
            bool deleted = use_cases.DeleteAuthor("J.K. Rowling");
            
            THEN("Author and books are deleted") {
                REQUIRE(deleted);
                auto all_authors = use_cases.GetAuthors();
                REQUIRE(all_authors.empty());
                auto all_books = use_cases.GetBooks();
                REQUIRE(all_books.empty());
            }
        }
        
        WHEN("Deleting non-existent author") {
            bool deleted = use_cases.DeleteAuthor("Nonexistent");
            
            THEN("Returns false") {
                REQUIRE_FALSE(deleted);
            }
        }
    }
}

SCENARIO("Edit author") {
    GIVEN("Repository with author") {
        auto authors = std::make_shared<MockAuthorRepository>();
        auto books = std::make_shared<MockBookRepository>();
        app::UseCasesImpl use_cases(authors, books);
        
        use_cases.AddAuthor("Old Name");
        
        WHEN("Editing author name") {
            bool edited = use_cases.EditAuthor("Old Name", "New Name");
            
            THEN("Author name is updated") {
                REQUIRE(edited);
                auto all_authors = use_cases.GetAuthors();
                REQUIRE(all_authors.size() == 1);
                REQUIRE(all_authors[0].GetName() == "New Name");
            }
        }
        
        WHEN("Editing with empty new name") {
            bool edited = use_cases.EditAuthor("Old Name", "");
            
            THEN("Returns false") {
                REQUIRE_FALSE(edited);
            }
        }
    }
}

SCENARIO("Delete book") {
    GIVEN("Repository with book") {
        auto authors = std::make_shared<MockAuthorRepository>();
        auto books = std::make_shared<MockBookRepository>();
        app::UseCasesImpl use_cases(authors, books);
        
        use_cases.AddAuthor("Author");
        auto result = use_cases.AddBookWithAuthorSelection(
            "Book", 2000, "Author", {"tag"});
        
        WHEN("Deleting book by ID") {
            bool deleted = use_cases.DeleteBook(result.book.GetId().ToString());
            
            THEN("Book is deleted") {
                REQUIRE(deleted);
                auto all_books = use_cases.GetBooks();
                REQUIRE(all_books.empty());
            }
        }
    }
}

SCENARIO("Edit book") {
    GIVEN("Repository with book") {
        auto authors = std::make_shared<MockAuthorRepository>();
        auto books = std::make_shared<MockBookRepository>();
        app::UseCasesImpl use_cases(authors, books);
        
        use_cases.AddAuthor("Author");
        auto result = use_cases.AddBookWithAuthorSelection(
            "Old Title", 2000, "Author", {"old_tag"});
        
        WHEN("Editing book") {
            bool edited = use_cases.EditBook(
                result.book.GetId().ToString(), 
                "New Title", 
                2001, 
                {"new_tag1", "new_tag2"});
            
            THEN("Book is updated") {
                REQUIRE(edited);
                auto books_by_title = use_cases.GetBooksByTitle("New Title");
                REQUIRE(books_by_title.size() == 1);
                REQUIRE(books_by_title[0].GetPublicationYear() == 2001);
                
                auto tags = use_cases.GetBookTags(result.book.GetId().ToString());
                REQUIRE(tags.size() == 2);
            }
        }
    }
}