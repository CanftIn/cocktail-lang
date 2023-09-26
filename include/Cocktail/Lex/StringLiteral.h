#ifndef COCKTAIL_LEX_STRING_LITERAL_H
#define COCKTAIL_LEX_STRING_LITERAL_H

#include <optional>
#include <string>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail::Lex {

struct StringLiteral {
 public:
  // 用于从给定的文本中提取字符串字面量。
  static auto Lex(llvm::StringRef source_text) -> std::optional<StringLiteral>;

  // 用于计算字符串字面量的实际值，同时处理任何转义序列。用于parser。
  auto ComputeValue(DiagnosticEmitter<const char*>& emitter) const
      -> std::string;

  // 返回字符串字面量的完整文本。
  [[nodiscard]] auto text() const -> llvm::StringRef { return text_; }

  // 用于判断字符串字面量是否为多行字符串。
  [[nodiscard]] auto is_multi_line() const -> bool { return multi_line_; }

  // 用于判断字符串字面量是否有有效的终止符。
  [[nodiscard]] auto is_terminated() const -> bool { return is_terminated_; }

 private:
  enum MultiLineKind : int8_t {
    NotMultiLine,  // 不是一个多行字符串字面量。
    MultiLine,     // 多行字符串字面量，但没有使用双引号。
    MultiLineWithDoubleQuotes  // 多行字符串字面量，并且使用了双引号。
  };

  /// 用于表示字符串字面量的引入部分。
  /// 引入部分是指字符串字面量开始的部分。
  struct Introducer;

  explicit StringLiteral(llvm::StringRef text, llvm::StringRef content,
                         int hash_level, MultiLineKind multi_line,
                         bool is_terminated)
      : text_(text),
        content_(content),
        hash_level_(hash_level),
        multi_line_(multi_line),
        is_terminated_(is_terminated) {}

  // 存储了一个字符串字面量的完整文本内容的引用。
  llvm::StringRef text_;
  // 存储了字符串字面值的实际内容的引用。
  // 对于多行字符串字面值，它的起始点是紧跟在文件类型指示符后的换行符，
  // 并在结束于闭合的`"""`符号之前。注意，此处的前导空白字符不会被移除。
  llvm::StringRef content_;
  // 记录了在字符串字面量的开头的`"`或`"""`前面的`#`符号的数量级。
  // 通常用于确定字符串字面量的缩进级别或其他特性。
  int hash_level_;
  // 表示字符串字面量是否为多行字符串。
  MultiLineKind multi_line_;
  // 用于指示字符串字面量是否已经正确终结，或者它是否只能用于表示错误情况。
  /// TODO: 如果字符串字面量未正确终结，则不是有效的字符串字面量。
  bool is_terminated_;
};

}  // namespace Cocktail::Lex

#endif  // COCKTAIL_LEX_STRING_LITERAL_H