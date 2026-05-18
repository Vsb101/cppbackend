#pragma once
#include "../domain/author.h"
#include "../domain/book.h"
#include "../domain/tag.h"
#include "../ui/struct_info.h"
#include "../app/unit_of_work.h"
#include <vector>
#include <optional>
#include <string>

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(const std::string& autor_id, const std::string& autor_name
        , const std::string& title, const int publication_year, const std::vector<std::string>& tags) = 0;
    virtual void EditAuthor(const info::AuthorInfo& author) const = 0;
    virtual info::Authors GetAuthors() const = 0;
    virtual info::Books GetBooks() const = 0;
    virtual info::BookInfo GetBook(const std::string& book_id) const = 0;
    virtual void DeleteBook(const std::string& book_id) const = 0;
    virtual void EditBook(const info::BookInfo& book) const = 0;
    virtual info::Books GetAuthorBooks(const std::string& author_id) const = 0;
    virtual void DeleteAuthor(const std::string& author_id) const = 0;
    virtual std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const = 0;
    virtual info::Books GetBooksByTitle(const std::string& book_title) const = 0;
protected:
    ~UseCases() = default;
};

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(UnitOfWorkFactory* uow_factory)
        : uow_factory_{uow_factory} {
    }

    void AddAuthor(const std::string& name) override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Authors().Save({domain::AuthorId::New(), name});
        uo_work->Commit();
    }

    void AddBook(const std::string& autor_id, const std::string& autor_name
        , const std::string& title, const int publication_year, const std::vector<std::string>& tags) override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        domain::AuthorId autor_id_obj;
        if (!autor_id.empty()) {
            autor_id_obj = domain::AuthorId::FromString(autor_id);
        } else {
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

    void EditAuthor(const info::AuthorInfo& author) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Authors().Edit(author);
        uo_work->Commit();
    }

    info::Authors GetAuthors() const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Authors().GetAuthors();
    }

    info::Books GetBooks() const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Books().GetBooks();
    }

    info::BookInfo GetBook(const std::string& book_id) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        auto book_info = uo_work->Books().GetBook(book_id);
        book_info.tags = uo_work->Tags().GetTags(book_id);
        return book_info;
    }

    void DeleteBook(const std::string& book_id) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Tags().DeleteTagsForBook(book_id);
        uo_work->Books().DeleteBook(book_id);
        uo_work->Commit();
    }

    void EditBook(const info::BookInfo& book) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Tags().DeleteTagsForBook(book.id);
        uo_work->Tags().Save(book.tags, book.id);
        uo_work->Books().EditBook(book);
        uo_work->Commit();
    }

    info::Books GetAuthorBooks(const std::string& author_id) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Books().GetBooksByAuthor(author_id);
    }

    void DeleteAuthor(const std::string& author_id) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Tags().DeleteTagsForAuthor(author_id);
        uo_work->Books().DeleteBooks(author_id);
        uo_work->Authors().Delete(author_id);
        uo_work->Commit();
    }

    std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Authors().GetAuthorByName(author_name);
    }

    info::Books GetBooksByTitle(const std::string& book_title) const override {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Books().GetBooksByTitle(book_title);
    }

private:
    UnitOfWorkFactory* uow_factory_;
};

}  // namespace app
