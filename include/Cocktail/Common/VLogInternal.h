#ifndef COCKTAIL_COMMON_VLOG_INTERNAL_H
#define COCKTAIL_COMMON_VLOG_INTERNAL_H

#include <cstdlib>

#include "llvm/Support/raw_ostream.h"

namespace Cocktail::Internal {

class VLoggingStream {
 public:
  struct Helper {};

  explicit VLoggingStream(llvm::raw_ostream* stream) : stream_(stream) {}

  ~VLoggingStream() = default;

  explicit operator bool() const { return true; }

  template <typename T>
  auto operator<<(const T& message) -> VLoggingStream& {
    *stream_ << message;
    return *this;
  }

  friend auto operator|(Helper /*unused*/, VLoggingStream& /*unused*/) -> void {
  }

 private:
  [[noreturn]] auto Done() -> void;

  llvm::raw_ostream* stream_;
};

}  // namespace Cocktail::Internal

#define COCKTAIL_VLOG_INTERNAL_STREAM(stream)    \
  Cocktail::Internal::VLoggingStream::Helper() | \
      Cocktail::Internal::VLoggingStream(stream)

#endif  // COCKTAIL_COMMON_VLOG_INTERNAL_H