#ifndef COCKTAIL_LEXER_CHARACTER_SET_H
#define COCKTAIL_LEXER_CHARACTER_SET_H

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail {

// [a-zA-Z]
inline auto IsAlpha(char c) -> bool { return llvm::isAlpha(c); }

// [0-9]
inline auto IsDecimalDigit(char c) -> bool { return llvm::isDigit(c); }

// [a-zA-Z0-9]
inline auto IsAlnum(char c) -> bool { return llvm::isAlnum(c); }

// Note that lowercase 'a'..'f' are currently not considered hexadecimal digits
// in any context.
// [0-9A-F]
inline auto IsUpperHexDigit(char c) -> bool {
  return ('0' <= c && c <= '9') || ('A' <= c && c <= 'F');
}

// [a-z]
inline auto IsLower(char c) -> bool { return 'a' <= c && c <= 'z'; }

inline auto IsHorizontalWhitespace(char c) -> bool {
  return c == ' ' || c == '\t';
}

// not check '\r' of Windows
inline auto IsVerticalWhitespace(char c) -> bool { return c == '\n'; }

inline auto IsSpace(char c) -> bool {
  return IsHorizontalWhitespace(c) || IsVerticalWhitespace(c);
}

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_CHARACTER_SET_H