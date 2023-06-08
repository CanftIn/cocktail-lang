#include "Cocktail/Lexer/TokenizedBuffer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <string>

#include "Cocktail/Common/CharacterSet.h"
#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Cocktail/Lexer/NumericLiteral.h"
#include "Cocktail/Lexer/StringLiteral.h"
#include "Cocktail/Lexer/TokenKind.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

struct TrailingComment : SimpleDiagnostic<TrailingComment> {
  static constexpr llvm::StringLiteral ShortName = "syntax-comments";
  static constexpr llvm::StringLiteral Message =
      "Trailing comments are not permitted.";
};

struct NoWhitespaceAfterCommentIntroducer
    : SimpleDiagnostic<NoWhitespaceAfterCommentIntroducer> {
  static constexpr llvm::StringLiteral ShortName = "syntax-comments";
  static constexpr llvm::StringLiteral Message =
      "Whitespace is required after '//'.";
};

struct UnmatchedClosing : SimpleDiagnostic<UnmatchedClosing> {
  static constexpr llvm::StringLiteral ShortName = "syntax-balanced-delimiters";
  static constexpr llvm::StringLiteral Message =
      "Closing symbol without a corresponding opening symbol";
};

struct MismatchedClosing : SimpleDiagnostic<MismatchedClosing> {
  static constexpr llvm::StringLiteral ShortName = "syntax-balanced-delimiters";
  static constexpr llvm::StringLiteral Message =
      "Closing symbol does not match most recent opening symbol";
};

struct UnrecognizedCharacters : SimpleDiagnostic<UnrecognizedCharacters> {
  static constexpr llvm::StringLiteral ShortName =
      "syntax-unrecognized-characters";
  static constexpr llvm::StringLiteral Message =
      "Encountered unrecognized characters while parsing";
};

template <typename... Fs>
struct Overload : Fs... {
  using Fs::operator()...;
};

template <typename... Fs>
Overload(Fs...) -> Overload<Fs...>;

template <typename V, typename... Fs>
auto VariantMatch(V&& v, Fs&&... fs) -> decltype(auto) {
  return std::visit(Overload{std::forward<Fs>(fs)...}, std::forward<V>(v));
}

class TokenizedBuffer::Lexer {
  TokenizedBuffer& buffer;

  SourceBufferLocationTranslator translator;
  LexerDiagnosticEmitter emitter;

  TokenLocationTranslator token_translator;
  TokenDiagnosticEmitter token_emitter;

  Line current_line;
  LineInfo* current_line_info;

  int current_column = 0;
  bool set_indent = false;

  llvm::SmallVector<Token, 8> open_groups;

 public:
  Lexer(TokenizedBuffer& buffer, DiagnosticConsumer& consumer)
      : buffer(buffer),
        translator(buffer),
        emitter(translator, consumer),
        token_translator(buffer),
        token_emitter(token_translator, consumer),
        current_line(buffer.AddLine({0, 0, 0})),
        current_line_info(&buffer.GetLineInfo(current_line)) {}

  class LexResult {
    bool formed_token;
    explicit LexResult(bool formed_token) : formed_token(formed_token) {}

   public:
    // permit implicit conversion from Token
    LexResult(Token /*unused*/) : LexResult(true) {}

    static auto NoMatch() -> LexResult { return LexResult(false); }

    explicit operator bool() const { return formed_token; }
  };

  auto HandleNewline() -> void {
    current_line_info->length = current_column;

    current_line =
        buffer.AddLine({current_line_info->start + current_column + 1, 0, 0});
    current_line_info = &buffer.GetLineInfo(current_line);
    current_column = 0;
    set_indent = false;
  }

  auto NoteWhitespace() -> void {
    if (!buffer.token_infos.empty()) {
      buffer.token_infos.back().has_trailing_space = true;
    }
  }

  auto SkipWhitespace(llvm::StringRef& source_text) -> bool {
    const char* const whitespace_start = source_text.begin();

    while (!source_text.empty()) {
      if (source_text.startswith("//")) {
        // Any comment must be the only non-whitespace on the line.
        if (set_indent) {
          emitter.EmitError<TrailingComment>(source_text.begin());
        }
        // The introducer '//' must be followed by whitespace or EOF.
        if (source_text.size() > 2 && !IsSpace(source_text[2])) {
          emitter.EmitError<NoWhitespaceAfterCommentIntroducer>(
              source_text.begin() + 2);
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
        default:
          assert(!IsSpace(source_text.front()));
          if (whitespace_start != source_text.begin()) {
            NoteWhitespace();
          }
          return true;

        case '\n':
          source_text = source_text.drop_front();
          if (source_text.empty()) {
            current_line_info->length = current_column;
            return false;
          }
          HandleNewline();
          continue;

        case ' ':
        case '\t':
          ++current_column;
          source_text = source_text.drop_front();
          continue;
      }
    }

    assert(source_text.empty() &&
           "Cannot reach here without finishing the text!");
    current_line_info->length = current_column;
    return false;
  }

  auto LexNumericLiteral(llvm::StringRef& source_text) -> LexResult {
    llvm::Optional<LexedNumericLiteral> literal =
        LexedNumericLiteral::Lex(source_text);
    if (!literal) {
      return LexResult::NoMatch();
    }

    int int_column = current_column;
    int token_size = literal->Text().size();
    current_column += token_size;
    source_text = source_text.drop_front(token_size);

    if (!set_indent) {
      current_line_info->indent = int_column;
      set_indent = true;
    }

    return VariantMatch(
        literal->ComputeValue(emitter),
        [&](LexedNumericLiteral::IntegerValue&& value) {
          auto token = buffer.AddToken({.kind = TokenKind::IntegerLiteral(),
                                        .token_line = current_line,
                                        .column = int_column});
          buffer.GetTokenInfo(token).literal_index =
              buffer.literal_int_storage.size();
          buffer.literal_int_storage.push_back(std::move(value.value));
          return token;
        },
        [&](LexedNumericLiteral::RealValue&& value) {
          auto token = buffer.AddToken({.kind = TokenKind::RealLiteral(),
                                        .token_line = current_line,
                                        .column = int_column});
          buffer.GetTokenInfo(token).literal_index =
              buffer.literal_int_storage.size();
          buffer.literal_int_storage.push_back(std::move(value.mantissa));
          buffer.literal_int_storage.push_back(std::move(value.exponent));
          assert(buffer.GetRealLiteral(token).IsDecimal() ==
                 (value.radix == 10));
          return token;
        },
        [&](LexedNumericLiteral::UnrecoverableError) {
          auto token = buffer.AddToken(TokenInfo{
              .kind = TokenKind::Error(),
              .token_line = current_line,
              .column = int_column,
              .error_length = token_size,
          });
          return token;
        });
  }

  auto LexStringLiteral(llvm::StringRef& source_text) -> LexResult {
    llvm::Optional<LexedStringLiteral> literal =
        LexedStringLiteral::Lex(source_text);
    if (!literal) {
      return LexResult::NoMatch();
    }

    Line string_line = current_line;
    int string_column = current_column;
    int literal_size = literal->Text().size();
    source_text = source_text.drop_front(literal_size);

    if (!set_indent) {
      current_line_info->indent = string_column;
      set_indent = true;
    }

    if (!literal->IsMultiLine()) {
      current_column += literal_size;
    } else {
      for (char c : literal->Text()) {
        if (c == '\n') {
          HandleNewline();
          current_line_info->indent = string_column;
          set_indent = true;
        } else {
          ++current_column;
        }
      }
    }

    auto token = buffer.AddToken({.kind = TokenKind::StringLiteral(),
                                  .token_line = string_line,
                                  .column = string_column});
    buffer.GetTokenInfo(token).literal_index =
        buffer.literal_string_storage.size();
    buffer.literal_string_storage.push_back(literal->ComputeValue(emitter));
    return token;
  }

  auto LexSymbolToken(llvm::StringRef& source_text) -> LexResult {
    TokenKind kind = llvm::StringSwitch<TokenKind>(source_text)
#define COCKTAIL_SYMBOL_TOKEN(Name, Spelling) \
  .StartsWith(Spelling, TokenKind::Name())
#include "Cocktail/Lexer/TokenRegistry.def"
                         .Default(TokenKind::Error());
    if (kind == TokenKind::Error()) {
      return LexResult::NoMatch();
    }

    if (!set_indent) {
      current_line_info->indent = current_column;
      set_indent = true;
    }

    CloseInvalidOpenGroups(kind);

    const char* location = source_text.begin();
    Token token = buffer.AddToken(
        {.kind = kind, .token_line = current_line, .column = current_column});
    current_column += kind.GetFixedSpelling().size();
    source_text = source_text.drop_front(kind.GetFixedSpelling().size());

    if (kind.IsOpeningSymbol()) {
      open_groups.push_back(token);
      return token;
    }

    if (!kind.IsClosingSymbol()) {
      return token;
    }

    TokenInfo& closing_token_info = buffer.GetTokenInfo(token);

    if (open_groups.empty()) {
      closing_token_info.kind = TokenKind::Error();
      closing_token_info.error_length = kind.GetFixedSpelling().size();

      emitter.EmitError<UnmatchedClosing>(location);
      return token;
    }

    Token opening_token = open_groups.pop_back_val();
    TokenInfo& opening_token_info = buffer.GetTokenInfo(opening_token);
    opening_token_info.closing_token = token;
    closing_token_info.opening_token = opening_token;
    return token;
  }

  auto CloseInvalidOpenGroups(TokenKind kind) -> void {
    if (!kind.IsClosingSymbol() && kind != TokenKind::Error()) {
      return;
    }

    while (!open_groups.empty()) {
      Token opening_token = open_groups.back();
      TokenKind opening_kind = buffer.GetTokenInfo(opening_token).kind;
      if (kind == opening_kind.GetClosingSymbol()) {
        return;
      }

      open_groups.pop_back();
      token_emitter.EmitError<MismatchedClosing>(opening_token);

      assert(!buffer.Tokens().empty() && "Must have a prior opening token!");
      Token prev_token = buffer.Tokens().end()[-1];

      Token closing_token = buffer.AddToken(
          {.kind = opening_kind.GetClosingSymbol(),
           .has_trailing_space = buffer.HasTrailingWhitespace(prev_token),
           .is_recovery = true,
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

  auto LexKeywordOrIdentifier(llvm::StringRef& source_text) -> LexResult {
    if (!IsAlpha(source_text.front()) && source_text.front() != '_') {
      return LexResult::NoMatch();
    }

    if (!set_indent) {
      current_line_info->indent = current_column;
      set_indent = true;
    }

    llvm::StringRef identifier_text =
        source_text.take_while([](char c) { return IsAlnum(c) || c == '_'; });
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
      return buffer.AddToken({.kind = kind,
                              .token_line = current_line,
                              .column = identifier_column});
    }

    return buffer.AddToken({.kind = TokenKind::Identifier(),
                            .token_line = current_line,
                            .column = identifier_column,
                            .id = GetOrCreateIdentifier(identifier_text)});
  }

  auto LexError(llvm::StringRef& source_text) -> LexResult {
    llvm::StringRef error_text = source_text.take_while([](char c) {
      if (IsAlnum(c)) {
        return false;
      }
      switch (c) {
        case '_':
        case '\t':
        case '\n':
          return false;
      }
      return llvm::StringSwitch<bool>(llvm::StringRef(&c, 1))
#define COCKTAIL_SYMBOL_TOKEN(Name, Spelling) .StartsWith(Spelling, false)
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
    emitter.EmitError<UnrecognizedCharacters>(error_text.begin());

    current_column += error_text.size();
    source_text = source_text.drop_front(error_text.size());
    return token;
  }

  auto AddEndOfFileToken() -> void {
    buffer.AddToken({.kind = TokenKind::EndOfFile(),
                     .token_line = current_line,
                     .column = current_column});
  }
};

auto TokenizedBuffer::Lex(SourceBuffer& source, DiagnosticConsumer& consumer)
    -> TokenizedBuffer {
  TokenizedBuffer buffer(source);
  ErrorTrackingDiagnosticConsumer error_tracking_consumer(consumer);
  Lexer lexer(buffer, error_tracking_consumer);

  llvm::StringRef source_text = source.Text();
  while (lexer.SkipWhitespace(source_text)) {
    Lexer::LexResult result = lexer.LexSymbolToken(source_text);
    if (!result) {
      result = lexer.LexKeywordOrIdentifier(source_text);
    }
    if (!result) {
      result = lexer.LexNumericLiteral(source_text);
    }
    if (!result) {
      result = lexer.LexStringLiteral(source_text);
    }
    if (!result) {
      result = lexer.LexError(source_text);
    }
    assert(result && "No token was lexed.");
  }

  lexer.NoteWhitespace();

  lexer.CloseInvalidOpenGroups(TokenKind::Error());
  lexer.AddEndOfFileToken();

  if (error_tracking_consumer.SeenError()) {
    buffer.has_errors = true;
  }

  return buffer;
}

auto TokenizedBuffer::GetKind(Token token) const -> TokenKind {
  return GetTokenInfo(token).kind;
}

auto TokenizedBuffer::GetLine(Token token) const -> Line {
  return GetTokenInfo(token).token_line;
}

auto TokenizedBuffer::GetLineNumber(Token token) const -> int {
  return GetLineNumber(GetLine(token));
}

auto TokenizedBuffer::GetColumnNumber(Token token) const -> int {
  return GetTokenInfo(token).column + 1;
}

auto TokenizedBuffer::GetTokenText(Token token) const -> llvm::StringRef {
  const auto& token_info = GetTokenInfo(token);
  llvm::StringRef fixed_spelling = token_info.kind.GetFixedSpelling();
  if (!fixed_spelling.empty()) {
    return fixed_spelling;
  }

  if (token_info.kind == TokenKind::Error()) {
    const auto& line_info = GetLineInfo(token_info.token_line);
    int64_t token_start = line_info.start + token_info.column;
    return source->Text().substr(token_start, token_info.error_length);
  }

  if (token_info.kind == TokenKind::IntegerLiteral() ||
      token_info.kind == TokenKind::RealLiteral()) {
    const auto& line_info = GetLineInfo(token_info.token_line);
    int64_t token_start = line_info.start + token_info.column;
    llvm::Optional<LexedNumericLiteral> relexed_token =
        LexedNumericLiteral::Lex(source->Text().substr(token_start));
    assert(relexed_token && "Could not reform numeric literal token.");
    return relexed_token->Text();
  }

  if (token_info.kind == TokenKind::StringLiteral()) {
    const auto& line_info = GetLineInfo(token_info.token_line);
    int64_t token_start = line_info.start + token_info.column;
    llvm::Optional<LexedStringLiteral> relexed_token =
        LexedStringLiteral::Lex(source->Text().substr(token_start));
    assert(relexed_token && "Could not reform string literal token.");
    return relexed_token->Text();
  }

  if (token_info.kind == TokenKind::EndOfFile()) {
    return llvm::StringRef();
  }

  assert(token_info.kind == TokenKind::Identifier() &&
         "Only identifiers have stored text!");
  return GetIdentifierText(token_info.id);
}

auto TokenizedBuffer::GetIdentifier(Token token) const -> Identifier {
  const auto& token_info = GetTokenInfo(token);
  assert(token_info.kind == TokenKind::Identifier() &&
         "The token must be an identifier!");
  return token_info.id;
}

auto TokenizedBuffer::GetIntegerLiteral(Token token) const
    -> const llvm::APInt& {
  const auto& token_info = GetTokenInfo(token);
  assert(token_info.kind == TokenKind::IntegerLiteral() &&
         "The token must be an integer literal!");
  return literal_int_storage[token_info.literal_index];
}

auto TokenizedBuffer::GetRealLiteral(Token token) const -> RealLiteralValue {
  const auto& token_info = GetTokenInfo(token);
  assert(token_info.kind == TokenKind::RealLiteral() &&
         "The token must be a real literal!");

  const auto& line_info = GetLineInfo(token_info.token_line);
  int64_t token_start = line_info.start + token_info.column;
  char second_char = source->Text()[token_start + 1];
  bool is_decimal = second_char != 'x' && second_char != 'b';

  return RealLiteralValue(this, token_info.literal_index, is_decimal);
}

auto TokenizedBuffer::GetStringLiteral(Token token) const -> llvm::StringRef {
  auto& token_info = GetTokenInfo(token);
  assert(token_info.kind == TokenKind::StringLiteral() &&
         "The token must be a string literal!");
  return literal_string_storage[token_info.literal_index];
}

auto TokenizedBuffer::GetMatchedClosingToken(Token opening_token) const
    -> Token {
  const auto& opening_token_info = GetTokenInfo(opening_token);
  assert(opening_token_info.kind.IsOpeningSymbol() &&
         "The token must be an opening group symbol!");
  return opening_token_info.closing_token;
}

auto TokenizedBuffer::GetMatchedOpeningToken(Token closing_token) const
    -> Token {
  const auto& closing_token_info = GetTokenInfo(closing_token);
  assert(closing_token_info.kind.IsClosingSymbol() &&
         "The token must be a closing group symbol!");
  return closing_token_info.opening_token;
}

auto TokenizedBuffer::HasLeadingWhitespace(Token token) const -> bool {
  auto it = TokenIterator(token);
  return it == Tokens().begin() || GetTokenInfo(*(it - 1)).has_trailing_space;
}

auto TokenizedBuffer::HasTrailingWhitespace(Token token) const -> bool {
  return GetTokenInfo(token).has_trailing_space;
}

auto TokenizedBuffer::IsRecoveryToken(Token token) const -> bool {
  return GetTokenInfo(token).is_recovery;
}

auto TokenizedBuffer::GetLineNumber(Line line) const -> int {
  return line.index + 1;
}

auto TokenizedBuffer::GetIndentColumnNumber(Line line) const -> int {
  return GetLineInfo(line).indent + 1;
}

auto TokenizedBuffer::GetIdentifierText(Identifier identifier) const
    -> llvm::StringRef {
  return identifier_infos[identifier.index].text;
}

auto TokenizedBuffer::PrintWidths::Widen(const PrintWidths& widths) -> void {
  index = std::max(index, widths.index);
  kind = std::max(kind, widths.kind);
  line = std::max(line, widths.line);
  column = std::max(column, widths.column);
  indent = std::max(indent, widths.indent);
}

static auto ComputeDecimalPrintedWidth(int number) -> int {
  assert(number >= 0 && "Negative numbers are not supported.");
  if (number == 0) {
    return 1;
  }

  return static_cast<int>(std::log10(number)) + 1;
}

auto TokenizedBuffer::GetTokenPrintWidths(Token token) const -> PrintWidths {
  PrintWidths widths = {};
  widths.index = ComputeDecimalPrintedWidth(token_infos.size());
  widths.kind = GetKind(token).Name().size();
  widths.line = ComputeDecimalPrintedWidth(GetLineNumber(token));
  widths.column = ComputeDecimalPrintedWidth(GetColumnNumber(token));
  widths.indent =
      ComputeDecimalPrintedWidth(GetIndentColumnNumber(GetLine(token)));
  return widths;
}

auto TokenizedBuffer::Print(llvm::raw_ostream& output_stream) const -> void {
  if (Tokens().begin() == Tokens().end()) {
    return;
  }

  PrintWidths widths = {};
  widths.index = ComputeDecimalPrintedWidth(token_infos.size());
  for (Token token : Tokens()) {
    widths.Widen(GetTokenPrintWidths(token));
  }

  for (Token token : Tokens()) {
    PrintToken(output_stream, token, widths);
    output_stream << "\n";
  }
}

auto TokenizedBuffer::PrintToken(llvm::raw_ostream& output_stream,
                                 Token token) const -> void {
  PrintToken(output_stream, token, {});
}

auto TokenizedBuffer::PrintToken(llvm::raw_ostream& output_stream, Token token,
                                 PrintWidths widths) const -> void {
  widths.Widen(GetTokenPrintWidths(token));
  int token_index = token.index;
  const auto& token_info = GetTokenInfo(token);
  llvm::StringRef token_text = GetTokenText(token);

  output_stream << llvm::formatv(
      "token: { index: {0}, kind: {1}, line: {2}, column: {3}, indent: {4}, "
      "spelling: '{5}'",
      llvm::format_decimal(token_index, widths.index),
      llvm::right_justify(
          (llvm::Twine("'") + token_info.kind.Name() + "'").str(),
          widths.kind + 2),
      llvm::format_decimal(GetLineNumber(token_info.token_line), widths.line),
      llvm::format_decimal(GetColumnNumber(token), widths.column),
      llvm::format_decimal(GetIndentColumnNumber(token_info.token_line),
                           widths.indent),
      token_text);

  if (token_info.kind == TokenKind::Identifier()) {
    output_stream << ", identifier: " << GetIdentifier(token).index;
  } else if (token_info.kind.IsOpeningSymbol()) {
    output_stream << ", closing_token: " << GetMatchedClosingToken(token).index;
  } else if (token_info.kind.IsClosingSymbol()) {
    output_stream << ", opening_token: " << GetMatchedOpeningToken(token).index;
  } else if (token_info.kind == TokenKind::StringLiteral()) {
    output_stream << ", value: `" << GetStringLiteral(token) << "`";
  }
  // TODO: Include value for numeric literals.

  if (token_info.has_trailing_space) {
    output_stream << ", has_trailing_space: true";
  }
  if (token_info.is_recovery) {
    output_stream << ", recovery: true";
  }

  output_stream << " }";
}

auto TokenizedBuffer::GetLineInfo(Line line) -> LineInfo& {
  return line_infos[line.index];
}

auto TokenizedBuffer::GetLineInfo(Line line) const -> const LineInfo& {
  return line_infos[line.index];
}

auto TokenizedBuffer::AddLine(LineInfo info) -> Line {
  line_infos.push_back(info);
  return Line(static_cast<int>(line_infos.size()) - 1);
}

auto TokenizedBuffer::GetTokenInfo(Token token) -> TokenInfo& {
  return token_infos[token.index];
}

auto TokenizedBuffer::GetTokenInfo(Token token) const -> const TokenInfo& {
  return token_infos[token.index];
}

auto TokenizedBuffer::AddToken(TokenInfo info) -> Token {
  token_infos.push_back(info);
  return Token(static_cast<int>(token_infos.size()) - 1);
}

auto TokenizedBuffer::SourceBufferLocationTranslator::GetLocation(
    const char* loc) -> Diagnostic::Location {
  assert(llvm::is_sorted(std::array{buffer->source->Text().begin(), loc,
                                    buffer->source->Text().end()}) &&
         "location not within buffer");
  int64_t offset = loc - buffer->source->Text().begin();

  auto* line_it = std::partition_point(
      buffer->line_infos.begin(), buffer->line_infos.end(),
      [offset](const LineInfo& line) { return line.start <= offset; });
  bool incomplete_line_info = line_it == buffer->line_infos.end();

  assert(line_it != buffer->line_infos.begin() &&
         "location precedes the start of the first line");
  --line_it;
  int line_number = line_it - buffer->line_infos.begin();
  int column_number = offset - line_it->start;

  if (incomplete_line_info) {
    column_number = 0;
    for (int64_t i = line_it->start; i != offset; ++i) {
      if (buffer->source->Text()[i] == '\n') {
        ++line_number;
        column_number = 0;
      } else {
        ++column_number;
      }
    }
  }

  return {.file_name = buffer->source->Filename().str(),
          .line_number = line_number + 1,
          .column_number = column_number + 1};
}

auto TokenizedBuffer::TokenLocationTranslator::GetLocation(Token token)
    -> Diagnostic::Location {
  auto& token_info = buffer->GetTokenInfo(token);
  auto& line_info = buffer->GetLineInfo(token_info.token_line);
  const char* token_start =
      buffer->source->Text().begin() + line_info.start + token_info.column;

  return SourceBufferLocationTranslator(*buffer).GetLocation(token_start);
}

}  // namespace Cocktail