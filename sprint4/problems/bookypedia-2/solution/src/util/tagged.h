#pragma once
#include <compare>
#include <functional>

namespace util {

// Паттерн "Тип-тег" (Type Tag).
//
// Позволяет создавать различные типы для одного и того же базового типа.
// Это помогает избежать ошибок при использовании похожих типов
// (например, ID разных сущностей).
//
// Пример использования:
//
//  struct AddressTag{}; // Тип тега для адреса, хранящего строку
//  using Address = util::Tagged<std::string, AddressTag>;
//
//  struct NameTag{}; // Тип тега для имени, хранящего строку
//  using Name = util::Tagged<std::string, NameTag>;
//
//  struct Person {
//      Name name;
//      Address address;
//  };
//
//  Name name{"Harry Potter"s};
//  Address address{"4 Privet Drive, Little Whinging, Surrey, England"s};
//
//  Person p1{name, address}; // OK
//  Person p2{address, name}; // Ошибка, Address и Name - разные типы
template <typename Value, typename Tag>
class Tagged {
public:
    using ValueType = Value;
    using TagType = Tag;

    // Конструктор с перемещением значения
    explicit Tagged(Value&& v)
        : value_(std::move(v)) {
    }
    
    // Конструктор с копированием значения
    explicit Tagged(const Value& v)
        : value_(v) {
    }

    // Получение константной ссылки на значение
    const Value& operator*() const {
        return value_;
    }

    // Получение изменяемой ссылки на значение
    Value& operator*() {
        return value_;
    }

    // Для C++20: spaceship-оператор обеспечивает полное сравнение по значению value_
    auto operator<=>(const Tagged<Value, Tag>&) const = default;

private:
    Value value_;
};

// Хеш-функция для Tagged-типов.
//
// Позволяет использовать Tagged-типы в unordered-контейнерах.
//
// Пример:
//  std::unordered_set<Name, TaggedHasher<Name>> names;
template <typename TaggedValue>
struct TaggedHasher {
    // Вычисляет хеш для внутреннего значения
    size_t operator()(const TaggedValue& value) const {
        return std::hash<typename TaggedValue::ValueType>{}(*value);
    }
};

}  // namespace util
