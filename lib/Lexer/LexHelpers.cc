#include "Cocktail/Lexer/LexHelpers.h"

#include "llvm/Support/FormatVariadic.h"

namespace Cocktail {

auto CanLexInteger(DiagnosticEmitter<const char*>& emitter,
                   llvm::StringRef text) -> bool {
  constexpr size_t DigitLimit = 1000;
  if (text.size() > DigitLimit) {
    COCKTAIL_DIAGNOSTIC(
        TooManyDigits, Error,
        "Found a sequence of {0} digits, which is greater than the "
        "limit of {1}.",
        size_t, size_t);
    emitter.Emit(text.begin(), TooManyDigits, text.size(), DigitLimit);
    return false;
  }
  return true;
}

}  // namespace Cocktail