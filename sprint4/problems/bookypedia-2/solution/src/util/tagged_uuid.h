#pragma once

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <string>
#include <string_view>

#include "tagged.h"

namespace util {

namespace detail {

using UUIDType = boost::uuids::uuid;

// Генерирует новый случайный UUID
inline UUIDType NewUUID() {
    return boost::uuids::random_generator()();
}

// Нулевой UUID (все байты равны 0)
constexpr UUIDType ZeroUUID{{0}};

// Преобразует UUID в строку. Формат: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
inline std::string UUIDToString(const UUIDType& uuid) {
    return boost::uuids::to_string(uuid);
}

// Преобразует строку в UUID. Выбрасывает исключение, если строка имеет неверный формат
inline UUIDType UUIDFromString(std::string_view str) {
    boost::uuids::string_generator gen;
    return gen(str.begin(), str.end());
}

}  // namespace detail

// Специализация Tagged для UUID.
// Предоставляет удобные методы для работы с типобезопасными UUID.
//
// Пример использования:
//
//  struct AuthorTag {};
//  using AuthorId = TaggedUUID<AuthorTag>;
//
//  AuthorId id = AuthorId::New();
//  std::string text = id.ToString();
//  AuthorId from_text = AuthorId::FromString(text);
template <typename Tag>
class TaggedUUID : public Tagged<detail::UUIDType, Tag> {
public:
    using Base = Tagged<detail::UUIDType, Tag>;
    using Tagged<detail::UUIDType, Tag>::Tagged;

    // Конструктор по умолчанию (создаёт нулевой UUID)
    TaggedUUID()
        : Base{detail::ZeroUUID} {
    }

    // Создаёт новый UUID со случайным значением
    static TaggedUUID New() {
        return TaggedUUID{detail::NewUUID()};
    }

    // Создаёт UUID из строкового представления.
    // Выбрасывает исключение, если строка имеет неверный формат
    static TaggedUUID FromString(const std::string& uuid_as_text) {
        return TaggedUUID{detail::UUIDFromString(uuid_as_text)};
    }

    // Преобразует UUID в строку
    std::string ToString() const {
        return detail::UUIDToString(**this);
    }
};

}  // namespace util