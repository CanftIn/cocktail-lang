#include "Cocktail/LexNew/LexHelpers.h"

namespace Cocktail::Lex {

auto CanLexInteger(DiagnosticEmitter<const char*>& emitter,
                   llvm::StringRef text) -> bool {
  // 1. 大整数（具有 10k+ 位数）的解析性能问题导致了工具链模糊测试器超时。
  // 2. 问题主要出现在 llvm::getAsInteger 函数中，该函数在处理大整数时性能下降。
  // 3. 有几种可能的解决方案，包括限制整数大小、优化解析性能和延迟解析。
  // 4. 目前，已经添加了一个限制来解决这个问题，但仍有优化的空间。

  // 用于限制数字的最大长度。
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

}  // namespace Cocktail::Lex