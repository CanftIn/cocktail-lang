#ifndef COCKTAIL_LEXER_TOKENIZED_BUFFER_H
#define COCKTAIL_LEXER_TOKENIZED_BUFFER_H

#include <cstdint>
#include <iterator>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Cocktail/Lexer/TokenKind.h"
#include "Cocktail/Source/SourceBuffer.h"
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

    friend auto operator==(const Token& lhs, const Token& rhs) -> bool {
      return lhs.index == rhs.index;
    }
    friend auto operator!=(const Token& lhs, const Token& rhs) -> bool {
      return lhs.index != rhs.index;
    }
    friend auto operator<(const Token& lhs, const Token& rhs) -> bool {
      return lhs.index < rhs.index;
    }
    friend auto operator<=(const Token& lhs, const Token& rhs) -> bool {
      return lhs.index <= rhs.index;
    }
    friend auto operator>(const Token& lhs, const Token& rhs) -> bool {
      return lhs.index > rhs.index;
    }
    friend auto operator>=(const Token& lhs, const Token& rhs) -> bool {
      return lhs.index >= rhs.index;
    }

   private:
    friend class TokenizedBuffer;

    explicit Token(int index) : index(index) {}

    int32_t index;
  };

  class Line {
   public:
    Line() = default;

    friend auto operator==(const Line& lhs, const Line& rhs) -> bool {
      return lhs.index == rhs.index;
    }
    friend auto operator!=(const Line& lhs, const Line& rhs) -> bool {
      return lhs.index != rhs.index;
    }
    friend auto operator<(const Line& lhs, const Line& rhs) -> bool {
      return lhs.index < rhs.index;
    }
    friend auto operator<=(const Line& lhs, const Line& rhs) -> bool {
      return lhs.index <= rhs.index;
    }
    friend auto operator>(const Line& lhs, const Line& rhs) -> bool {
      return lhs.index > rhs.index;
    }
    friend auto operator>=(const Line& lhs, const Line& rhs) -> bool {
      return lhs.index >= rhs.index;
    }

   private:
    friend class TokenizedBuffer;

    explicit Line(int index) : index(index) {}

    int32_t index;
  };

  class Identifier {
   public:
    Identifier() = default;

    friend auto operator==(const Identifier& lhs, const Identifier& rhs)
        -> bool {
      return lhs.index == rhs.index;
    }
    friend auto operator!=(const Identifier& lhs, const Identifier& rhs)
        -> bool {
      return lhs.index != rhs.index;
    }

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

  class RealLiteralValue {
   public:
    auto Mantissa() const -> const llvm::APInt& {
      return buffer->literal_int_storage[literal_index];
    }

    auto Exponent() const -> const llvm::APInt& {
      return buffer->literal_int_storage[literal_index + 1];
    }

    auto IsDecimal() const -> bool { return is_decimal; }

   private:
    friend class TokenizedBuffer;

    RealLiteralValue(const TokenizedBuffer* buffer, int32_t literal_index,
                     bool is_decimal)
        : buffer(buffer),
          literal_index(literal_index),
          is_decimal(is_decimal) {}

    const TokenizedBuffer* buffer;
    int32_t literal_index;
    bool is_decimal;
  };

  static auto Lex(SourceBuffer& source, DiagnosticEmitter& emitter)
      -> TokenizedBuffer;

  [[nodiscard]] auto HasErrors() const -> bool { return has_errors; }

  [[nodiscard]] auto Tokens() const -> llvm::iterator_range<TokenIterator> {
    return llvm::make_range(TokenIterator(Token(0)),
                            TokenIterator(Token(token_infos.size())));
  }

  [[nodiscard]] auto Size() const -> int { return token_infos.size(); }

  [[nodiscard]] auto GetKind(Token token) const -> TokenKind;
  [[nodiscard]] auto GetLine(Token token) const -> Line;

  [[nodiscard]] auto GetLineNumber(Token token) const -> int;

  // Returns the 1-based line number.
  [[nodiscard]] auto GetLineNumber(Line line) const -> int;

  [[nodiscard]] auto GetColumnNumber(Token token) const -> int;

  [[nodiscard]] auto GetTokenText(Token token) const -> llvm::StringRef;

  [[nodiscard]] auto GetIdentifier(Token token) const -> Identifier;

  // Returns the value of an `IntegerLiteral()` token.
  [[nodiscard]] auto GetIntegerLiteral(Token token) const -> const llvm::APInt&;

  // Returns the value of a `RealLiteral()` token.
  [[nodiscard]] auto GetRealLiteral(Token token) const -> RealLiteralValue;

  [[nodiscard]] auto GetMatchedClosingToken(Token opening_token) const -> Token;

  [[nodiscard]] auto GetMatchedOpeningToken(Token closing_token) const -> Token;

  [[nodiscard]] auto IsRecoveryToken(Token token) const -> bool;

  // Returns the 1-based indentation column number.
  [[nodiscard]] auto GetIndentColumnNumber(Line line) const -> int;

  // Returns the text for an identifier.
  [[nodiscard]] auto GetIdentifierText(Identifier id) const -> llvm::StringRef;

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
  [[nodiscard]] auto GetLineInfo(Line line) const -> const LineInfo&;
  auto AddLine(LineInfo info) -> Line;
  auto GetTokenInfo(Token token) -> TokenInfo&;
  [[nodiscard]] auto GetTokenInfo(Token token) const -> const TokenInfo&;
  auto AddToken(TokenInfo info) -> Token;
  [[nodiscard]] auto GetTokenPrintWidths(Token token) const -> PrintWidths;
  auto PrintToken(llvm::raw_ostream& output_stream, Token token,
                  PrintWidths widths) const -> void;

  SourceBuffer* source;

  llvm::SmallVector<TokenInfo, 16> token_infos;

  llvm::SmallVector<LineInfo, 16> line_infos;

  llvm::SmallVector<IdentifierInfo, 16> identifier_infos;

  llvm::SmallVector<llvm::APInt, 16> literal_int_storage;

  llvm::DenseMap<llvm::StringRef, Identifier> identifier_map;

  bool has_errors = false;
};

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_TOKENIZED_BUFFER_H
