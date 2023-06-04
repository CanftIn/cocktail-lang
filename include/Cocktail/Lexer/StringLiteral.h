#ifndef COCKTAIL_LEXER_STRING_LITERAL_H
#define COCKTAIL_LEXER_STRING_LITERAL_H

#include <Cocktail/Diagnostics/DiagnosticEmitter.h>

#include <string>

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail {

struct LexedStringLiteral {
 public:
  [[nodiscard]] auto Text() const -> llvm::StringRef { return text; }

  [[nodiscard]] auto IsMultiLine() const -> bool { return multi_line; }

  static auto Lex(llvm::StringRef source_text)
      -> llvm::Optional<LexedStringLiteral>;

  auto ComputeValue(DiagnosticEmitter<const char*>& emitter) const
      -> std::string;

 private:
  LexedStringLiteral(llvm::StringRef text, llvm::StringRef content,
                     int hash_level, bool multi_line)
      : text(text),
        content(content),
        hash_level(hash_level),
        multi_line(multi_line) {}

  llvm::StringRef text;

  llvm::StringRef content;

  int hash_level;

  bool multi_line;
};

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_STRING_LITERAL_H