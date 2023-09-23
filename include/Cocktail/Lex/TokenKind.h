#ifndef COCKTAIL_LEX_TOKEN_KIND_H
#define COCKTAIL_LEX_TOKEN_KIND_H

#include <cstdint>

#include "Cocktail/Common/Check.h"
#include "Cocktail/Common/EnumBase.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadicDetails.h"

namespace Cocktail::Lex {

COCKTAIL_DEFINE_RAW_ENUM_CLASS(TokenKind, uint8_t) {
#define COCKTAIL_TOKEN(TokenName) COCKTAIL_RAW_ENUM_ENUMERATOR(TokenName)
#include "Cocktail/Lex/TokenKind.def"
};

class TokenKind : public COCKTAIL_ENUM_BASE(TokenKind) {
 public:
#define COCKTAIL_TOKEN(TokenName) COCKTAIL_ENUM_CONSTANT_DECLARATION(TokenName)
#include "Cocktail/Lex/TokenKind.def"

  /// 所有keyword token。
  static const llvm::ArrayRef<TokenKind> KeywordTokens;

  /// 检查此标记是否为简单的符号序列（如标点符号）。
  /// 这些符号可以直接出现在源代码中，并且可以使用starts_with进行词法分析。
  [[nodiscard]] auto is_symbol() const -> bool { return IsSymbol[AsInt()]; }

  /// 检查此标记是否为分组符号（如括号、大括号等），这些符号在标记流中必须匹配。
  [[nodiscard]] auto is_grouping_symbol() const -> bool {
    return IsGroupingSymbol[AsInt()];
  }

  /// 对于结束符号，返回其对应的开头符号。
  [[nodiscard]] auto opening_symbol() const -> TokenKind {
    auto result = OpeningSymbol[AsInt()];
    COCKTAIL_CHECK(result != Error) << "Only closing symbols are valid!";
    return result;
  }

  /// 检查此标记是否为分组的开头符号。
  [[nodiscard]] auto is_opening_symbol() const -> bool {
    return IsOpeningSymbol[AsInt()];
  }

  /// 对于开头符号，返回其对应的结束符号。
  [[nodiscard]] auto closing_symbol() const -> TokenKind {
    auto result = ClosingSymbol[AsInt()];
    COCKTAIL_CHECK(result != Error) << "Only opening symbol are valid!";
    return result;
  }

  /// 检查此标记是否为分组的结束符号。
  [[nodiscard]] auto is_closing_symbol() const -> bool {
    return IsClosingSymbol[AsInt()];
  }

  /// 检查此标记是否为单字符符号，且此字符不是其他符号的一部分。
  [[nodiscard]] auto is_one_char_symbol() const -> bool {
    return IsOneCharSymbol[AsInt()];
  };

  /// 检查此标记是否为关键字。
  [[nodiscard]] auto is_keyword() const -> bool { return IsKeyword[AsInt()]; };

  /// 检查此标记是否为带有大小的类型字面量（如整数、浮点数类型字面量）。
  [[nodiscard]] auto is_sized_type_literal() const -> bool {
    return *this == TokenKind::IntegerTypeLiteral ||
           *this == TokenKind::UnsignedIntegerTypeLiteral ||
           *this == TokenKind::FloatingPointTypeLiteral;
  };

  /// 如果此标记在源代码中有固定的拼写，则返回它。否则返回空字符串。
  [[nodiscard]] auto fixed_spelling() const -> llvm::StringRef {
    return FixedSpelling[AsInt()];
  };

  /// 获取此标记所对应的解析树节点的预期数量。
  [[nodiscard]] auto expected_parse_tree_size() const -> int {
    return ExpectedParseTreeSize[AsInt()];
  }

  /// 检查此标记是否在提供的列表中。
  [[nodiscard]] auto IsOneOf(std::initializer_list<TokenKind> kinds) const
      -> bool {
    for (TokenKind kind : kinds) {
      if (*this == kind) {
        return true;
      }
    }
    return false;
  }

 private:
  static const TokenKind KeywordTokensStorage[];

  static const bool IsSymbol[];
  static const bool IsGroupingSymbol[];
  static const bool IsOpeningSymbol[];
  static const bool IsClosingSymbol[];
  static const TokenKind OpeningSymbol[];
  static const TokenKind ClosingSymbol[];
  static const bool IsOneCharSymbol[];
  static const bool IsKeyword[];
  static const llvm::StringLiteral FixedSpelling[];
  static const int8_t ExpectedParseTreeSize[];
};

#define COCKTAIL_TOKEN(TokenName) \
  COCKTAIL_ENUM_CONSTANT_DEFINITION(TokenKind, TokenName)
#include "Cocktail/Lex/TokenKind.def"

constexpr TokenKind TokenKind::KeywordTokensStorage[] = {
#define COCKTAIL_KEYWORD_TOKEN(TokenName, Spelling) TokenKind::TokenName,
#include "Cocktail/Lex/TokenKind.def"
};

constexpr llvm::ArrayRef<TokenKind> TokenKind::KeywordTokens =
    KeywordTokensStorage;

}  // namespace Cocktail::Lex

namespace llvm {

template <>
struct format_provider<Cocktail::Lex::TokenKind> {
  static void format(const Cocktail::Lex::TokenKind& kind, raw_ostream& out,
                     StringRef /*style*/) {
    auto spelling = kind.fixed_spelling();
    if (!spelling.empty()) {
      out << spelling;
    } else {
      out << kind;
    }
  }
};

}  // namespace llvm

#endif  // COCKTAIL_LEX_TOKEN_KIND_H