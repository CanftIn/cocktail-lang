#ifndef COCKTAIL_LEXER_TOKEN_KIND_H
#define COCKTAIL_LEXER_TOKEN_KIND_H

#include <cstdint>
#include <initializer_list>
#include <iterator>

#include "llvm/ADT/StringRef.h"

namespace Cocktail {

class TokenKind {
  enum class KindEnum : uint8_t {
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

  friend auto operator==(const TokenKind& lhs, const TokenKind& rhs) -> bool {
    return lhs.kind_value == rhs.kind_value;
  }

  friend auto operator!=(const TokenKind& lhs, const TokenKind& rhs) -> bool {
    return lhs.kind_value != rhs.kind_value;
  }

  [[nodiscard]] auto Name() const -> llvm::StringRef;

  [[nodiscard]] auto IsKeyword() const -> bool;

  [[nodiscard]] auto IsSymbol() const -> bool;

  [[nodiscard]] auto IsGroupingSymbol() const -> bool;

  [[nodiscard]] auto IsOpeningSymbol() const -> bool;

  [[nodiscard]] auto IsClosingSymbol() const -> bool;

  [[nodiscard]] auto GetOpeningSymbol() const -> TokenKind;

  [[nodiscard]] auto GetClosingSymbol() const -> TokenKind;

  [[nodiscard]] auto GetFixedSpelling() const -> llvm::StringRef;

  [[nodiscard]] auto IsOneOf(std::initializer_list<TokenKind> kinds) const
      -> bool {
    for (TokenKind kind : kinds) {
      if (*this == kind) {
        return true;
      }
    }
    return false;
  }

  constexpr operator KindEnum() const { return kind_value; }

 private:
  constexpr explicit TokenKind(KindEnum kind_value) : kind_value(kind_value) {}

  KindEnum kind_value;
};

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_TOKEN_KIND_H