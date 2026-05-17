#include "postgres.h"

#include <pqxx/zview.hxx>

namespace postgres {

    using namespace std::literals;
    using pqxx::operator"" _zv;

    void AuthorRepositoryImpl::Save(const domain::Author& author) {
        // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
        // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
        // запросов выполнить в рамках одной транзакции.
        // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
        if(author.GetName().empty()){
            throw std::logic_error("");
        }
        work_.exec_params(
            R"(
    INSERT INTO authors (id, name) VALUES ($1, $2)
    ON CONFLICT (id) DO UPDATE SET name=$2;
    )"_zv,
            author.GetId().ToString(), author.GetName());
    }

    void AuthorRepositoryImpl::Delete(const std::string& author_id) {
        work_.exec_params("DELETE FROM authors WHERE id = '" + author_id + "';");
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
        auto query_text = "SELECT id, name FROM authors where name = '" + author_name + "'";
        auto result = read_transaction_.query01<std::string_view, std::string_view>(query_text);
        if (result) {
            auto [id, name] = *result;
            return {{.id = std::string(id), .name = std::string(name)}};
        };
        return std::nullopt;
    }


    void BookRepositoryImpl::Save(const domain::Book& Book) {
        if(Book.GetAuthorId().ToString().empty() || Book.GetTitle().empty() || Book.GetPublicationYear() <= 0){
            throw std::logic_error("");
        }
        work_.exec_params(
            R"(
    INSERT INTO Books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
    ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
    )"_zv,
            Book.GetId().ToString(), Book.GetAuthorId().ToString()
            , Book.GetTitle(), Book.GetPublicationYear());
    }

    void BookRepositoryImpl::DeleteBooks(const std::string& author_id) {
        work_.exec_params("DELETE FROM books WHERE author_id = '" + author_id + "';");
    };

    void BookRepositoryImpl::DeleteBook(const std::string& book_id) {
        work_.exec_params("DELETE FROM books WHERE id = '" + book_id + "';");
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
        info::BookInfo res;
        auto query_text = "SELECT b.title, b.publication_year, a.name as author_name FROM books b "
                            "INNER JOIN authors a on a.id = b.author_id "
                            "WHERE b.id = '" + book_id + "';";
        auto result = read_transaction_.query1<std::string_view, int, std::string_view>(query_text);
        auto [title, year, author_name] = result;
        res.id = book_id;
        res.title = title;
        res.publication_year = year;
        res.author_name = author_name;
        return res; 
    }

    info::Books BookRepositoryImpl::GetBooksByAuthor(const std::string& author_id) const {
        info::Books res;
        auto query_text = "SELECT title, publication_year FROM books WHERE author_id = '" 
        + author_id + "' ORDER BY publication_year, title";
        // Выполняем запрос и итерируемся по строкам ответа
        for (auto [title, year, author_name] : read_transaction_.query<std::string_view, int, std::string_view>(query_text)) {
            res.emplace_back(std::string(title), year);
        }
        return res;
    }

    info::Books BookRepositoryImpl::GetBooksByTitle(const std::string& book_title) const {
        info::Books res;
        auto query_text = "SELECT b.id as book_id, b.title, b.publication_year, a.name as author_name FROM books b "
                            "INNER JOIN authors a on a.id = b.author_id "
                            "WHERE b.title = '" + book_title + "' ";
                            "ORDER BY b.title, a.name, b.publication_year;";
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

    TagRepositoryImpl::TagRepositoryImpl(pqxx::work& work, pqxx::read_transaction& read_transaction)
        : work_{work}
        , read_transaction_{read_transaction} {
    }

    void TagRepositoryImpl::Save(const std::vector<domain::Tag>& tags) {
        if (tags.empty()) {
            return;
        }
        std::string tag_values;
        for (const auto& tag : tags) {
            tag_values.append("('"+tag.GetBookId().ToString()+"', '"+tag.GetTag()+"'), ");
        }
        if (tag_values.size()>1) {
            tag_values.erase(tag_values.size()-2);
        }
        work_.exec_params("INSERT INTO book_tags (book_id, tag) VALUES " + tag_values + ";");
    }

    void TagRepositoryImpl::Save(const std::vector<std::string>& tags, const std::string& book_id) {
        if (tags.empty()) {
            return;
        }
        std::string tag_values;
        for (const auto& tag : tags) {
            tag_values.append("('"+book_id+"', '"+tag+"'), ");
        }
        if (tag_values.size()>1) {
            tag_values.erase(tag_values.size()-2);
        }
        work_.exec_params("INSERT INTO book_tags (book_id, tag) VALUES " + tag_values + ";");
    }

    void TagRepositoryImpl::DeleteTagsForAuthor(const std::string& author_id) {
        work_.exec_params("DELETE FROM book_tags WHERE book_id in (SELECT id FROM books WHERE author_id = '" + author_id + "');");
    };

    void TagRepositoryImpl::DeleteTagsForBook(const std::string& book_id) {
        work_.exec_params("DELETE FROM book_tags WHERE book_id = '" + book_id + "';");
    };


    std::vector<std::string> TagRepositoryImpl::GetTags(const std::string&  book_id) const {
        std::vector<std::string> res;
        auto query_text = "SELECT tag FROM book_tags"
        " WHERE book_id = '" + book_id + "' ORDER BY tag;";
        // Выполняем запрос и итерируемся по строкам ответа
        for (auto [tag] : read_transaction_.query<std::string_view>(query_text)) {
            res.emplace_back(std::string(tag));
        }
        return res;
    }


    Database::Database(pqxx::connection connection)
        : connection_{std::move(connection)} 
        , uow_factory_{connection_}
        {
        // auto uow = uow_factory_.CreateUnitOfWork();
        // uow->
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
        // коммитим изменения
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

        // try{
        //     pqxx::read_transaction read_transaction_(connection_);
        // pqxx::work work_(connection_);
        // pqxx::work work_2(connection_);

        //     UnitOfWorkImpl work1(connection_);
        // } catch (const std::exception& e ) {
        //     std::string mes = e.what();
        //     std::cout << mes;
        // }
        // auto work2 = std::make_shared<UnitOfWorkImpl>(connection_, type_of_tr);
        return std::make_shared<UnitOfWorkImpl>(connection_, type_of_tr);
    }


}  // namespace postgres
