#ifndef COCKTAIL_TESTING_LEXER_T_H
#define COCKTAIL_TESTING_LEXER_T_H

#include <gmock/gmock.h>

#include <array>

#include "Cocktail/Common/StringHelpers.h"
#include "Cocktail/Diagnostics/DiagnosticEmitter.h"

namespace Cocktail::Testing {

// 将诊断信息（例如错误、警告等）的位置从字符指针转换为更易读的行号和列号格式。
// 例如"`12.5`:1:3"。
class SingleTokenDiagnosticTranslator
    : public DiagnosticLocationTranslator<const char*> {
 public:
  explicit SingleTokenDiagnosticTranslator(llvm::StringRef token)
      : token_(token) {}

  auto GetLocation(const char* pos) -> DiagnosticLocation override {
    // 确保给定的位置 pos 确实在 token_ 中。
    COCKTAIL_CHECK(StringRefContainsPointer(token_, pos))
        << "invalid diagnostic location";
    llvm::StringRef prefix = token_.take_front(pos - token_.begin());
    // 将 token 分割为两部分：最后一个换行符之前的部分和最后一个换行符之后的部分。
    auto [before_last_newline, this_line] = prefix.rsplit('\n');
    // 根据这两部分，计算出行号和列号。
    if (before_last_newline.size() == prefix.size()) {
      return {.line_number = 1,
              .column_number = static_cast<int32_t>(pos - token_.begin() + 1)};
    } else {
      return {.line_number =
                  static_cast<int32_t>(before_last_newline.count('\n') + 2),
              .column_number = static_cast<int32_t>(this_line.size() + 1)};
    }
  }

 private:
  llvm::StringRef token_; // 要进行词法分析的字符串。
};

}  // namespace Cocktail::Testing

#endif  // COCKTAIL_TESTING_LEXER_T_H