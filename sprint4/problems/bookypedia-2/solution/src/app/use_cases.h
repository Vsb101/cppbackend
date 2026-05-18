#pragma onc
#include "../domain/author.h"
#include "../domain/book.h"
#include "../domain/tag.h"
#include "../ui/struct_info.h"
#include "../app/unit_of_work.h"
#include <vector>
#include <optional>
#include <string>

namespace app {

// Интерфейс для использования предметной области (Use Cases)
// Определяет бизнес-логику приложения
class UseCases {
public:
    // Добавление нового автора
    virtual void AddAuthor(const std::string& name) = 0;
    // Добавление новой книги с тегами
    // autor_id - ID существующего автора (непустой) или пустой для нового
    // autor_name - имя автора (используется если autor_id пуст)
    virtual void AddBook(const std::string& autor_id, const std::string& autor_name
        , const std::string& title, const int publication_year, const std::vector<std::string>& tags) = 0;
    // Редактирование информации об авторе
    virtual void EditAuthor(const info::AuthorInfo& author) const = 0;
    // Получение списка всех авторов
    virtual info::Authors GetAuthors() const = 0;
    // Получение списка всех книг (с авторами)
    virtual info::Books GetBooks() const = 0;
    // Получение подробной информации о книге (включая теги)
    virtual info::BookInfo GetBook(const std::string& book_id) const = 0;
    // Удаление книги
    virtual void DeleteBook(const std::string& book_id) const = 0;
    // Редактирование книги (название, год, теги)
    virtual void EditBook(const info::BookInfo& book) const = 0;
    // Получение книг автора
    virtual info::Books GetAuthorBooks(const std::string& author_id) const = 0;
    // Удаление автора и всех его книг с тегами
    virtual void DeleteAuthor(const std::string& author_id) const = 0;
    // Поиск автора по имени
    virtual std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const = 0;
    // Поиск книг по названию (может вернуть несколько)
    virtual info::Books GetBooksByTitle(const std::string& book_title) const = 0;

protected:
    ~UseCases() = default;
};

// Реализация UseCases с использованием UnitOfWork
// Каждая операция выполняется в отдельной транзакции
class UseCasesImpl : public UseCases {
public:
    // Конструктор с фабрикой UnitOfWork
    explicit UseCasesImpl(UnitOfWorkFactory* uow_factory)
        : uow_factory_{uow_factory} {
    }

    // Добавление автора
    // Создаёт нового автора с генерацией уникального ID
    void AddAuthor(const std::string& name) override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Authors().Save({domain::AuthorId::New(), name});
        uo_work->Commit();
    }

    // Добавление книги
    // Если autor_id пуст, создаёт нового автора с именем autor_name
    // Сохраняет книгу и все указанные теги
    void AddBook(const std::string& autor_id, const std::string& autor_name
        , const std::string& title, const int publication_year, const std::vector<std::string>& tags) override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        domain::AuthorId autor_id_obj;
        if (!autor_id.empty()) {
            autor_id_obj = domain::AuthorId::FromString(autor_id);
        } else {
            // Если ID не указан, создаём нового автора
            autor_id_obj = domain::AuthorId::New();
            uo_work->Authors().Save({autor_id_obj, autor_name});
        }
        domain::BookId book_id_obj = domain::BookId::New();
        uo_work->Books().Save({book_id_obj, autor_id_obj, title, publication_year});
        std::vector<domain::Tag> tags_obj;
        for (const auto& tag : tags) {
            tags_obj.emplace_back(book_id_obj, tag);
        }
        uo_work->Tags().Save(tags_obj);
        uo_work->Commit();
    }

    // Редактирование автора
    void EditAuthor(const info::AuthorInfo& author) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Authors().Edit(author);
        uo_work->Commit();
    }

    // Получение списка всех авторов
    info::Authors GetAuthors() const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Authors().GetAuthors();
    }

    // Получение списка всех книг с авторами
    info::Books GetBooks() const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Books().GetBooks();
    }

    // Получение полной информации о книге (включая теги)
    info::BookInfo GetBook(const std::string& book_id) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        auto book_info = uo_work->Books().GetBook(book_id);
        book_info.tags = uo_work->Tags().GetTags(book_id);
        return book_info;
    }

    // Удаление книги и её тегов
    void DeleteBook(const std::string& book_id) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Tags().DeleteTagsForBook(book_id);
        uo_work->Books().DeleteBook(book_id);
        uo_work->Commit();
    }

    // Редактирование книги (название, год, теги)
    // Заменяет все теги книги на новые
    void EditBook(const info::BookInfo& book) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Tags().DeleteTagsForBook(book.id);
        uo_work->Tags().Save(book.tags, book.id);
        uo_work->Books().EditBook(book);
        uo_work->Commit();
    }

    // Получение всех книг автора
    info::Books GetAuthorBooks(const std::string& author_id) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Books().GetBooksByAuthor(author_id);
    }

    // Удаление автора и каскадное удаление всех его книг и тегов
    void DeleteAuthor(const std::string& author_id) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Tags().DeleteTagsForAuthor(author_id);
        uo_work->Books().DeleteBooks(author_id);
        uo_work->Authors().Delete(author_id);
        uo_work->Commit();
    }

    // Поиск автора по имени
    std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Authors().GetAuthorByName(author_name);
    }

    // Поиск книг по названию
    info::Books GetBooksByTitle(const std::string& book_title) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Books().GetBooksByTitle(book_title);
    }

private:
    UnitOfWorkFactory* uow_factory_;
};

}  // namespace app
