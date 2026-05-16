#pragma once
#include <compare>

namespace util {

/**
 * Шаблонный класс "помечённое значение".
 * В нём хранится значение и тег, по которому можно различать значения одного типа.
 * Пример:
 *
 *  struct AddressTag{}; // пустой тип для метки адреса, например
 *  using Address = util::Tagged<std::string, AddressTag>;
 *
 *  struct NameTag{}; // пустой тип для метки имени, например
 *  using Name = util::Tagged<std::string, NameTag>;
 *
 *  struct Person {
 *      Name name;
 *      Address address;
 *  };
 *
 *  Name name{"Harry Potter"s};
 *  Address address{"4 Privet Drive, Little Whinging, Surrey, England"s};
 *
 * Person p1{name, address}; // OK
 * Person p2{address, name}; // ошибка, Address и Name - разные типы
 */
template <typename Value, typename Tag>
class Tagged {
public:
    using ValueType = Value;
    using TagType = Tag;

    explicit Tagged(Value&& v)
        : value_(std::move(v)) {
    }
    explicit Tagged(const Value& v)
        : value_(v) {
    }

    const Value& operator*() const {
        return value_;
    }

    Value& operator*() {
        return value_;
    }

    // Для C++20 можно использовать трёхстороннее сравнение Tagged-типов
    // по значению value_
    auto operator<=>(const Tagged<Value, Tag>&) const = default;

private:
    Value value_;
};

// Хэшер для Tagged-типов, чтобы Tagged-обёртки можно было использовать в unordered-контейнерах
template <typename TaggedValue>
struct TaggedHasher {
    size_t operator()(const TaggedValue& value) const {
        // возвращаем хэш от хранимого значения
        return std::hash<typename TaggedValue::ValueType>{}(*value);
    }
};

}  // namespace util