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
  enum class Radix : uint8_t { Binary = 2, Decimal = 10, Hexadecimal = 16 };

  struct IntegerValue {
    llvm::APInt value;
  };

  struct RealValue {
    Radix radix;
    llvm::APInt mantissa;
    llvm::APInt exponent;
  };

  struct UnrecoverableError {};

  using Value = std::variant<IntegerValue, RealValue, UnrecoverableError>;

  static auto Lex(llvm::StringRef source_text)
      -> llvm::Optional<LexedNumericLiteral>;

  auto ComputeValue(DiagnosticEmitter<const char*>& emitter) const -> Value;

  [[nodiscard]] auto text() const -> llvm::StringRef { return text_; }

 private:
  LexedNumericLiteral() = default;

  class Parser;

  llvm::StringRef text_;

  // The offset of '.'
  int radix_point_;

  // The offset of the alphabetical character introducing the exponent.
  int exponent_;
};

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_NUMERIC_LITERAL_H