#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"
#include "../domain/tag.h"

namespace app {
using namespace domain;

    void UseCasesImpl::AddAuthor(const std::string& name) {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Authors().Save({AuthorId::New(), name});
        uo_work->Commit();
    }


    void UseCasesImpl::AddBook(const std::string& autor_id, const std::string& autor_name
        , const std::string& title, const int publication_year, const std::vector<std::string>& tags) 
        {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        // Если есть ИД автора - берем это ИД
        AuthorId autor_id_obj;
        if (!autor_id.empty()) {
            autor_id_obj = AuthorId::FromString(autor_id);
        } else { // Если нет ИД генерируем новое и сохраняем нового автора
            autor_id_obj = AuthorId::New();
            uo_work->Authors().Save({autor_id_obj, autor_name});
        }
        // Сохраняем книгу
        BookId book_id_obj = BookId::New();
        uo_work->Books().Save({book_id_obj, autor_id_obj, title, publication_year}); 
        // Сохраняем теги для книги
        std::vector<domain::Tag> tags_obj;
        for (const auto& tag : tags) {
            tags_obj.emplace_back(book_id_obj, tag);
        } 
        uo_work->Tags().Save(tags_obj); 
        // Завершаем транзакцию
        uo_work->Commit();
    }

    void UseCasesImpl::EditAuthor(const info::AuthorInfo& author) const {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Authors().Edit(author);
        uo_work->Commit();
    }

    info::Authors UseCasesImpl::GetAuthors() const {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Authors().GetAuthors();
    }

    info::Books UseCasesImpl::GetBooks() const {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Books().GetBooks();
    }

    info::BookInfo UseCasesImpl::GetBook(const std::string& book_id) const {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        auto book_info = uo_work->Books().GetBook(book_id);
        book_info.tags = uo_work->Tags().GetTags(book_id);
        return book_info;
    }

    void UseCasesImpl::DeleteBook(const std::string& book_id) const {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Tags().DeleteTagsForBook(book_id);
        uo_work->Books().DeleteBook(book_id);
        uo_work->Commit();

    }

    void UseCasesImpl::EditBook(const info::BookInfo& book) const {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Tags().DeleteTagsForBook(book.id);
        uo_work->Tags().Save(book.tags, book.id);
        uo_work->Books().EditBook(book);

        uo_work->Commit();

    }

    info::Books UseCasesImpl::GetAuthorBooks(const std::string& author_id) const {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Books().GetBooksByAuthor(author_id);
    }

    void UseCasesImpl::DeleteAuthor(const std::string& author_id) const {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Write);
        uo_work->Tags().DeleteTagsForAuthor(author_id);
        uo_work->Books().DeleteBooks(author_id);
        uo_work->Authors().Delete(author_id);
        uo_work->Commit();
    }

    std::optional<info::AuthorInfo> UseCasesImpl::GetAuthorByName(const std::string& author_name) const {
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Authors().GetAuthorByName(author_name);
    }

    info::Books UseCasesImpl::GetBooksByTitle(const std::string& book_title) const{
        auto uo_work = uow_factory_->CreateUnitOfWork(TypeOfTransaction::Read);
        return uo_work->Books().GetBooksByTitle(book_title);
    }

}  // namespace app
