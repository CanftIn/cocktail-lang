#ifndef COCKTAIL_LEXER_STRING_LITERAL_H
#define COCKTAIL_LEXER_STRING_LITERAL_H

#include <Cocktail/Diagnostics/DiagnosticEmitter.h>

#include <string>

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail {

struct LexedStringLiteral {
 public:
  static auto Lex(llvm::StringRef source_text)
      -> llvm::Optional<LexedStringLiteral>;

  auto ComputeValue(DiagnosticEmitter<const char*>& emitter) const
      -> std::string;

  [[nodiscard]] auto text() const -> llvm::StringRef { return text_; }

  [[nodiscard]] auto is_multi_line() const -> bool { return multi_line_; }

  [[nodiscard]] auto is_terminated() const -> bool { return is_terminated_; }

 private:
  LexedStringLiteral(llvm::StringRef text, llvm::StringRef content,
                     int hash_level, bool multi_line, bool is_terminated)
      : text_(text),
        content_(content),
        hash_level_(hash_level),
        multi_line_(multi_line),
        is_terminated_(is_terminated) {}

  llvm::StringRef text_;

  llvm::StringRef content_;

  int hash_level_;

  bool multi_line_;

  bool is_terminated_;
};

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_STRING_LITERAL_H