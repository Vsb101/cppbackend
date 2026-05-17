#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>
#include <memory>
#include <optional>
#include <vector>
#include <string>

#include "../domain/author.h"

namespace postgres {

// Класс-обёртка для транзакции контекста Unit of Work
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

    // Позволяет UseCases передать текущую активную транзакцию
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
    pqxx::work* current_tx_ = nullptr; // Ссылка на глобальную транзакцию команды
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

private:
    pqxx::connection& connection_;
    pqxx::work* current_tx_ = nullptr;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & { return authors_; }
    BookRepositoryImpl& GetBooks() & { return books_; }

    // Метод для UseCases, чтобы выполнять команды атомарно
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
            throw; // Пробрасываем ошибку дальше для UI слоя
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
