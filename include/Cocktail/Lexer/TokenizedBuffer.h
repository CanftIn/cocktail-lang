#ifndef COCKTAIL_LEXER_TOKENIZED_BUFFER_H
#define COCKTAIL_LEXER_TOKENIZED_BUFFER_H

#include <cstdint>
#include <iterator>

#include "Cocktail/Lexer/TokenKind.h"
#include "Cocktail/SourceBuffer.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator.h"
#include "llvm/ADT/iterator_range.h"

namespace Cocktail {

class TokenizedBuffer {
 public:
  class Token {
   public:
    Token() = default;
    // clang-format off
    auto operator==(const Token& rhs) const -> bool { return index == rhs.index; }
    auto operator!=(const Token& rhs) const -> bool { return index != rhs.index; }
    auto operator<(const Token& rhs) const -> bool { return index < rhs.index; }
    auto operator<=(const Token& rhs) const -> bool { return index <= rhs.index; }
    auto operator>(const Token& rhs) const -> bool { return index > rhs.index; }
    auto operator>=(const Token& rhs) const -> bool { return index >= rhs.index; }
    // clang-format on
   private:
    friend class TokenizedBuffer;

    explicit Token(int index) : index(index) {}

    int32_t index;
  };

  class Line {
   public:
    Line() = default;
    // clang-format off
    auto operator==(const Line& rhs) const -> bool { return index == rhs.index; }
    auto operator!=(const Line& rhs) const -> bool { return index != rhs.index; }
    auto operator<(const Line& rhs) const -> bool { return index < rhs.index; }
    auto operator<=(const Line& rhs) const -> bool { return index <= rhs.index; }
    auto operator>(const Line& rhs) const -> bool { return index > rhs.index; }
    auto operator>=(const Line& rhs) const -> bool { return index >= rhs.index; }
    // clang-format on
   private:
    friend class TokenizedBuffer;

    explicit Line(int index) : index(index) {}

    int32_t index;
  };

  class Identifier {
   public:
    Identifier() = default;
    // clang-format off
    auto operator==(const Identifier& rhs) const -> bool { return index == rhs.index; }
    auto operator!=(const Identifier& rhs) const -> bool { return index != rhs.index; }
    // clang-format on
   private:
    friend class TokenizedBuffer;

    explicit Identifier(int index) : index(index) {}

    int32_t index;
  };

  class TokenIterator
      : public llvm::iterator_facade_base<
            TokenIterator, std::random_access_iterator_tag, Token, int> {
   public:
    TokenIterator() = default;

    explicit TokenIterator(Token token) : token(token) {}

    auto operator==(const TokenIterator& rhs) const -> bool {
      return token == rhs.token;
    }

    auto operator<(const TokenIterator& rhs) const -> bool {
      return token < rhs.token;
    }

    auto operator*() const -> const Token& { return token; }
    auto operator*() -> Token& { return token; }

    auto operator-(const TokenIterator& rhs) const -> int {
      return token.index - rhs.token.index;
    }

    auto operator+=(int n) -> TokenIterator& {
      token.index += n;
      return *this;
    }

    auto operator-=(int n) -> TokenIterator& {
      token.index -= n;
      return *this;
    }

   private:
    friend class TokenizedBuffer;

    Token token;
  };

  static auto Lex(SourceBuffer& source) -> TokenizedBuffer;

  auto Tokens() const -> llvm::iterator_range<TokenIterator> {
    return llvm::make_range(TokenIterator(Token(0)),
                            TokenIterator(Token(token_infos.size())));
  }

  auto HasErrors() const -> bool { return has_errors; }

  auto Size() const -> int { return token_infos.size(); }

  auto GetKind(Token token) const -> TokenKind;
  auto GetLine(Token token) const -> Line;

  auto GetLineNumber(Token token) const -> int;

  // Returns the 1-based line number.
  auto GetLineNumber(Line line) const -> int;

  auto GetColumnNumber(Token token) const -> int;

  auto GetTokenText(Token token) const -> llvm::StringRef;

  auto GetIdentifier(Token token) const -> Identifier;

  // Returns the value of an `IntegerLiteral()` token.
  auto GetIntegerLiteral(Token token) const -> llvm::APInt;

  auto GetMatchedClosingToken(Token opening_token) const -> Token;

  auto GetMatchedOpeningToken(Token closing_token) const -> Token;

  auto IsRecoveryToken(Token token) const -> bool;

  // Returns the 1-based indentation column number.
  auto GetIndentColumnNumber(Line line) const -> int;

  // Returns the text for an identifier.
  auto GetIdentifierText(Identifier id) const -> llvm::StringRef;

  // Prints a description of the tokenized stream to the provided `raw_ostream`.
  //
  // It prints one line of information for each token in the buffer, including
  // the kind of token, where it occurs within the source file, indentation for
  // the associated line, the spelling of the token in source, and any
  // additional information tracked such as which unique identifier it is or any
  // matched grouping token.
  //
  // Each line is formatted as a YAML record:
  //
  // clang-format off
  // ```
  // token: { index: 0, kind: 'Semi', line: 1, column: 1, indent: 1, spelling: ';' }
  // ```
  // clang-format on
  //
  // This can be parsed as YAML using tools like `python-yq` combined with `jq`
  // on the command line. The format is also reasonably amenable to other
  // line-oriented shell tools from `grep` to `awk`.
  auto Print(llvm::raw_ostream& output_stream) const -> void;

  // Prints a description of a single token.  See `print` for details on the
  // format.
  auto PrintToken(llvm::raw_ostream& output_stream, Token token) const -> void;

 private:
  class Lexer;
  friend Lexer;

  struct PrintWidths {
    int index;
    int kind;
    int column;
    int line;
    int indent;

    void Widen(const PrintWidths& new_width);
  };

  struct TokenInfo {
    TokenKind kind;

    bool is_recovery = false;

    Line token_line;

    int32_t column;

    union {
      static_assert(
          sizeof(Token) <= sizeof(int32_t),
          "Unable to pack token and identifier index into the same space!");

      Identifier id;
      int32_t literal_index;
      Token closing_token;
      Token opening_token;
      int32_t error_length;
    };
  };

  struct LineInfo {
    int64_t start;

    int32_t length;

    int32_t indent;
  };

  struct IdentifierInfo {
    llvm::StringRef text;
  };

  explicit TokenizedBuffer(SourceBuffer& source) : source(&source) {}

  auto GetLineInfo(Line line) -> LineInfo&;
  auto GetLineInfo(Line line) const -> const LineInfo&;
  auto AddLine(LineInfo info) -> Line;
  auto GetTokenInfo(Token token) -> TokenInfo&;
  auto GetTokenInfo(Token token) const -> const TokenInfo&;
  auto AddToken(TokenInfo info) -> Token;
  auto GetTokenPrintWidths(Token token) const -> PrintWidths;
  auto PrintToken(llvm::raw_ostream& output_stream, Token token,
                  PrintWidths widths) const -> void;

  SourceBuffer* source;

  llvm::SmallVector<TokenInfo, 16> token_infos;

  llvm::SmallVector<LineInfo, 16> line_infos;

  llvm::SmallVector<IdentifierInfo, 16> identifier_infos;

  llvm::SmallVector<llvm::APInt, 16> int_literals;

  llvm::DenseMap<llvm::StringRef, Identifier> identifier_map;

  bool has_errors = false;
};

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_TOKENIZED_BUFFER_H
