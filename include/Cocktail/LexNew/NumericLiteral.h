#ifndef COCKTAIL_LEXNEW_NUMERIC_LITERAL_H
#define COCKTAIL_LEXNEW_NUMERIC_LITERAL_H

#include <optional>
#include <variant>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail::Lex {

// 表示从源代码缓冲区中提取的数值字面量。
class NumericLiteral {
 public:
  enum class Radix : uint8_t { Binary = 2, Decimal = 10, Hexadecimal = 16 };

  // 表示整数字面量的值。
  struct IntegerValue {
    llvm::APInt value;
  };

  // 表示实数字面量的值。
  struct RealValue {
    Radix radix;  // 表示指数的基数，可以是二进制或十进制。
    llvm::APInt mantissa;  // 表示实数的尾数，存储为任意精度的无符号整数。
    llvm::APInt exponent;  // 表示实数的指数，存储为任意精度的有符号整数。
  };

  // 表示无法恢复的错误。
  struct UnrecoverableError {};

  // 表示数值字面量的值。
  using Value = std::variant<IntegerValue, RealValue, UnrecoverableError>;

  // 用于从给定的文本中提取数值字面量。
  static auto Lex(llvm::StringRef source_text) -> std::optional<NumericLiteral>;

  // 计算数值字面量的值。如果字面量无效，它会向给定的发射器发出诊断信息。
  auto ComputeValue(DiagnosticEmitter<const char*>& emitter) const -> Value;

  // 返回与此字面量对应的文本。
  [[nodiscard]] auto text() const -> llvm::StringRef { return text_; }

 private:
  NumericLiteral() = default;

  class Parser;

  // 表示数值字面量的文本。
  llvm::StringRef text_;

  // 表示小数点'.'的偏移量。
  int radix_point_;

  // 表示引入指数的字母字符的偏移量。
  // 在有效的字面量中，这将是一个'e'或'p'，并且可能后跟一个'+'或'-'，
  // 但对于错误恢复，这可能只是无效令牌中的最后一个小写字母。
  // exponent_始终大于或等于radix_point_。如果没有指数，则设置为text_的大小。
  int exponent_;
};

}  // namespace Cocktail::Lex

#endif  // COCKTAIL_LEXNEW_NUMERIC_LITERAL_H