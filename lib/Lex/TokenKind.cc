#include "Cocktail/Lex/TokenKind.h"

namespace Cocktail::Lex {

COCKTAIL_DEFINE_ENUM_CLASS_NAMES(TokenKind) = {
#define COCKTAIL_TOKEN(TokenName) COCKTAIL_ENUM_CLASS_NAME_STRING(TokenName)
#include "Cocktail/Lex/TokenKind.def"
};

constexpr bool TokenKind::IsSymbol[] = {
#define COCKTAIL_TOKEN(TokenName) false,
#define COCKTAIL_SYMBOL_TOKEN(TokenName, Spelling) true,
#include "Cocktail/Lex/TokenKind.def"
};

constexpr bool TokenKind::IsGroupingSymbol[] = {
#define COCKTAIL_TOKEN(TokenName) false,
#define COCKTAIL_OPENING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, ClosingName) \
  true,
#define COCKTAIL_CLOSING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, OpeningName) \
  true,
#include "Cocktail/Lex/TokenKind.def"
};

constexpr bool TokenKind::IsOpeningSymbol[] = {
#define COCKTAIL_TOKEN(TokenName) false,
#define COCKTAIL_OPENING_GROUP_SYMBOL_TOKEN(TokeName, Spelling, ClosingName) \
  true,
#include "Cocktail/Lex/TokenKind.def"
};

constexpr bool TokenKind::IsClosingSymbol[] = {
#define COCKTAIL_TOKEN(TokenName) false,
#define COCKTAIL_CLOSING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, OpeningName) \
  true,
#include "Cocktail/Lex/TokenKind.def"
};

constexpr TokenKind TokenKind::OpeningSymbol[] = {
#define COCKTAIL_TOKEN(TokenName) Error,
#define COCKTAIL_CLOSING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, OpeningName) \
  OpeningName,
#include "Cocktail/Lex/TokenKind.def"
};

constexpr TokenKind TokenKind::ClosingSymbol[] = {
#define COCKTAIL_TOKEN(TokenName) Error,
#define COCKTAIL_OPENING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, ClosingName) \
  ClosingName,
#include "Cocktail/Lex/TokenKind.def"
};

constexpr bool TokenKind::IsOneCharSymbol[] = {
#define COCKTAIL_TOKEN(TokenName) false,
#define COCKTAIL_ONE_CHAR_SYMBOL_TOKEN(TokenName, Spelling) true,
#include "Cocktail/Lex/TokenKind.def"
};

constexpr bool TokenKind::IsKeyword[] = {
#define COCKTAIL_TOKEN(TokeName) false,
#define COCKTAIL_KEYWORD_TOKEN(TokenName, Spelling) true,
#include "Cocktail/Lex/TokenKind.def"
};

constexpr llvm::StringLiteral TokenKind::FixedSpelling[] = {
#define COCKTAIL_TOKEN(TokenName) "",
#define COCKTAIL_SYMBOL_TOKEN(TokenName, Spelling) Spelling,
#define COCKTAIL_KEYWORD_TOKEN(TokenName, Spelling) Spelling,
#include "Cocktail/Lex/TokenKind.def"
};

constexpr int8_t TokenKind::ExpectedParseTreeSize[] = {
#define COCKTAIL_TOKEN(Name) 1,
#define COCKTAIL_TOKEN_WITH_VIRTUAL_NODE(Size) 2,
#include "Cocktail/Lex/TokenKind.def"
};

}  // namespace Cocktail::Lex