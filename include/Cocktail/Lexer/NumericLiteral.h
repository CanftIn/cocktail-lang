#ifndef COCKTAIL_LEXER_NUMERIC_LITERAL_H
#define COCKTAIL_LEXER_NUMERIC_LITERAL_H

#include <utility>
#include <variant>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail {

class LexedNumericLiteral {
 public:
  [[nodiscard]] auto Text() const -> llvm::StringRef { return text; }

  static auto Lex(llvm::StringRef source_text)
      -> llvm::Optional<LexedNumericLiteral>;

  struct IntegerValue {
    llvm::APInt value;
  };

  struct RealValue {
    int radix;
    llvm::APInt mantissa;
    llvm::APInt exponent;
  };

  struct UnrecoverableError {};

  using Value = std::variant<IntegerValue, RealValue, UnrecoverableError>;

  auto ComputeValue(DiagnosticEmitter<const char*>& emitter) const -> Value;

 private:
  LexedNumericLiteral() = default;

  class Parser;

  llvm::StringRef text;

  // The offset of '.'
  int radix_point;

  // The offset of the alphabetical character introducing the exponent.
  int exponent;
};

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_NUMERIC_LITERAL_H