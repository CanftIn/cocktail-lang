#ifndef COCKTAIL_LEXER_TOKENKIND_H
#define COCKTAIL_LEXER_TOKENKIND_H

#include <cstdint>
#include <iterator>

#include "llvm/ADT/StringRef.h"

namespace Cocktail {

class TokenKind {
  enum class KindEnum : int8_t {
#define COCKTAIL_TOKEN(TokenName) TokenName,
#include "Cocktail/Lexer/TokenRegistry.def"
  };

 public:
#define COCKTAIL_TOKEN(TokenName)                  \
  static constexpr auto TokenName() -> TokenKind { \
    return TokenKind(KindEnum::TokenName);         \
  }
#include "Cocktail/Lexer/TokenRegistry.def"

  TokenKind() = delete;

  auto operator==(const TokenKind& rhs) const -> bool {
    return kind_value == rhs.kind_value;
  }

  auto operator!=(const TokenKind& rhs) const -> bool {
    return kind_value != rhs.kind_value;
  }

  auto Name() const -> llvm::StringRef;

  auto IsSymbol() const -> bool;

  auto IsGroupingSymbol() const -> bool;

  auto GetOpeningSymbol() const -> TokenKind;

  auto IsOpeningSymbol() const -> bool;

  auto GetClosingSymbol() const -> TokenKind;

  auto IsClosingSymbol() const -> bool;

  auto IsKeyword() const -> bool;

  auto GetFixedSpelling() const -> llvm::StringRef;

  constexpr explicit operator int() const {
    return static_cast<int>(kind_value);
  }

 private:
  constexpr explicit TokenKind(KindEnum kind_value) : kind_value(kind_value) {}

  KindEnum kind_value;
};

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_TOKENKIND_H