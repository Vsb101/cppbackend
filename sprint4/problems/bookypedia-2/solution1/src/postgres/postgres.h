#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>
#include <memory>
#include <optional>
#include <vector>
#include <string>
#include <unordered_map>

#include "../domain/author.h"

namespace postgres {

class TransactionContext {
public:
    explicit TransactionContext(pqxx::connection& conn) : tx_(conn) {}
    pqxx::work& GetTx() { return tx_; }
    void Commit() { tx_.commit(); }
private:
    pqxx::work tx_;
};

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection) : connection_{connection} {}

    void SetCurrentTx(pqxx::work* tx) { current_tx_ = tx; }

    void Save(const domain::Author& author) override;
    std::vector<domain::Author> GetAll() const override;
    std::optional<domain::Author> LoadById(const domain::AuthorId& id) const override;
    std::optional<domain::Author> LoadByName(const std::string& name) const override;
    void Delete(const domain::AuthorId& id) override;
    void Update(const domain::Author& author) override;
    std::vector<domain::Book> GetAuthorBooks(const domain::AuthorId& author_id) const override;

private:
    pqxx::connection& connection_;
    pqxx::work* current_tx_ = nullptr;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& connection) : connection_{connection} {}

    void SetCurrentTx(pqxx::work* tx) { current_tx_ = tx; }

    void Save(const domain::Book& book) override;
    std::vector<domain::Book> GetAll() const override;
    std::vector<domain::Book> GetByAuthorId(const domain::AuthorId& author_id) const override;
    std::optional<domain::Book> LoadById(const domain::BookId& id) const override;
    std::vector<domain::Book> GetByTitle(const std::string& title) const override;
    void Delete(const domain::BookId& id) override;
    void Update(const domain::Book& book) override;
    std::vector<std::string> GetBookTags(const domain::BookId& book_id) const override;
    void SetBookTags(const domain::BookId& book_id, const std::vector<std::string>& tags) override;
    void DeleteBookTags(const domain::BookId& book_id) override;
    std::unordered_map<std::string, std::vector<std::string>> GetBooksTagsBatch(
        const std::vector<std::string>& book_ids) const override;

private:
    pqxx::connection& connection_;
    pqxx::work* current_tx_ = nullptr;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & { return authors_; }
    BookRepositoryImpl& GetBooks() & { return books_; }

    template <typename Block>
    void ExecuteTransaction(Block&& block) {
        pqxx::work tx{connection_};
        authors_.SetCurrentTx(&tx);
        books_.SetCurrentTx(&tx);
        try {
            block();
            tx.commit();
        } catch (...) {
            authors_.SetCurrentTx(nullptr);
            books_.SetCurrentTx(nullptr);
            throw;
        }
        authors_.SetCurrentTx(nullptr);
        books_.SetCurrentTx(nullptr);
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl books_{connection_};
};

}  // namespace postgres