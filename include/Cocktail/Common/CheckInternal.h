#ifndef COCKTAIL_COMMON_CHECK_INTERNAL_H
#define COCKTAIL_COMMON_CHECK_INTERNAL_H

#include "llvm/Support/raw_ostream.h"

namespace Cocktail::Internal {

// Wraps a stream and exiting for fatal errors.
class ExitingStream {
 public:
  struct AddSeparator {};

  struct Helper {};

  ExitingStream() : buffer(buffer_str) {}

  [[noreturn]] ~ExitingStream();

  explicit operator bool() const { return true; }

  template <typename T>
  auto operator<<(const T& message) -> ExitingStream& {
    if (separator) {
      buffer << ": ";
      separator = false;
    }
    buffer << message;
    return *this;
  }

  auto operator<<(AddSeparator /*unused*/) -> ExitingStream& {
    separator = true;
    return *this;
  }

  [[noreturn]] friend auto operator|(Helper /*unused*/, ExitingStream& stream)
      -> void {
    stream.Done();
  }

 private:
  [[noreturn]] auto Done() -> void;

  bool separator = false;

  std::string buffer_str;
  llvm::raw_string_ostream buffer;
};

}  // namespace Cocktail::Internal

#define COCKTAIL_CHECK_INTERNAL_STREAM()        \
  Cocktail::Internal::ExitingStream::Helper() | \
      Cocktail::Internal::ExitingStream()

#endif  // COCKTAIL_COMMON_CHECK_INTERNAL_H