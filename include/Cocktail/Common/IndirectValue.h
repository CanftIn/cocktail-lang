#ifndef COCKTAIL_COMMON_INDIRECT_VALUE_H
#define COCKTAIL_COMMON_INDIRECT_VALUE_H

#include <memory>
#include <type_traits>
#include <utility>

namespace Cocktail {

template <typename T>
class IndirectValue;

template <typename Callable>
auto CreateIndirectValue(Callable callable)
    -> IndirectValue<std::decay_t<decltype(callable())>>;

template <typename T>
class IndirectValue {
 public:
  IndirectValue() : value(std::make_unique<T>()) {}

  IndirectValue(T value) : value(std::make_unique<T>(std::move(value))) {}

  IndirectValue(const IndirectValue& other)
      : value(std::make_unique<T>(*other)) {}

  IndirectValue(IndirectValue&& other)
      : value(std::make_unique<T>(std::move(*other))) {}

  auto operator=(const IndirectValue& other) -> IndirectValue& {
    *value = *other.value;
    return *this;
  }

  auto operator=(IndirectValue&& other) -> IndirectValue& {
    *value = std::move(*other.value);
    return *this;
  }

  auto operator*() -> T& { return *value; }
  auto operator*() const -> const T& { return *value; }

  auto operator->() -> T* { return value.get(); }
  auto operator->() const -> const T* { return value.get(); }

  auto GetPointer() -> T* { return value.get(); }
  auto GetPointer() const -> const T* { return value.get(); }

 private:
  static_assert(std::is_object_v<T>, "T must be an object type");

  template <typename Callable>
  friend auto CreateIndirectValue(Callable callable)
      -> IndirectValue<std::decay_t<decltype(callable())>>;

  template <typename... Args>
  IndirectValue(std::unique_ptr<T> value) : value(std::move(value)) {}

  const std::unique_ptr<T> value;
};

template <typename Callable>
auto CreateIndirectValue(Callable callable)
    -> IndirectValue<std::decay_t<decltype(callable())>> {
  using T = std::decay_t<decltype(callable())>;
  return IndirectValue<T>(std::unique_ptr<T>(new T(callable())));
}

}  // namespace Cocktail

#endif  // COCKTAIL_COMMON_INDIRECT_VALUE_H