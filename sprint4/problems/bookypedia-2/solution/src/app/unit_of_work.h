#pragma once

#include <memory>

namespace domain {

class AuthorRepository;
class BookRepository;
class TagRepository;

}  // namespace domain

namespace app {

// Тип транзакции: только чтение или запись
enum class TypeOfTransaction {
    Read,   // Для операций чтения
    Write   // Для операций изменения данных
};

// Unit of Work - паттерн для управления транзакциями
// Обеспечивает атомарность операций (все или ничего)
class UnitOfWork {
public:
    // Завершение транзакции (commit)
    virtual void Commit() = 0;
    // Получение репозитория авторов
    virtual domain::AuthorRepository& Authors() = 0;
    // Получение репозитория книг
    virtual domain::BookRepository& Books() = 0;
    // Получение репозитория тегов
    virtual domain::TagRepository& Tags() = 0;

protected:
    ~UnitOfWork() = default;
};

// Фабрика для создания UnitOfWork
class UnitOfWorkFactory {
public:
    // Создание "работы" с указанным типом транзакции
    virtual std::shared_ptr<UnitOfWork> CreateUnitOfWork(TypeOfTransaction type_of_tr) = 0;

protected:
    ~UnitOfWorkFactory() = default;
};

}  // namespace app