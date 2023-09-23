#ifndef COCKTAIL_LEXER_TOKENIZED_BUFFER_H
#define COCKTAIL_LEXER_TOKENIZED_BUFFER_H

#include <cstdint>
#include <iterator>

#include "Cocktail/Common/Ostream.h"
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
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

class TokenizedBuffer;

namespace Internal {

class TokenizedBufferToken {
 public:
  using Token = TokenizedBufferToken;

  TokenizedBufferToken() = default;

  friend auto operator==(const Token& lhs, const Token& rhs) -> bool {
    return lhs.index_ == rhs.index_;
  }
  friend auto operator!=(const Token& lhs, const Token& rhs) -> bool {
    return lhs.index_ != rhs.index_;
  }
  friend auto operator<(const Token& lhs, const Token& rhs) -> bool {
    return lhs.index_ < rhs.index_;
  }
  friend auto operator<=(const Token& lhs, const Token& rhs) -> bool {
    return lhs.index_ <= rhs.index_;
  }
  friend auto operator>(const Token& lhs, const Token& rhs) -> bool {
    return lhs.index_ > rhs.index_;
  }
  friend auto operator>=(const Token& lhs, const Token& rhs) -> bool {
    return lhs.index_ >= rhs.index_;
  }

 private:
  friend TokenizedBuffer;

  explicit TokenizedBufferToken(int index) : index_(index) {}

  int32_t index_;
};

}  // namespace Internal

class TokenizedBuffer {
 public:
  using Token = Internal::TokenizedBufferToken;

  class Line {
   public:
    Line() = default;

    friend auto operator==(const Line& lhs, const Line& rhs) -> bool {
      return lhs.index_ == rhs.index_;
    }
    friend auto operator!=(const Line& lhs, const Line& rhs) -> bool {
      return lhs.index_ != rhs.index_;
    }
    friend auto operator<(const Line& lhs, const Line& rhs) -> bool {
      return lhs.index_ < rhs.index_;
    }
    friend auto operator<=(const Line& lhs, const Line& rhs) -> bool {
      return lhs.index_ <= rhs.index_;
    }
    friend auto operator>(const Line& lhs, const Line& rhs) -> bool {
      return lhs.index_ > rhs.index_;
    }
    friend auto operator>=(const Line& lhs, const Line& rhs) -> bool {
      return lhs.index_ >= rhs.index_;
    }

   private:
    friend class TokenizedBuffer;

    explicit Line(int index) : index_(index) {}

    int32_t index_;
  };

  class Identifier {
   public:
    Identifier() = default;

    friend auto operator==(const Identifier& lhs, const Identifier& rhs)
        -> bool {
      return lhs.index_ == rhs.index_;
    }
    friend auto operator!=(const Identifier& lhs, const Identifier& rhs)
        -> bool {
      return lhs.index_ != rhs.index_;
    }

   private:
    friend class TokenizedBuffer;

    explicit Identifier(int index) : index_(index) {}

    int32_t index_;
  };

  // Random-access iterator
  class TokenIterator
      : public llvm::iterator_facade_base<
            TokenIterator, std::random_access_iterator_tag, const Token, int> {
   public:
    TokenIterator() = default;

    explicit TokenIterator(Token token) : token_(token) {}

    auto operator==(const TokenIterator& rhs) const -> bool {
      return token_ == rhs.token_;
    }

    auto operator<(const TokenIterator& rhs) const -> bool {
      return token_ < rhs.token_;
    }

    auto operator*() const -> const Token& { return token_; }

    using iterator_facade_base::operator-;
    auto operator-(const TokenIterator& rhs) const -> int {
      return token_.index_ - rhs.token_.index_;
    }

    auto operator+=(int n) -> TokenIterator& {
      token_.index_ += n;
      return *this;
    }

    auto operator-=(int n) -> TokenIterator& {
      token_.index_ -= n;
      return *this;
    }

    auto Print(llvm::raw_ostream& output) const -> void;

   private:
    friend class TokenizedBuffer;

    Token token_;
  };

  class RealLiteralValue {
   public:
    [[nodiscard]] auto Mantissa() const -> const llvm::APInt& {
      return buffer_->literal_int_storage_[literal_index_];
    }

    [[nodiscard]] auto Exponent() const -> const llvm::APInt& {
      return buffer_->literal_int_storage_[literal_index_ + 1];
    }

    [[nodiscard]] auto IsDecimal() const -> bool { return is_decimal_; }

    auto Print(llvm::raw_ostream& output_stream) const -> void {
      output_stream << Mantissa() << "*" << (is_decimal_ ? "10" : "2") << "^"
                    << Exponent();
    }

   private:
    friend class TokenizedBuffer;

    RealLiteralValue(const TokenizedBuffer* buffer, int32_t literal_index,
                     bool is_decimal)
        : buffer_(buffer),
          literal_index_(literal_index),
          is_decimal_(is_decimal) {}

    const TokenizedBuffer* buffer_;
    int32_t literal_index_;
    bool is_decimal_;
  };

  class TokenLocationTranslator
      : public DiagnosticLocationTranslator<Internal::TokenizedBufferToken> {
   public:
    explicit TokenLocationTranslator(TokenizedBuffer& buffer,
                                     int* last_line_lexed_to_column)
        : buffer_(&buffer),
          last_line_lexed_to_column_(last_line_lexed_to_column) {}

    auto GetLocation(Token token) -> DiagnosticLocation override;

   private:
    TokenizedBuffer* buffer_;
    int* last_line_lexed_to_column_;
  };

  static auto Lex(SourceBuffer& source, DiagnosticConsumer& consumer)
      -> TokenizedBuffer;

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

  [[nodiscard]] auto GetTypeLiteralSize(Token token) const
      -> const llvm::APInt&;

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

  [[nodiscard]] auto has_errors() const -> bool { return has_errors_; }

  [[nodiscard]] auto tokens() const -> llvm::iterator_range<TokenIterator> {
    return llvm::make_range(TokenIterator(Token(0)),
                            TokenIterator(Token(token_infos_.size())));
  }

  [[nodiscard]] auto size() const -> int { return token_infos_.size(); }

 private:
  class Lexer;
  friend Lexer;

  class SourceBufferLocationTranslator
      : public DiagnosticLocationTranslator<const char*> {
   public:
    explicit SourceBufferLocationTranslator(TokenizedBuffer& buffer,
                                            int* last_line_lexed_to_column)
        : buffer_(&buffer),
          last_line_lexed_to_column_(last_line_lexed_to_column) {}

    auto GetLocation(const char* loc) -> DiagnosticLocation override;

   private:
    TokenizedBuffer* buffer_;
    int* last_line_lexed_to_column_;
  };

  struct PrintWidths {
    auto Widen(const PrintWidths& widths) -> void;

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

  explicit TokenizedBuffer(SourceBuffer& source) : source_(&source) {}

  auto GetLineInfo(Line line) -> LineInfo&;
  [[nodiscard]] auto GetLineInfo(Line line) const -> const LineInfo&;
  auto AddLine(LineInfo info) -> Line;
  auto GetTokenInfo(Token token) -> TokenInfo&;
  [[nodiscard]] auto GetTokenInfo(Token token) const -> const TokenInfo&;
  auto AddToken(TokenInfo info) -> Token;
  [[nodiscard]] auto GetTokenPrintWidths(Token token) const -> PrintWidths;
  auto PrintToken(llvm::raw_ostream& output_stream, Token token,
                  PrintWidths widths) const -> void;

  SourceBuffer* source_;

  llvm::SmallVector<TokenInfo, 16> token_infos_;

  llvm::SmallVector<LineInfo, 16> line_infos_;

  llvm::SmallVector<IdentifierInfo, 16> identifier_infos_;

  llvm::SmallVector<llvm::APInt, 16> literal_int_storage_;

  llvm::SmallVector<std::string, 16> literal_string_storage_;

  llvm::DenseMap<llvm::StringRef, Identifier> identifier_map_;

  bool has_errors_ = false;
};

using LexerDiagnosticEmitter = DiagnosticEmitter<const char*>;

using TokenDiagnosticEmitter = DiagnosticEmitter<TokenizedBuffer::Token>;

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_TOKENIZED_BUFFER_H
