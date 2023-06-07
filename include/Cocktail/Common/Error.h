#ifndef COCKTAIL_COMMON_ERROR_H
#define COCKTAIL_COMMON_ERROR_H

#include <string>
#include <variant>

#include "Cocktail/Common/Check.h"
#include "Cocktail/Common/Ostream.h"
#include "llvm/ADT/Twine.h"

namespace Cocktail {

struct Success {};

class [[nodiscard]] Error {
 public:
  explicit Error(llvm::Twine location, llvm::Twine message)
      : location_(location.str()), message_(message.str()) {
    COCKTAIL_CHECK(!message_.empty()) << "Errors must have a message.";
  }

  explicit Error(llvm::Twine message) : Error("", message) {}

  Error(Error&& other) noexcept
      : location_(std::move(other.location_)),
        message_(std::move(other.message_)) {}

  auto operator=(Error&& other) noexcept -> Error& {
    location_ = std::move(other.location_);
    message_ = std::move(other.message_);
    return *this;
  }

  auto Print(llvm::raw_ostream& out) const -> void {
    if (!location().empty()) {
      out << location() << ": ";
    }
    out << message();
  }

  auto location() const -> const std::string& { return location_; }

  auto message() const -> const std::string& { return message_; }

 private:
  std::string location_;
  std::string message_;
};

template <typename T>
class [[nodiscard]] ErrorOr {
 public:
  ErrorOr(Error err) : value_(std::move(err)) {}

  ErrorOr(T value) : value_(std::move(value)) {}

  auto ok() const -> bool { return std::holds_alternative<T>(value_); }

  auto error() const& -> const Error& {
    COCKTAIL_CHECK(!ok());
    return std::get<Error>(value_);
  }

  auto error() && -> Error {
    COCKTAIL_CHECK(!ok());
    return std::get<Error>(std::move(value_));
  }

  auto operator*() -> T& {
    COCKTAIL_CHECK(ok());
    return std::get<T>(value_);
  }

  auto operator*() const -> const T& {
    COCKTAIL_CHECK(ok());
    return std::get<T>(value_);
  }

  auto operator->() -> T* {
    COCKTAIL_CHECK(ok());
    return &std::get<T>(value_);
  }

  auto operator->() const -> const T* {
    COCKTAIL_CHECK(ok());
    return &std::get<T>(value_);
  }

 private:
  std::variant<Error, T> value_;
};

class ErrorBuilder {
 public:
  explicit ErrorBuilder(std::string location = "")
      : location_(std::move(location)),
        out_(std::make_unique<llvm::raw_string_ostream>(message_)) {}

  template <typename T>
  [[nodiscard]] auto operator<<(const T& message) && -> ErrorBuilder&& {
    *out_ << message;
    return std::move(*this);
  }

  template <typename T>
  auto operator<<(const T& message) & -> ErrorBuilder& {
    *out_ << message;
    return *this;
  }

  operator Error() { return Error(location_, message_); }

  template <typename T>
  operator ErrorOr<T>() {
    return Error(location_, message_);
  }

 private:
  std::string location_;
  std::string message_;
  std::unique_ptr<llvm::raw_string_ostream> out_;
};

}  // namespace Cocktail

// Macro hackery to get a unique variable name.
#define COCKTAIL_MAKE_UNIQUE_NAME_IMPL(a, b, c) a##b##c
#define COCKTAIL_MAKE_UNIQUE_NAME(a, b, c) \
  COCKTAIL_MAKE_UNIQUE_NAME_IMPL(a, b, c)

// Macro to prevent a top-level comma from being interpreted as a macro
// argument separator.
#define COCKTAIL_PROTECT_COMMAS(...) __VA_ARGS__

#define COCKTAIL_RETURN_IF_ERROR_IMPL(unique_name, expr)                  \
  if (auto unique_name = (expr); /* NOLINT(bugprone-macro-parentheses) */ \
      !(unique_name).ok()) {                                              \
    return std::move(unique_name).error();                                \
  }

#define COCKTAIL_RETURN_IF_ERROR(expr)                                    \
  COCKTAIL_RETURN_IF_ERROR_IMPL(                                          \
      COCKTAIL_MAKE_UNIQUE_NAME(_llvm_error_line, __LINE__, __COUNTER__), \
      COCKTAIL_PROTECT_COMMAS(expr))

#define COCKTAIL_ASSIGN_OR_RETURN_IMPL(unique_name, var, expr)        \
  auto unique_name = (expr); /* NOLINT(bugprone-macro-parentheses) */ \
  if (!(unique_name).ok()) {                                          \
    return std::move(unique_name).error();                            \
  }                                                                   \
  var = std::move(*(unique_name)); /* NOLINT(bugprone-macro-parentheses) */

#define COCKTAIL_ASSIGN_OR_RETURN(var, expr)                                 \
  COCKTAIL_ASSIGN_OR_RETURN_IMPL(                                            \
      COCKTAIL_MAKE_UNIQUE_NAME(_llvm_expected_line, __LINE__, __COUNTER__), \
      COCKTAIL_PROTECT_COMMAS(var), COCKTAIL_PROTECT_COMMAS(expr))

#endif  // COCKTAIL_COMMON_ERROR_H