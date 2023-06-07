#include "Cocktail/Common/CheckInternal.h"

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Signals.h"

namespace Cocktail::Internal {

auto PrintAfterStackTrace(void* str) -> void {
  llvm::errs() << reinterpret_cast<char*>(str);
}

ExitingStream::~ExitingStream() {
  llvm_unreachable(
      "Exiting streams should only be constructed by check.h macros that "
      "ensure the special operator| exits the program prior to their "
      "destruction!");
}

auto ExitingStream::Done() -> void {
  buffer << "\n";
  llvm::sys::AddSignalHandler(PrintAfterStackTrace,
                              const_cast<char*>(buffer_str.c_str()));
  std::abort();
}

}  // namespace Cocktail::Internal