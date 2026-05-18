#include "postgres.h"

#include <pqxx/zview.hxx>
#include <sstream>

namespace postgres {

    using namespace std::literals;
    using pqxx::operator"" _zv;

    void AuthorRepositoryImpl::Save(const domain::Author& author) {
        if (author.GetName().empty()) {
            throw std::logic_error("Author name cannot be empty");
        }
        work_.exec_params(
            R"(
    INSERT INTO authors (id, name) VALUES ($1, $2)
    ON CONFLICT (id) DO UPDATE SET name=$2;
    )"_zv,
            author.GetId().ToString(), author.GetName());
    }

    void AuthorRepositoryImpl::Delete(const std::string& author_id) {
        // Удаление автора по ID (параметризированный запрос для защиты от SQL injection)
        work_.exec_params("DELETE FROM authors WHERE id = $1;", author_id);
    }

    void AuthorRepositoryImpl::Edit(const info::AuthorInfo& author) {
        work_.exec_params("UPDATE authors SET name = $1 WHERE id = $2;", author.name, author.id);
    };

    info::Authors AuthorRepositoryImpl::GetAuthors() const {
        info::Authors res;
        auto query_text = "SELECT id, name FROM authors ORDER BY name"_zv;
        // Выполняем запрос и итерируемся по строкам ответа
        for (auto [id, name] : read_transaction_.query<std::string_view, std::string_view>(query_text)) {
            res.emplace_back(std::string(id), std::string(name));
        }
        return res;
    }

    std::optional<info::AuthorInfo> AuthorRepositoryImpl::
    GetAuthorByName(const std::string& author_name) const {
        // Поиск автора по имени (параметризированный запрос для защиты от SQL injection)
        auto query_text = "SELECT id, name FROM authors WHERE name = $1"_zv;
        auto result = read_transaction_.query01<std::string_view, std::string_view>(query_text, author_name);
        if (result) {
            auto [id, name] = *result;
            return {{.id = std::string(id), .name = std::string(name)}};
        };
        return std::nullopt;
    }

    void BookRepositoryImpl::Save(const domain::Book& book) {
        if (book.GetAuthorId().ToString().empty()) {
            throw std::logic_error("Book author ID cannot be empty");
        }
        if (book.GetTitle().empty()) {
            throw std::logic_error("Book title cannot be empty");
        }
        if (book.GetPublicationYear() <= 0 || book.GetPublicationYear() > 2100) {
            throw std::logic_error("Invalid publication year: must be between 1 and 2100");
        }
        work_.exec_params(
            R"(
    INSERT INTO Books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
    ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
    )"_zv,
            book.GetId().ToString(), book.GetAuthorId().ToString()
            , book.GetTitle(), book.GetPublicationYear());
    }

    void BookRepositoryImpl::DeleteBooks(const std::string& author_id) {
        // Удаление всех книг автора (параметризированный запрос)
        work_.exec_params("DELETE FROM books WHERE author_id = $1;", author_id);
    };

    void BookRepositoryImpl::DeleteBook(const std::string& book_id) {
        // Удаление одной книги по ID (параметризированный запрос)
        work_.exec_params("DELETE FROM books WHERE id = $1;", book_id);
    }

    void BookRepositoryImpl::EditBook(const info::BookInfo& book) {
        work_.exec_params("UPDATE books SET title = $1 "
            ", publication_year = $2 "
            "WHERE id = $3;", book.title, book.publication_year, book.id);
    }

    info::Books BookRepositoryImpl::GetBooks() const {
        info::Books res;
        auto query_text = "SELECT b.id as book_id, b.title, b.publication_year, a.name as author_name FROM books b "
        "INNER JOIN authors a on a.id = b.author_id "
        " ORDER BY b.title, a.name, b.publication_year";
        // Выполняем запрос и итерируемся по строкам ответа
        for (auto [book_id, title, year, author_name] : read_transaction_.query<std::string_view, std::string_view, int, std::string_view>(query_text)) {
            info::BookInfo book;
            book.id = book_id;
            book.title = title;
            book.author_name = author_name;
            book.publication_year = year;
            res.emplace_back(std::move(book));
        }
        return res;
    }

    info::BookInfo BookRepositoryImpl::GetBook(const std::string& book_id) const {
        // Получение книги по ID с информацией об авторе (параметризированный запрос)
        info::BookInfo res;
        auto query_text = "SELECT b.title, b.publication_year, a.name as author_name FROM books b "
                            "INNER JOIN authors a on a.id = b.author_id "
                            "WHERE b.id = $1"_zv;
        auto result = read_transaction_.query1<std::string_view, int, std::string_view>(query_text, book_id);
        auto [title, year, author_name] = result;
        res.id = book_id;
        res.title = title;
        res.publication_year = year;
        res.author_name = author_name;
        return res; 
    }

    info::Books BookRepositoryImpl::GetBooksByAuthor(const std::string& author_id) const {
        // Получение всех книг автора (параметризированный запрос, сортировка в БД)
        info::Books res;
        res.reserve(10);  // Предполагаем, что у автора не очень много книг
        auto query_text = "SELECT b.title, b.publication_year FROM books b "
            "WHERE b.author_id = $1 "
            "ORDER BY b.publication_year, b.title"_zv;
        for (auto [title, year] : read_transaction_.query<std::string_view, int>(query_text, author_id)) {
            res.emplace_back(std::string(title), year);
        }
        return res;
    }

    info::Books BookRepositoryImpl::GetBooksByTitle(const std::string& book_title) const {
        // Поиск книг по названию (параметризированный запрос, сортировка в БД)
        info::Books res;
        res.reserve(10);  // Предполагаем, что книг с одним названием не очень много
        auto query_text = "SELECT b.id as book_id, b.title, b.publication_year, a.name as author_name FROM books b "
                            "INNER JOIN authors a on a.id = b.author_id "
                            "WHERE b.title = $1 "
                            "ORDER BY b.title, a.name, b.publication_year"_zv;
        for (auto [book_id, title, year, author_name] : read_transaction_.query<std::string_view, std::string_view, int, std::string_view>(query_text, book_title)) {
            info::BookInfo book;
            book.id = book_id;
            book.title = title;
            book.author_name = author_name;
            book.publication_year = year;
            res.emplace_back(std::move(book));
        }
        return res;
    }

    TagRepositoryImpl::TagRepositoryImpl(pqxx::work& work, pqxx::read_transaction& read_transaction)
        : work_{work}
        , read_transaction_{read_transaction} {
    }

    void TagRepositoryImpl::Save(const std::vector<domain::Tag>& tags) {
        if (tags.empty()) {
            return;
        }
        // Используем ostringstream для эффективной сборки SQL (избегаем множественных realloc)
        std::ostringstream tag_values;
        bool first = true;
        for (const auto& tag : tags) {
            if (!first) {
                tag_values << ", ";
            }
            first = false;
            tag_values << "('" << tag.GetBookId().ToString() << "', '" << tag.GetTag() << "')";
        }
        work_.exec_params("INSERT INTO book_tags (book_id, tag) VALUES " + tag_values.str() + ";");
    }

    void TagRepositoryImpl::Save(const std::vector<std::string>& tags, const std::string& book_id) {
        if (tags.empty()) {
            return;
        }
        // Используем ostringstream для эффективной сборки SQL
        std::ostringstream tag_values;
        bool first = true;
        for (const auto& tag : tags) {
            if (!first) {
                tag_values << ", ";
            }
            first = false;
            tag_values << "('" << book_id << "', '" << tag << "')";
        }
        work_.exec_params("INSERT INTO book_tags (book_id, tag) VALUES " + tag_values.str() + ";");
    }

    void TagRepositoryImpl::DeleteTagsForAuthor(const std::string& author_id) {
        // Удаление всех тегов автора (через все его книги, параметризированный запрос)
        work_.exec_params("DELETE FROM book_tags WHERE book_id IN (SELECT id FROM books WHERE author_id = $1);", author_id);
    };

    void TagRepositoryImpl::DeleteTagsForBook(const std::string& book_id) {
        // Удаление всех тегов книги (параметризированный запрос)
        work_.exec_params("DELETE FROM book_tags WHERE book_id = $1;", book_id);
    };

    std::vector<std::string> TagRepositoryImpl::GetTags(const std::string& book_id) const {
        // Получение тегов книги (уже отсортированы в БД, параметризированный запрос)
        std::vector<std::string> res;
        auto query_text = "SELECT tag FROM book_tags WHERE book_id = $1 ORDER BY tag"_zv;
        for (auto [tag] : read_transaction_.query<std::string_view>(query_text, book_id)) {
            res.emplace_back(std::string(tag));
        }
        return res;
    }

    Database::Database(pqxx::connection connection)
        : connection_{std::move(connection)} 
        , uow_factory_{connection_} {
        
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
        author_id UUID NOT NULL ,
        title varchar(100) NOT NULL ,
        publication_year integer 
    ); 
    )"_zv);
        work.exec(R"(
    CREATE TABLE IF NOT EXISTS book_tags (
        book_id UUID NOT NULL ,
        tag varchar(30) NOT NULL 
    ); 
    )"_zv);
        work.commit();
    }

    UnitOfWorkImpl::UnitOfWorkImpl(pqxx::connection& connection, app::TypeOfTransaction type_of_tr) 
        : connection_(connection) {
        if (type_of_tr == app::TypeOfTransaction::Read) {
            read_transaction_ = std::make_unique<pqxx::read_transaction>(connection_);
        } else {
            work_ = std::make_unique<pqxx::work>(connection_);
        }
        authors_ = std::make_unique<AuthorRepositoryImpl>(*work_, *read_transaction_);
        books_ = std::make_unique<BookRepositoryImpl>(*work_, *read_transaction_);
        tags_ = std::make_unique<TagRepositoryImpl>(*work_, *read_transaction_);
    }

    void UnitOfWorkImpl::Commit() {
        work_->commit();
    }

    domain::AuthorRepository& UnitOfWorkImpl::Authors() {
        return *authors_;
    }

    domain::BookRepository& UnitOfWorkImpl::Books() {
        return *books_;
    }

    domain::TagRepository& UnitOfWorkImpl::Tags() {
        return *tags_;
    }

    UnitOfWorkFactoryImpl::UnitOfWorkFactoryImpl(pqxx::connection& connection) 
        : connection_(connection){
    }

    std::shared_ptr<app::UnitOfWork> UnitOfWorkFactoryImpl::CreateUnitOfWork(app::TypeOfTransaction type_of_tr) {
        return std::make_shared<UnitOfWorkImpl>(connection_, type_of_tr);
    }

}  // namespace postgres
