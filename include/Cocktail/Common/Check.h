#ifndef COCKTAIL_COMMON_CHECK_H
#define COCKTAIL_COMMON_CHECK_H

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

// Wraps a stream and exiting for fatal errors.
class ExitingStream {
 public:
  ~ExitingStream() {
    // Finish with a newline.
    llvm::errs() << "\n";
    if (treat_as_bug) {
      std::abort();
    } else {
      std::exit(-1);
    }
  }

  // Indicates that initial input is in, so this is where a ": " should be added
  // before user input.
  auto AddSeparator() -> ExitingStream& {
    separator = true;
    return *this;
  }

  // Indicates that the program is exiting due to a bug in the program, rather
  // than, e.g., invalid input.
  auto TreatAsBug() -> ExitingStream& {
    treat_as_bug = true;
    return *this;
  }

  // If the bool cast occurs, it's because the condition is false. This supports
  // && short-circuiting the creation of ExitingStream.
  explicit operator bool() const { return true; }

  // Forward output to llvm::errs.
  template <typename T>
  auto operator<<(const T& message) -> ExitingStream& {
    if (separator) {
      llvm::errs() << ": ";
      separator = false;
    }
    llvm::errs() << message;
    return *this;
  }

 private:
  // Whether a separator should be printed if << is used again.
  bool separator = false;

  // Whether the program is exiting due to a bug.
  bool treat_as_bug = false;
};

// For example:
//   CHECK(is_valid) << "Data is not valid!";
#define CHECK(condition)                                                      \
  (!(condition)) && (Cocktail::ExitingStream() << "CHECK failure: " #condition) \
                        .AddSeparator()                                       \
                        .TreatAsBug()

// For example:
//   FATAL() << "Unreachable!";
#define FATAL() Cocktail::ExitingStream().TreatAsBug() << "FATAL: "

}  // namespace Cocktail

#endif  // COCKTAIL_COMMON_CHECK_H