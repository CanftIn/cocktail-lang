#include "Cocktail/Lexer/TokenKind.h"
#include "Cocktail/Common/Check.h"

namespace Cocktail {

auto TokenKind::Name() const -> llvm::StringRef {
  static constexpr llvm::StringLiteral Names[] = {
#define COCKTAIL_TOKEN(TokenName) #TokenName,
#include "Cocktail/Lexer/TokenRegistry.def"
  };
  return Names[static_cast<int>(kind_value_)];
}

auto TokenKind::IsKeyword() const -> bool {
  static constexpr bool Table[] = {
#define COCKTAIL_TOKEN(TokenName) false,
#define COCKTAIL_KEYWORD_TOKEN(TokenName, Spelling) true,
#include "Cocktail/Lexer/TokenRegistry.def"
  };
  return Table[static_cast<int>(kind_value_)];
}

auto TokenKind::IsSymbol() const -> bool {
  static constexpr bool Table[] = {
#define COCKTAIL_TOKEN(TokenName) false,
#define COCKTAIL_SYMBOL_TOKEN(TokenName, Spelling) true,
#include "Cocktail/Lexer/TokenRegistry.def"
  };
  return Table[static_cast<int>(kind_value_)];
}

auto TokenKind::IsGroupingSymbol() const -> bool {
  static constexpr bool Table[] = {
#define COCKTAIL_TOKEN(TokenName) false,
#define COCKTAIL_OPENING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, ClosingName) \
  true,
#define COCKTAIL_CLOSING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, OpeningName) \
  true,
#include "Cocktail/Lexer/TokenRegistry.def"
  };
  return Table[static_cast<int>(kind_value_)];
}

auto TokenKind::IsOpeningSymbol() const -> bool {
  static constexpr bool Table[] = {
#define COCKTAIL_TOKEN(TokenName) false,
#define COCKTAIL_OPENING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, ClosingName) \
  true,
#include "Cocktail/Lexer/TokenRegistry.def"
  };
  return Table[static_cast<int>(kind_value_)];
}

auto TokenKind::IsClosingSymbol() const -> bool {
  static constexpr bool Table[] = {
#define COCKTAIL_TOKEN(TokenName) false,
#define COCKTAIL_CLOSING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, OpeningName) \
  true,
#include "Cocktail/Lexer/TokenRegistry.def"
  };
  return Table[static_cast<int>(kind_value_)];
}

auto TokenKind::IsSizedTypeLiteral() const -> bool {
  return *this == TokenKind::IntegerTypeLiteral() ||
         *this == TokenKind::UnsignedIntegerTypeLiteral() ||
         *this == TokenKind::FloatingPointTypeLiteral();
}

auto TokenKind::GetOpeningSymbol() const -> TokenKind {
  static constexpr TokenKind Table[] = {
#define COCKTAIL_TOKEN(TokenName) Error(),
#define COCKTAIL_CLOSING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, OpeningName) \
  OpeningName(),
#include "Cocktail/Lexer/TokenRegistry.def"
  };
  auto result = Table[static_cast<int>(kind_value_)];
  COCKTAIL_CHECK(result != Error()) << "Only closing symbols are valid!";
  return result;
}

auto TokenKind::GetClosingSymbol() const -> TokenKind {
  static constexpr TokenKind Table[] = {
#define COCKTAIL_TOKEN(TokenName) Error(),
#define COCKTAIL_OPENING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, ClosingName) \
  ClosingName(),
#include "Cocktail/Lexer/TokenRegistry.def"
  };
  auto result = Table[static_cast<int>(kind_value_)];
  COCKTAIL_CHECK(result != Error()) << "Only closing symbols are valid!";
  return result;
}

auto TokenKind::GetFixedSpelling() const -> llvm::StringRef {
  static constexpr llvm::StringLiteral Table[] = {
#define COCKTAIL_TOKEN(TokenName) "",
#define COCKTAIL_SYMBOL_TOKEN(TokenName, Spelling) Spelling,
#define COCKTAIL_KEYWORD_TOKEN(TokenName, Spelling) Spelling,
#include "Cocktail/Lexer/TokenRegistry.def"
  };
  return Table[static_cast<int>(kind_value_)];
}

}  // namespace Cocktail