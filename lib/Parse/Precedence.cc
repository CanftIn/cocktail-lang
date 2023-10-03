#include "Cocktail/Parse/Precedence.h"

#include "Cocktail/Common/Check.h"

namespace Cocktail::Parse {

namespace {
// 定义不同的操作符优先级。
enum PrecedenceLevel : int8_t {
  // 哨兵值，表示最高的优先级，不代表任何操作符。
  Highest,
  // Terms.
  TermPrefix,
  // Numeric.
  IncrementDecrement,
  NumericPrefix,
  Modulo,
  Multiplicative,
  Additive,
  // Bitwise.
  BitwisePrefix,
  BitwiseAnd,
  BitwiseOr,
  BitwiseXor,
  BitShift,
  // Type formation.
  TypePrefix,
  TypePostfix,
  // Casts.
  As,
  // Logical.
  LogicalPrefix,
  Relational,
  LogicalAnd,
  LogicalOr,
  // Conditional.
  If,
  // Assignment.
  Assignment,
  // 哨兵值，表示最低的优先级。
  Lowest,
};
// 表示优先级的数量。
constexpr int8_t NumPrecedenceLevels = Lowest + 1;

// 存储操作符之间的优先级关系。
// 包含一个二维数组 table，其中 table[a][b] 表示操作符 a 和 b 之间的优先级关系。
struct OperatorPriorityTable {
  // 初始化操作符优先级表。
  constexpr OperatorPriorityTable() : table() {
    // Start with a list of <higher precedence>, <lower precedence>
    // relationships.
    MarkHigherThan({Highest}, {TermPrefix, LogicalPrefix});
    MarkHigherThan({TermPrefix},
                   {NumericPrefix, BitwisePrefix, IncrementDecrement});
    MarkHigherThan({NumericPrefix, BitwisePrefix},
                   {As, Multiplicative, Modulo, BitwiseAnd, BitwiseOr,
                    BitwiseXor, BitShift});
    MarkHigherThan({Multiplicative}, {Additive});
    MarkHigherThan(
        {As, Additive, Modulo, BitwiseAnd, BitwiseOr, BitwiseXor, BitShift},
        {Relational});
    MarkHigherThan({Relational, LogicalPrefix}, {LogicalAnd, LogicalOr});
    MarkHigherThan({LogicalAnd, LogicalOr}, {If});
    MarkHigherThan({If}, {Assignment});
    MarkHigherThan({Assignment, IncrementDecrement}, {Lowest});

    // Types are mostly a separate precedence graph.
    MarkHigherThan({Highest}, {TypePrefix});
    MarkHigherThan({TypePrefix}, {TypePostfix});
    MarkHigherThan({TypePostfix}, {As});

    // Compute the transitive closure of the above relationships: if we parse
    // `a $ b @ c` as `(a $ b) @ c` and parse `b @ c % d` as `(b @ c) % d`,
    // then we will parse `a $ b @ c % d` as `((a $ b) @ c) % d` and should
    // also parse `a $ bc % d` as `(a $ bc) % d`.
    MakeTransitivelyClosed();

    // Make the relation symmetric. If we parse `a $ b @ c` as `(a $ b) @ c`
    // then we want to parse `a @ b $ c` as `a @ (b $ c)`.
    MakeSymmetric();

    // Fill in the diagonal, which represents operator associativity.
    AddAssociativityRules();

    ConsistencyCheck();
  }

  // 用于标记一组操作符比另一组操作符具有更高的优先级。
  constexpr void MarkHigherThan(
      std::initializer_list<PrecedenceLevel> higher_group,
      std::initializer_list<PrecedenceLevel> lower_group) {
    for (auto higher : higher_group) {
      for (auto lower : lower_group) {
        table[higher][lower] = OperatorPriority::LeftFirst;
      }
    }
  }

  // 用于计算操作符优先级关系的传递闭包。
  // 例如，如果操作符 a 优先于 b，并且 b 优先于 c，那么 a 也应该优先于 c。
  constexpr void MakeTransitivelyClosed() {
    bool changed = false;
    do {
      changed = false;
      // NOLINTNEXTLINE(modernize-loop-convert)
      for (int8_t a = 0; a != NumPrecedenceLevels; ++a) {
        for (int8_t b = 0; b != NumPrecedenceLevels; ++b) {
          if (table[a][b] == OperatorPriority::LeftFirst) {
            for (int8_t c = 0; c != NumPrecedenceLevels; ++c) {
              if (table[b][c] == OperatorPriority::LeftFirst &&
                  table[a][c] != OperatorPriority::LeftFirst) {
                table[a][c] = OperatorPriority::LeftFirst;
                changed = true;
              }
            }
          }
        }
      }
    } while (changed);
  }

  // 用于使操作符优先级关系对称。
  // 例如，如果操作符 a 优先于 b，那么 b 应该在 a 之后。
  constexpr void MakeSymmetric() {
    for (int8_t a = 0; a != NumPrecedenceLevels; ++a) {
      for (int8_t b = 0; b != NumPrecedenceLevels; ++b) {
        if (table[a][b] == OperatorPriority::LeftFirst) {
          COCKTAIL_CHECK(table[b][a] != OperatorPriority::LeftFirst)
              << "inconsistent lookup table entries";
          table[b][a] = OperatorPriority::RightFirst;
        }
      }
    }
  }

  // 用于添加关于操作符结合性的规则。
  // 例如，加法是左结合的，所以 a + b + c 应该解析为 (a + b) + c。
  constexpr void AddAssociativityRules() {
    // Associativity rules occupy the diagonal

    // For prefix operators, RightFirst would mean `@@x` is `@(@x)` and
    // Ambiguous would mean it's an error. LeftFirst is meaningless.
    for (PrecedenceLevel prefix : {TermPrefix, If}) {
      table[prefix][prefix] = OperatorPriority::RightFirst;
    }

    // Postfix operators are symmetric with prefix operators.
    for (PrecedenceLevel postfix : {TypePostfix}) {
      table[postfix][postfix] = OperatorPriority::LeftFirst;
    }

    // Traditionally-associative operators are given left-to-right
    // associativity.
    for (PrecedenceLevel assoc :
         {Multiplicative, Additive, BitwiseAnd, BitwiseOr, BitwiseXor,
          LogicalAnd, LogicalOr}) {
      table[assoc][assoc] = OperatorPriority::LeftFirst;
    }

    // For other operators, we require explicit parentheses.
  }

  // 用于检查操作符优先级表的一致性。
  constexpr void ConsistencyCheck() {
    for (int8_t level = 0; level != NumPrecedenceLevels; ++level) {
      if (level != Highest) {
        COCKTAIL_CHECK(table[Highest][level] == OperatorPriority::LeftFirst &&
                       table[level][Highest] == OperatorPriority::RightFirst)
            << "Highest is not highest priority";
      }
      if (level != Lowest) {
        COCKTAIL_CHECK(table[Lowest][level] == OperatorPriority::RightFirst &&
                       table[level][Lowest] == OperatorPriority::LeftFirst)
            << "Lowest is not lowest priority";
      }
    }
  }

  OperatorPriority table[NumPrecedenceLevels][NumPrecedenceLevels];
};

}  // namespace

auto PrecedenceGroup::ForPostfixExpression() -> PrecedenceGroup {
  // 后缀表达式的优先级是最高的。
  return PrecedenceGroup(Highest);
}

auto PrecedenceGroup::ForTopLevelExpression() -> PrecedenceGroup {
  // 条件表达式（if condition then value1 else value2）被视为顶级表达式。
  return PrecedenceGroup(If);
}

auto PrecedenceGroup::ForExpressionStatement() -> PrecedenceGroup {
  // 这里返回的是最低的优先级，因为在大多数情况下，表达式语句不需要特定的优先级。
  return PrecedenceGroup(Lowest);
}

auto PrecedenceGroup::ForType() -> PrecedenceGroup {
  // 类型的优先级和顶级表达式的优先级是相同的。
  return ForTopLevelExpression();
}

auto PrecedenceGroup::ForLeading(Lex::TokenKind kind)
    -> std::optional<PrecedenceGroup> {
  switch (kind) {
    case Lex::TokenKind::Star:
    case Lex::TokenKind::Amp:
      return PrecedenceGroup(TermPrefix);

    case Lex::TokenKind::Not:
      return PrecedenceGroup(LogicalPrefix);

    case Lex::TokenKind::Minus:
      return PrecedenceGroup(NumericPrefix);

    case Lex::TokenKind::MinusMinus:
    case Lex::TokenKind::PlusPlus:
      return PrecedenceGroup(IncrementDecrement);

    case Lex::TokenKind::Caret:
      return PrecedenceGroup(BitwisePrefix);

    case Lex::TokenKind::If:
      return PrecedenceGroup(If);

    case Lex::TokenKind::Const:
      return PrecedenceGroup(TypePrefix);

    default:
      return std::nullopt;
  }
}

auto PrecedenceGroup::ForTrailing(Lex::TokenKind kind, bool infix)
    -> std::optional<Trailing> {
  switch (kind) {
    // Assignment operators.
    case Lex::TokenKind::Equal:
    case Lex::TokenKind::PlusEqual:
    case Lex::TokenKind::MinusEqual:
    case Lex::TokenKind::StarEqual:
    case Lex::TokenKind::SlashEqual:
    case Lex::TokenKind::PercentEqual:
    case Lex::TokenKind::AmpEqual:
    case Lex::TokenKind::PipeEqual:
    case Lex::TokenKind::CaretEqual:
    case Lex::TokenKind::GreaterGreaterEqual:
    case Lex::TokenKind::LessLessEqual:
      return Trailing{.level = Assignment, .is_binary = true};

    // Logical operators.
    case Lex::TokenKind::And:
      return Trailing{.level = LogicalAnd, .is_binary = true};
    case Lex::TokenKind::Or:
      return Trailing{.level = LogicalOr, .is_binary = true};

    // Bitwise operators.
    case Lex::TokenKind::Amp:
      return Trailing{.level = BitwiseAnd, .is_binary = true};
    case Lex::TokenKind::Pipe:
      return Trailing{.level = BitwiseOr, .is_binary = true};
    case Lex::TokenKind::Caret:
      return Trailing{.level = BitwiseXor, .is_binary = true};
    case Lex::TokenKind::GreaterGreater:
    case Lex::TokenKind::LessLess:
      return Trailing{.level = BitShift, .is_binary = true};

    // Relational operators.
    case Lex::TokenKind::EqualEqual:
    case Lex::TokenKind::ExclaimEqual:
    case Lex::TokenKind::Less:
    case Lex::TokenKind::LessEqual:
    case Lex::TokenKind::Greater:
    case Lex::TokenKind::GreaterEqual:
    case Lex::TokenKind::LessEqualGreater:
      return Trailing{.level = Relational, .is_binary = true};

    // Additive operators.
    case Lex::TokenKind::Plus:
    case Lex::TokenKind::Minus:
      return Trailing{.level = Additive, .is_binary = true};

    // Multiplicative operators.
    case Lex::TokenKind::Slash:
      return Trailing{.level = Multiplicative, .is_binary = true};
    case Lex::TokenKind::Percent:
      return Trailing{.level = Modulo, .is_binary = true};

    // `*` could be multiplication or pointer type formation.
    case Lex::TokenKind::Star:
      return infix ? Trailing{.level = Multiplicative, .is_binary = true}
                   : Trailing{.level = TypePostfix, .is_binary = false};

    // Cast operator.
    case Lex::TokenKind::As:
      return Trailing{.level = As, .is_binary = true};

    // Prefix-only operators.
    case Lex::TokenKind::Const:
    case Lex::TokenKind::MinusMinus:
    case Lex::TokenKind::Not:
    case Lex::TokenKind::PlusPlus:
      break;

    // Symbolic tokens that might be operators eventually.
    case Lex::TokenKind::Tilde:
    case Lex::TokenKind::Backslash:
    case Lex::TokenKind::Comma:
    case Lex::TokenKind::TildeEqual:
    case Lex::TokenKind::Exclaim:
    case Lex::TokenKind::LessGreater:
    case Lex::TokenKind::Question:
    case Lex::TokenKind::Colon:
      break;

    // Symbolic tokens that are intentionally not operators.
    case Lex::TokenKind::At:
    case Lex::TokenKind::LessMinus:
    case Lex::TokenKind::MinusGreater:
    case Lex::TokenKind::EqualGreater:
    case Lex::TokenKind::ColonEqual:
    case Lex::TokenKind::Period:
    case Lex::TokenKind::Semi:
      break;

    default:
      break;
  }

  return std::nullopt;
}

auto PrecedenceGroup::GetPriority(PrecedenceGroup left, PrecedenceGroup right)
    -> OperatorPriority {
  static constexpr OperatorPriorityTable Lookup;
  return Lookup.table[left.level_][right.level_];
}

}  // namespace Cocktail::Parse