#ifndef COCKTAIL_COMMON_INDIRECT_VALUE_H
#define COCKTAIL_COMMON_INDIRECT_VALUE_H

#include <memory>
#include <type_traits>
#include <utility>

namespace Cocktail {

/// IndirectValue 主要用途之一是定义递归类型，防止无限嵌套。
template <typename T>
class IndirectValue {
 public:
  IndirectValue() : value_(std::make_unique<T>()) {}

  IndirectValue(T value) : value_(std::make_unique<T>(std::move(value))) {}

  IndirectValue(const IndirectValue& other)
      : value_(std::make_unique<T>(*other)) {}

  IndirectValue(IndirectValue&& other)
      : value_(std::make_unique<T>(std::move(*other))) {}

  auto operator=(const IndirectValue& other) -> IndirectValue& {
    *value_ = *other.value_;
    return *this;
  }

  auto operator=(IndirectValue&& other) -> IndirectValue& {
    *value_ = std::move(*other.value_);
    return *this;
  }

  auto operator*() -> T& { return *value_; }
  auto operator*() const -> const T& { return *value_; }

  auto operator->() -> T* { return value_.get(); }
  auto operator->() const -> const T* { return value_.get(); }

  auto GetPointer() -> T* { return value_.get(); }
  auto GetPointer() const -> const T* { return value_.get(); }

 private:
  static_assert(std::is_object_v<T>, "T must be an object type");

  template <typename Callable>
  friend auto CreateIndirectValue(Callable callable)
      -> IndirectValue<std::decay_t<decltype(callable())>>;

  template <typename... Args>
  IndirectValue(std::unique_ptr<T> value) : value_(std::move(value)) {}

  const std::unique_ptr<T> value_;
};

template <typename Callable>
auto CreateIndirectValue(Callable callable)
    -> IndirectValue<std::decay_t<decltype(callable())>> {
  using T = std::decay_t<decltype(callable())>;
  return IndirectValue<T>(std::unique_ptr<T>(new T(callable())));
}

}  // namespace Cocktail

#endif  // COCKTAIL_COMMON_INDIRECT_VALUE_H