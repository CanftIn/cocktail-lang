#ifndef COCKTAIL_LEXER_NUMERIC_LITERAL_H
#define COCKTAIL_LEXER_NUMERIC_LITERAL_H

#include <utility>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail {

class NumericLiteralToken {
 public:
  auto Text() const -> llvm::StringRef { return text; }

  static auto Lex(llvm::StringRef source_text)
      -> llvm::Optional<NumericLiteralToken>;

  class Parser;

 private:
  NumericLiteralToken() {}

  llvm::StringRef text;

  // The offset of '.'
  int radix_point;

  // The offset of the alphabetical character introducing the exponent.
  int exponent;
};

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_NUMERIC_LITERAL_H