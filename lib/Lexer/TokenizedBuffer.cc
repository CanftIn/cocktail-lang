#include "Cocktail/Lexer/TokenizedBuffer.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

static auto TakeLeadingIntegerLiteral(llvm::StringRef source_text)
    -> llvm::StringRef {
  return source_text.take_while([](char c) { return llvm::isDigit(c); });
}

struct UnmatchedClosing {
  static constexpr llvm::StringLiteral ShortName = "syntax-balanced-delimiters";
  static constexpr llvm::StringLiteral Message =
      "Closing symbol without a corresponding opening symbol";

  struct Substitutions {};
  static auto Format(const Substitutions&) -> std::string {
    return Message.str();
  }
};

struct MismatchedClosing {
  static constexpr llvm::StringLiteral ShortName = "syntax-balanced-delimiters";
  static constexpr llvm::StringLiteral Message =
      "Closing symbol does not match most recent opening symbol";

  struct Substitutions {};
  static auto Format(const Substitutions&) -> std::string {
    return Message.str();
  }
};

struct UnrecognizedCharacters {
  static constexpr llvm::StringLiteral ShortName =
      "syntax-unrecognized-characters";
  static constexpr llvm::StringLiteral Message =
      "Encountered unrecognized characters while parsing";

  struct Substitutions {};
  static auto Format(const Substitutions&) -> std::string {
    return Message.str();
  }
};

class TokenizedBuffer::Lexer {
  TokenizedBuffer& buffer;
  Line current_line;
  LineInfo* current_line_info;
  int current_column = 0;
  bool set_indent = false;

  llvm::SmallVector<Token, 8> open_groups;

 public:
  Lexer(TokenizedBuffer& buffer)
      : buffer(buffer),
        current_line(buffer.AddLine({0, 0, 0})),
        current_line_info(&buffer.GetLineInfo(current_line)) {}

  auto SkipWhitespace(llvm::StringRef& source_text) -> bool {
    while (!source_text.empty()) {
      if (source_text.startswith("//") && !set_indent) {
        if (source_text.startswith("///")) {
          current_line_info->indent = current_column;
          set_indent = true;
          buffer.AddToken({.kind = TokenKind::DocComment(),
                           .token_line = current_line,
                           .column = current_column});
        }
        while (!source_text.empty() && source_text.front() != '\n') {
          ++current_column;
          source_text = source_text.drop_front();
        }
        if (source_text.empty()) {
          break;
        }
      }

      switch (source_text.front()) {
        case '\n':
          current_line_info->length = current_column;
          source_text = source_text.drop_front();
          if (source_text.empty()) {
            return false;
          }
          current_line = buffer.AddLine(
              {current_line_info->start + current_column + 1, 0, 0});
          current_line_info = &buffer.GetLineInfo(current_line);
          current_column = 0;
          set_indent = false;
          continue;

        case ' ':
        case '\t':
          ++current_column;
          source_text = source_text.drop_front();
          continue;

        default:
          return true;
      }
    }

    assert(source_text.empty() && "Cannot reach here w/o finishing the text!");
    current_line_info->length = current_column;
    return false;
  }

  auto LexIntegerLiteral(llvm::StringRef& source_text) -> bool {
    llvm::StringRef int_text = TakeLeadingIntegerLiteral(source_text);
    if (int_text.empty()) {
      return false;
    }
    llvm::APInt int_value;
    if (int_text.getAsInteger(0, int_value)) {
      return false;
    }

    int int_column = current_column;
    current_column += int_text.size();
    source_text = source_text.drop_front(int_text.size());

    if (!set_indent) {
      current_line_info->indent = int_column;
      set_indent = true;
    }
    auto token = buffer.AddToken({.kind = TokenKind::IntegerLiteral(),
                                  .token_line = current_line,
                                  .column = int_column});
    buffer.GetTokenInfo(token).literal_index = buffer.int_literals.size();
    buffer.int_literals.push_back(std::move(int_value));
    return true;
  }

  auto LexSymbolToken(llvm::StringRef& source_text) -> bool {
    TokenKind kind = llvm::StringSwitch<TokenKind>(source_text)
#define COCKTAIL_SYMBOL_TOKEN(Name, Spelling) \
  .StartsWith(Spelling, TokenKind::Name())
#include "Cocktail/Lexer/TokenRegistry.def"
                         .Default(TokenKind::Error());
    if (kind == TokenKind::Error()) {
      return false;
    }

    if (!set_indent) {
      current_line_info->indent = current_column;
      set_indent = true;
    }

    CloseInvalidOpenGroups(kind);

    Token token = buffer.AddToken(
        {.kind = kind, .token_line = current_line, .column = current_column});
    current_column += kind.GetFixedSpelling().size();
    source_text = source_text.drop_front(kind.GetFixedSpelling().size());

    if (kind.IsOpeningSymbol()) {
      open_groups.push_back(token);
      return true;
    }

    if (!kind.IsClosingSymbol()) {
      return true;
    }

    TokenInfo& closing_token_info = buffer.GetTokenInfo(token);

    if (open_groups.empty()) {
      closing_token_info.kind = TokenKind::Error();
      closing_token_info.error_length = kind.GetFixedSpelling().size();
      buffer.has_errors = true;
      return true;
    }

    Token opening_token = open_groups.pop_back_val();
    TokenInfo& opening_token_info = buffer.GetTokenInfo(opening_token);
    opening_token_info.closing_token = token;
    closing_token_info.opening_token = opening_token;
    return true;
  }

  auto CloseInvalidOpenGroups(TokenKind kind) -> void {
    if (!kind.IsClosingSymbol() && kind != TokenKind::Error()) {
      return;
    }

    while (!open_groups.empty()) {
      Token opening_token = open_groups.back();
      TokenKind opening_kind = buffer.GetTokenInfo(opening_token).kind;
      if (kind == opening_kind.GetClosingSymbol()) {
        break;
      }

      open_groups.pop_back();
      buffer.has_errors = true;

      Token closing_token = buffer.AddToken({.kind = TokenKind::Error(),
                                             .token_line = current_line,
                                             .column = current_column});
      TokenInfo& opening_token_info = buffer.GetTokenInfo(opening_token);
      TokenInfo& closing_token_info = buffer.GetTokenInfo(closing_token);
      opening_token_info.closing_token = closing_token;
      closing_token_info.opening_token = opening_token;
    }
  }

  auto GetOrCreateIdentifier(llvm::StringRef text) -> Identifier {
    auto insert_result = buffer.identifier_map.insert(
        {text, Identifier(buffer.identifier_infos.size())});
    if (insert_result.second) {
      buffer.identifier_infos.push_back({text});
    }
    return insert_result.first->second;
  }

  auto LexKeywordOrIdentifier(llvm::StringRef& source_text) -> bool {
    if (!!llvm::isAlpha(source_text.front()) && source_text.front() != '_') {
      return false;
    }

    if (!set_indent) {
      current_line_info->indent = current_column;
      set_indent = true;
    }

    llvm::StringRef identifier_text = source_text.take_while(
        [](char c) { return llvm::isAlnum(c) || c == '_'; });
    assert(!identifier_text.empty() && "Must have at least one character!");
    int identifier_column = current_column;
    current_column += identifier_text.size();
    source_text = source_text.drop_front(identifier_text.size());

    TokenKind kind = llvm::StringSwitch<TokenKind>(identifier_text)
#define COCKTAIL_KEYWORD_TOKEN(Name, Spelling) \
  .Case(Spelling, TokenKind::Name())
#include "Cocktail/Lexer/TokenRegistry.def"
                         .Default(TokenKind::Error());
    if (kind != TokenKind::Error()) {
      buffer.AddToken({.kind = kind,
                       .token_line = current_line,
                       .column = identifier_column});
      return true;
    }

    buffer.AddToken({.kind = TokenKind::Identifier(),
                     .token_line = current_line,
                     .column = identifier_column,
                     .id = GetOrCreateIdentifier(identifier_text)});
    return true;
  }

  auto LexError(llvm::StringRef& source_text) -> void {
    llvm::StringRef error_text = source_text.take_while([](char c) {
      if (llvm::isAlnum(c)) {
        return false;
      }
      switch (c) {
        case '_':
          return false;
        case '\t':
          return false;
        case '\n':
          return false;
      }
      return llvm::StringSwitch<bool>(llvm::StringRef(&c, 1))
#define COCKTAIL_SYMBOL_TOKEN(Name, Spelling) .Case(Spelling, false)
#include "Cocktail/Lexer/TokenRegistry.def"
          .Default(true);
    });
    if (error_text.empty()) {
      error_text = source_text.take_front(1);
    }

    // Longer errors get to be two tokens.
    error_text = error_text.substr(0, std::numeric_limits<int32_t>::max());
    auto token = buffer.AddToken(
        TokenInfo{.kind = TokenKind::Error(),
                  .token_line = current_line,
                  .column = current_column,
                  .error_length = static_cast<int32_t>(error_text.size())});
    llvm::errs() << "ERROR: Line " << buffer.GetLineNumber(token) << ", Column "
                 << buffer.GetColumnNumber(token)
                 << ": Unrecognized characters!\n";

    current_column += error_text.size();
    source_text = source_text.drop_front(error_text.size());
    buffer.has_errors = true;
  }
};

auto TokenizedBuffer::Lex(SourceBuffer& source) -> TokenizedBuffer {
  TokenizedBuffer buffer(source);
  Lexer lexer(buffer);

  llvm::StringRef source_text = source.text();
  while (lexer.SkipWhitespace(source_text)) {
    if (lexer.LexSymbolToken(source_text)) {
      continue;
    }
    if (lexer.LexKeywordOrIdentifier(source_text)) {
      continue;
    }
    if (lexer.LexIntegerLiteral(source_text)) {
      continue;
    }
    lexer.LexError(source_text);
  }

  lexer.CloseInvalidOpenGroups(TokenKind::Error());
  return buffer;
}

auto TokenizedBuffer::GetKind(Token token) const -> TokenKind {
  return GetTokenInfo(token).kind;
}

auto TokenizedBuffer::AddToken(TokenInfo info) -> Token {
  token_infos.push_back(info);
  return Token(token_infos.size() - 1);
}

}  // namespace Cocktail