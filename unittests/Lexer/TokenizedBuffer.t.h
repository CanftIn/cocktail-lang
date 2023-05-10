#ifndef COCKTAIL_LEXER_TOKENIZED_BUFFER_T_H
#define COCKTAIL_LEXER_TOKENIZED_BUFFER_T_H

#include <gmock/gmock.h>

#include "Cocktail/Lexer/TokenizedBuffer.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/YAMLParser.h"

namespace Cocktail {

inline void PrintTo(const TokenizedBuffer& buffer, std::ostream* output) {
  std::string message;
  llvm::raw_string_ostream message_stream(message);
  message_stream << "\n";
  buffer.Print(message_stream);
  *output << message_stream.str();
}

namespace Testing {

struct ExpectedToken {
  friend auto operator<<(std::ostream& output, const ExpectedToken& expected)
      -> std::ostream& {
    output << "\ntoken: { kind: '" << expected.kind.Name().str();
    if (expected.line != -1) {
      output << "', line: " << expected.line;
    }
    if (expected.column != -1) {
      output << ", column " << expected.column;
    }
    if (expected.indent_column != -1) {
      output << ", indent: " << expected.indent_column;
    }
    if (!expected.text.empty()) {
      output << ", spelling: '" << expected.text.str() << "'";
    }
    if (expected.recovery) {
      output << ", recovery: true";
    }
    output << " }";
    return output;
  }

  TokenKind kind;
  int line = -1;
  int column = -1;
  int indent_column = -1;
  bool recovery = false;
  llvm::StringRef text = "";
};

MATCHER_P(HasTokens, raw_all_expected, "") {
  const TokenizedBuffer& buffer = arg;
  llvm::ArrayRef<ExpectedToken> all_expected(raw_all_expected);

  bool matches = true;
  auto buffer_it = buffer.Tokens().begin();
  for (const ExpectedToken& expected : all_expected) {
    if (buffer_it == buffer.Tokens().end()) {
      break;
    }

    int index = buffer_it - buffer.Tokens().begin();
    auto token = *buffer_it++;

    TokenKind actual_kind = buffer.GetKind(token);
    if (actual_kind != expected.kind) {
      *result_listener << "\nToken " << index << " is a "
                       << actual_kind.Name().str() << ", expected a "
                       << expected.kind.Name().str() << ".";
      matches = false;
    }

    int actual_line = buffer.GetLineNumber(token);
    if (expected.line != -1 && actual_line != expected.line) {
      *result_listener << "\nToken " << index << " is at line " << actual_line
                       << ", expected " << expected.line << ".";
      matches = false;
    }

    int actual_column = buffer.GetColumnNumber(token);
    if (expected.column != -1 && actual_column != expected.column) {
      *result_listener << "\nToken " << index << " is at column "
                       << actual_column << ", expected " << expected.column
                       << ".";
      matches = false;
    }

    int actual_indent_column =
        buffer.GetIndentColumnNumber(buffer.GetLine(token));
    if (expected.indent_column != -1 &&
        actual_indent_column != expected.indent_column) {
      *result_listener << "\nToken " << index << " has column indent "
                       << actual_indent_column << ", expected "
                       << expected.indent_column << ".";
      matches = false;
    }

    int actual_recovery = buffer.IsRecoveryToken(token);
    if (expected.recovery != actual_recovery) {
      *result_listener << "\nToken " << index << " is "
                       << (actual_recovery ? "recovery" : "non-recovery")
                       << ", expected "
                       << (expected.recovery ? "recovery" : "non-recovery")
                       << ".";
      matches = false;
    }

    llvm::StringRef actual_text = buffer.GetTokenText(token);
    if (!expected.text.empty() && actual_text != expected.text) {
      *result_listener << "\nToken " << index << " has spelling `"
                       << actual_text.str() << "`, expected `"
                       << expected.text.str() << "`.";
      matches = false;
    }
  }

  int actual_size = buffer.Tokens().end() - buffer.Tokens().begin();
  if (static_cast<int>(all_expected.size()) != actual_size) {
    *result_listener << "\nExpected " << all_expected.size()
                     << " tokens but found " << actual_size << ".";
    matches = false;
  }
  return matches;
}

}  // namespace Testing

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_TOKENIZED_BUFFER_T_H