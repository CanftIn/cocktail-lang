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

class TokenizedBuffer;

namespace Internal {

class TokenizedBufferToken {
 public:
  using Token = TokenizedBufferToken;

  TokenizedBufferToken() = default;

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
  friend TokenizedBuffer;

  explicit TokenizedBufferToken(int index) : index(index) {}

  int32_t index;
};

}  // namespace Internal

class TokenizedBuffer {
 public:
  using Token = Internal::TokenizedBufferToken;

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

  // Random-access iterator
  class TokenIterator
      : public llvm::iterator_facade_base<
            TokenIterator, std::random_access_iterator_tag, const Token, int> {
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

    using iterator_facade_base::operator-;
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
    [[nodiscard]] auto Mantissa() const -> const llvm::APInt& {
      return buffer->literal_int_storage[literal_index];
    }

    [[nodiscard]] auto Exponent() const -> const llvm::APInt& {
      return buffer->literal_int_storage[literal_index + 1];
    }

    [[nodiscard]] auto IsDecimal() const -> bool { return is_decimal; }

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

  class TokenLocationTranslator
      : public DiagnosticLocationTranslator<Internal::TokenizedBufferToken> {
   public:
    explicit TokenLocationTranslator(TokenizedBuffer& buffer)
        : buffer(&buffer) {}

    auto GetLocation(Token token) -> Diagnostic::Location override;

   private:
    TokenizedBuffer* buffer;
  };

  static auto Lex(SourceBuffer& source, DiagnosticConsumer& consumer)
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

  [[nodiscard]] auto GetIntegerLiteral(Token token) const -> const llvm::APInt&;

  [[nodiscard]] auto GetRealLiteral(Token token) const -> RealLiteralValue;

  [[nodiscard]] auto GetStringLiteral(Token token) const -> llvm::StringRef;

  [[nodiscard]] auto GetMatchedClosingToken(Token opening_token) const -> Token;

  [[nodiscard]] auto GetMatchedOpeningToken(Token closing_token) const -> Token;

  // Returns whether the given token has leading whitespace.
  [[nodiscard]] auto HasLeadingWhitespace(Token token) const -> bool;
  // Returns whether the given token has trailing whitespace.
  [[nodiscard]] auto HasTrailingWhitespace(Token token) const -> bool;

  [[nodiscard]] auto IsRecoveryToken(Token token) const -> bool;

  // Returns the 1-based indentation column number.
  [[nodiscard]] auto GetIndentColumnNumber(Line line) const -> int;

  // Returns the text for an identifier.
  [[nodiscard]] auto GetIdentifierText(Identifier id) const -> llvm::StringRef;

  auto Print(llvm::raw_ostream& output_stream) const -> void;

  auto PrintToken(llvm::raw_ostream& output_stream, Token token) const -> void;

 private:
  class Lexer;
  friend Lexer;

  class SourceBufferLocationTranslator
      : public DiagnosticLocationTranslator<const char*> {
   public:
    explicit SourceBufferLocationTranslator(TokenizedBuffer& buffer)
        : buffer(&buffer) {}

    auto GetLocation(const char* pos) -> Diagnostic::Location override;

   private:
    TokenizedBuffer* buffer;
  };

  struct PrintWidths {
    auto Widen(const PrintWidths& new_width) -> void;

    int index;
    int kind;
    int column;
    int line;
    int indent;
  };

  struct TokenInfo {
    TokenKind kind;

    bool has_trailing_space = false;

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

  llvm::SmallVector<std::string, 16> literal_string_storage;

  llvm::DenseMap<llvm::StringRef, Identifier> identifier_map;

  bool has_errors = false;
};

using LexerDiagnosticEmitter = DiagnosticEmitter<const char*>;

using TokenDiagnosticEmitter = DiagnosticEmitter<TokenizedBuffer::Token>;

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_TOKENIZED_BUFFER_H
