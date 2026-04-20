#pragma once

#include <compare>
#include <cstddef>
#include <functional>
#include <type_traits>

namespace util {

/**
 * @brief Типобезопасная обёртка над значением
 * 
 * Позволяет создать уникальный тип на основе существующего,
 * предотвращая случайную подмену значений разных типов.
 * 
 * @tparam Value Базовый тип значения
 * @tparam Tag Уникальная метка типа (пустая структура)
 * 
 * @code
 * struct AddressTag {};
 * struct NameTag {};
 * 
 * using Address = Tagged<std::string, AddressTag>;
 * using Name = Tagged<std::string, NameTag>;
 * 
 * Name name{"Harry"};
 * Address addr{"Privet Drive"};
 * 
 * Person p1{name, addr};      // OK
 * Person p2{addr, name};      // Ошибка компиляции!
 * @endcode
 */
template <typename Value, typename Tag>
class Tagged {
 public:
  using ValueType = Value;
  using TagType = Tag;

  Tagged() = default;

  explicit Tagged(const Value& v) : value_(v) {}

  explicit Tagged(Value&& v) : value_(std::move(v)) {}

  [[nodiscard]] constexpr const Value& GetValue() const { return value_; }

  [[nodiscard]] constexpr Value& GetValue() { return value_; }

  [[nodiscard]] constexpr const Value& operator*() const { return value_; }

  [[nodiscard]] constexpr Value& operator*() { return value_; }

  explicit operator const Value&() const { return value_; }

  explicit operator Value&() { return value_; }

  [[nodiscard]] auto operator<=>(const Tagged&) const = default;

 private:
  Value value_;
};

/**
 * @brief Хеш-функция для Tagged-типов
 * 
 * Позволяет использовать Tagged в unordered-контейнерах
 * 
 * @tparam TaggedValue Тип Tagged<Value, Tag>
 */
template <typename TaggedValue>
struct TaggedHasher {
  [[nodiscard]] size_t operator()(const TaggedValue& value) const {
    using ValueType = typename TaggedValue::ValueType;
    return std::hash<ValueType>{}(*value);
  }
};

}  // namespace util