#ifndef COCKTAIL_TESTING_LEXER_T_H
#define COCKTAIL_TESTING_LEXER_T_H

#include <gmock/gmock.h>

#include <array>
#include <string>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/FormatVariadic.h"

namespace Cocktail::Testing {

// A diagnostic translator for tests that lex a single token. Produces
// locations such as "`12.5`:1:3" to refer to the third character in the token.
class SingleTokenDiagnosticTranslator
    : public DiagnosticLocationTranslator<const char*> {
 public:
  // Form a translator for a given token. The string provided here must refer
  // to the same character array that we are going to lex.
  explicit SingleTokenDiagnosticTranslator(llvm::StringRef token)
      : token(token) {}

  auto GetLocation(const char* pos) -> DiagnosticLocation override {
    assert(llvm::is_sorted(std::array{token.begin(), pos, token.end()}) &&
           "invalid diagnostic location");
    llvm::StringRef prefix = token.take_front(pos - token.begin());
    auto [before_last_newline, this_line] = prefix.rsplit('\n');
    if (before_last_newline.size() == prefix.size()) {
      // On first line.
      return {.file_name = SynthesizeFilename(),
              .line_number = 1,
              .column_number = static_cast<int32_t>(pos - token.begin() + 1)};
    } else {
      // On second or subsequent lines. Note that the line number here is 2
      // more than the number of newlines because `rsplit` removed one newline
      // and `line_number` is 1-based.
      return {.file_name = SynthesizeFilename(),
              .line_number =
                  static_cast<int32_t>(before_last_newline.count('\n') + 2),
              .column_number = static_cast<int32_t>(this_line.size() + 1)};
    }
  }

 private:
  [[nodiscard]] auto SynthesizeFilename() const -> std::string {
    return llvm::formatv("`{0}`", token);
  }

  llvm::StringRef token;
};

}  // namespace Cocktail::Testing

#endif  // COCKTAIL_TESTING_LEXER_T_H