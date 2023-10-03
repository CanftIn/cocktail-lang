#ifndef COCKTAIL_PARSE_PRECEDENCE_H
#define COCKTAIL_PARSE_PRECEDENCE_H

#include <optional>

#include "Cocktail/Lex/TokenKind.h"

namespace Cocktail::Parse {

// 定义了三种可能的操作符优先级关系。
// 注释提供了一个示例，说明了当给定两个操作符$和@以及一个表达式：
// `a $ b @ c` 时，应该如何解析该表达式。
enum class OperatorPriority : int8_t {
  // 左操作符有更高的优先级：`(a $ b) @ c`。
  LeftFirst = -1,
  // 表达式是歧义的，没有明确的优先级。
  Ambiguous = 0,
  // 右操作符有更高的优先级：`a $ (b @ c)`。
  RightFirst = 1,
};

// 定义了三种可能的操作符结合性。
enum class Associativity : int8_t {
  LeftToRight = -1,  // 从左到右结合。
  None = 0,          // 没有结合性。
  RightToLeft = 1    // 从右到左结合。
};

// 代表与操作符或表达式相关的优先级组。
class PrecedenceGroup {
 public:
  struct Trailing;

  // 这种类型的对象只能使用下面的静态工厂函数构造。
  PrecedenceGroup() = delete;

  // 获取后缀表达式的哨兵优先级。所有的操作符都比这个优先级低。
  // 这意味着后缀表达式（如函数调用或数组索引）在没有括号的情况下总是首先被解析。
  // 例如，对于 x++ 这样的后缀表达式，它的优先级是最高的。
  static auto ForPostfixExpression() -> PrecedenceGroup;

  // 获取顶级或括号内的表达式的优先级。所有的表达式操作符都比这个优先级高。
  // 这意味着在括号内或顶级的表达式不会被其他操作符打断。
  static auto ForTopLevelExpression() -> PrecedenceGroup;

  // 获取语句上下文的哨兵优先级。
  // 所有的操作符，包括像=和++这样的语句操作符，都比这个优先级高。
  // 这确保了在语句上下文中，操作符总是优先于其他结构。
  static auto ForExpressionStatement() -> PrecedenceGroup;

  // 获取解析类型表达式时的优先级。所有的类型操作符都比这个优先级高。
  // 这意味着在解析类型表达式时，类型操作符（如指针或数组修饰符）总是首先被解析。
  static auto ForType() -> PrecedenceGroup;

  // 查找给定的前缀操作符标记的操作符信息。
  // 处理前缀操作符，例如 !（逻辑非）或 ++（前缀递增）。
  static auto ForLeading(Lex::TokenKind kind) -> std::optional<PrecedenceGroup>;

  // 查找给定的中缀或后缀操作符标记的操作符信息。
  // infix参数指示这是否是一个有效的中缀操作符，但只在同一个操作符符号既
  // 可以作为中缀又可以作为后缀时考虑。
  // 处理后缀和中缀操作符，例如 *（乘法或指针类型）。
  static auto ForTrailing(Lex::TokenKind kind, bool infix)
      -> std::optional<Trailing>;

  // 比较两个PrecedenceGroup对象的优先级。
  friend auto operator==(PrecedenceGroup lhs, PrecedenceGroup rhs) -> bool {
    return lhs.level_ == rhs.level_;
  }
  friend auto operator!=(PrecedenceGroup lhs, PrecedenceGroup rhs) -> bool {
    return lhs.level_ != rhs.level_;
  }

  // 比较两个相邻操作符的优先级。
  static auto GetPriority(PrecedenceGroup left, PrecedenceGroup right)
      -> OperatorPriority;

  // 获取此优先级组的结合性。
  [[nodiscard]] auto GetAssociativity() const -> Associativity {
    return static_cast<Associativity>(GetPriority(*this, *this));
  }

 private:
  // 通过int8_t进行隐式转换。
  // NOLINTNEXTLINE(google-explicit-constructor)
  PrecedenceGroup(int8_t level) : level_(level) {}

  // 表示优先级。
  int8_t level_;
};

// 表示后缀操作符的优先级信息。
struct PrecedenceGroup::Trailing {
  // 表示优先级。
  PrecedenceGroup level;
  // 表示这是否是一个二元中缀操作符。
  // 二元中缀操作符：true，一元后缀操作符：false。
  bool is_binary;
};

}  // namespace Cocktail::Parse

#endif  // COCKTAIL_PARSE_PRECEDENCE_H