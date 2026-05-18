#pragma once
#include <string>

#include "book.h"

namespace domain {

// Теги книги
class Tag {
public:
    Tag(domain::BookId book_id, std::string tag)
        : book_id_(std::move(book_id))
        , tag_(std::move(tag)) {
    }

    // Получение идентификатора книги
    const domain::BookId& GetBookId() const noexcept {
        return book_id_;
    }

    // Получение текста тега
    const std::string& GetTag() const noexcept {
        return tag_;
    }

private:
    domain::BookId book_id_;
    std::string tag_;
};

// Репозиторий для работы с тегами книг
class TagRepository {
public:
    // Сохранение списка тегов для книги
    virtual void Save(const std::vector<Tag>& tags) = 0;
    // Сохранение тегов по ID книги (альтернативный метод)
    virtual void Save(const std::vector<std::string>& tags, const std::string& book_id) = 0;
    // Удаление всех тегов автора (при удалении автора)
    virtual void DeleteTagsForAuthor(const std::string& author_id) = 0;
    // Удаление всех тегов книги
    virtual void DeleteTagsForBook(const std::string& book_id) = 0;
    // Получение списка тегов книги
    virtual std::vector<std::string> GetTags(const std::string& book_id) const = 0;

protected:
    ~TagRepository() = default;
};

}  // namespace domain
