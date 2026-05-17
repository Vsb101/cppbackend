#pragma once
#include <pqxx/pqxx>
#include <pqxx/connection>
#include <pqxx/transaction>
#include <string>

#include "../domain/author.h"
#include "../domain/book.h"
#include "../domain/tag.h"
#include "../app/unit_of_work.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::work& work, pqxx::read_transaction& read_transaction)
        : work_{work}
        , read_transaction_{read_transaction} {
    }

    void Save(const domain::Author& author) override;
    void Delete(const std::string& author_id) override;
    void Edit(const info::AuthorInfo& author) override;
    info::Authors GetAuthors() const override;
    std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const override;
private:
    // pqxx::connection& connection_;
    pqxx::work& work_;
    pqxx::read_transaction& read_transaction_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::work& work, pqxx::read_transaction& read_transaction)
        : work_{work}
        , read_transaction_{read_transaction} {
    }

    void Save(const domain::Book& Book) override;
    void DeleteBooks(const std::string& author_id) override;
    void DeleteBook(const std::string& book_id) override;
    void EditBook(const info::BookInfo& book) override;
    info::Books GetBooks() const override;
    info::BookInfo GetBook(const std::string& book_id) const override;
    info::Books GetBooksByAuthor(const std::string& author_id) const override;
    info::Books GetBooksByTitle(const std::string& book_title) const override;

private:
    // pqxx::connection& connection_;
    pqxx::work& work_;
    pqxx::read_transaction& read_transaction_;
};

class TagRepositoryImpl : public domain::TagRepository {
public:
    explicit TagRepositoryImpl(pqxx::work& work, pqxx::read_transaction& read_transaction);
    void Save(const std::vector<domain::Tag>& tags) override;
    void Save(const std::vector<std::string>& tags, const std::string& book_id);
    void DeleteTagsForAuthor(const std::string& author_id) override;
    void DeleteTagsForBook(const std::string& book_id);
    std::vector<std::string> GetTags(const std::string&  book_id) const override;
private:
    pqxx::work& work_;
    pqxx::read_transaction& read_transaction_;
};


class UnitOfWorkImpl : public app::UnitOfWork {
public:
    explicit UnitOfWorkImpl(pqxx::connection& connection, app::TypeOfTransaction type_of_tr);
    void Commit() override;
    domain::AuthorRepository& Authors() override;
    domain::BookRepository& Books() override;
    domain::TagRepository& Tags() override;
private:
    pqxx::connection& connection_;
    std::unique_ptr<pqxx::work> work_;
    std::unique_ptr<pqxx::read_transaction> read_transaction_;
    std::unique_ptr<AuthorRepositoryImpl> authors_;
    std::unique_ptr<BookRepositoryImpl> books_;
    std::unique_ptr<TagRepositoryImpl> tags_;
};

class UnitOfWorkFactoryImpl : public app::UnitOfWorkFactory {
public:
    explicit UnitOfWorkFactoryImpl(pqxx::connection& connection);
    std::shared_ptr<app::UnitOfWork> CreateUnitOfWork(app::TypeOfTransaction type_of_tr) override;
private:
    pqxx::connection& connection_;
};


class Database {
public:
    explicit Database(pqxx::connection connection);

    // AuthorRepositoryImpl& GetAuthors() & {
    //     return authors_;
    // }

    // BookRepositoryImpl& GetBooks() & {
    //     return books_;
    // }
    
    app::UnitOfWorkFactory* GetUowFactory()  {
        return &uow_factory_;
    }
private:
    pqxx::connection connection_;
    // AuthorRepositoryImpl authors_{connection_};
    // BookRepositoryImpl books_{connection_};
    UnitOfWorkFactoryImpl uow_factory_;
};


}  // namespace postgres