#pragma once
#include <string>
#include <optional>

#include "../util/tagged_uuid.h"
#include "../ui/struct_info.h"

namespace domain {

namespace detail {

// Маркер для типобезопасного AuthorId
struct AuthorTag {};

}  // namespace detail

// Типобезопасный идентификатор автора
using AuthorId = util::TaggedUUID<detail::AuthorTag>;

// Сущность "Автор"
class Author {
public:
    Author(AuthorId id, std::string name)
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    // Получение идентификатора автора
    const AuthorId& GetId() const noexcept {
        return id_;
    }

    // Получение имени автора
    const std::string& GetName() const noexcept {
        return name_;
    }

private:
    AuthorId id_;
    std::string name_;
};

// Репозиторий для работы с авторами
class AuthorRepository {
public:
    // Сохранение автора (добавление или обновление)
    virtual void Save(const Author& author) = 0;
    // Удаление автора по ID
    virtual void Delete(const std::string& author_id) = 0;
    // Редактирование информации об авторе
    virtual void Edit(const info::AuthorInfo& author) = 0;
    // Получение списка всех авторов
    virtual info::Authors GetAuthors() const = 0;
    // Поиск автора по имени
    virtual std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const = 0;

protected:
    ~AuthorRepository() = default;
};

}  // namespace domain
