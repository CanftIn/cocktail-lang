#include "Cocktail/LexNew/NumericLiteral.h"

#include <bitset>

#include "Cocktail/Common/CharacterSet.h"
#include "Cocktail/Common/Check.h"
#include "Cocktail/LexNew/LexHelpers.h"
#include "llvm/ADT/StringExtras.h"

namespace Cocktail::Lex {

// 使Radix与formatv一起使用。
static auto operator<<(llvm::raw_ostream& out, NumericLiteral::Radix radix)
    -> llvm::raw_ostream& {
  switch (radix) {
    case NumericLiteral::Radix::Binary:
      out << "binary";
      break;
    case NumericLiteral::Radix::Decimal:
      out << "decimal";
      break;
    case NumericLiteral::Radix::Hexadecimal:
      out << "hexadecimal";
      break;
  }
  return out;
}

// 贪婪词法分析，消耗尽可能多的字符，直到遇到一个不能是数值字面量一部分的字符为止。
auto NumericLiteral::Lex(llvm::StringRef source_text)
    -> std::optional<NumericLiteral> {
  NumericLiteral result;

  // 判断source_text是否为空以及第一个字符是否为数字。
  if (source_text.empty() || !IsDecimalDigit(source_text.front())) {
    return std::nullopt;
  }

  bool seen_plus_minus = false;
  bool seen_radix_point = false;
  bool seen_potential_exponent = false;

  // 由于之前已经确认过首字符，这里索引从1开始。
  int i = 1;
  for (int n = source_text.size(); i != n; ++i) {
    char c = source_text[i];
    if (IsAlnum(c) || c == '_') {
      // 只支持小写的 'e'，如果存在该字符且发现点号以及未探索
      // 到加减号则记录exponent索引位置，否则继续下一轮循环。
      if (IsLower(c) && seen_radix_point && !seen_plus_minus) {
        result.exponent_ = i;
        seen_potential_exponent = true;
      }
      continue;
    }

    // 当前字符为 '.' 时，记录radix_point。
    if (c == '.' && i + 1 != n && IsAlnum(source_text[i + 1]) &&
        !seen_radix_point) {
      result.radix_point_ = i;
      seen_radix_point = true;
      continue;
    }

    // 当前字符为 '+' 或 '-' 时，记录seen_plus_minus。
    if ((c == '+' || c == '-') && seen_potential_exponent &&
        result.exponent_ == i - 1 && i + 1 != n &&
        IsAlnum(source_text[i + 1])) {
      assert(!seen_plus_minus && "should only consume one + or -");
      seen_plus_minus = true;
      continue;
    }
    break;
  }

  // 返回探索到的字符串，以当前i的值为索引切分子串。
  result.text_ = source_text.substr(0, i);
  // 记录 '.' 偏移。
  if (!seen_radix_point) {
    result.radix_point_ = i;
  }
  // 记录 'e' 或 'p' 偏移。
  if (!seen_potential_exponent) {
    result.exponent_ = i;
  }

  return result;
}

// 解析数值字面量。
class NumericLiteral::Parser {
 public:
  Parser(DiagnosticEmitter<const char*>& emitter, NumericLiteral literal);

  // 表示该数值字面量是否是整数。
  // 通过检查小数点的位置是否在文本的末尾来确定。
  auto IsInteger() -> bool {
    return literal_.radix_point_ == static_cast<int>(literal_.text_.size());
  }

  // 检查数值字面量令牌在语法上是否有效和有意义，如果不是，则进行诊断。
  auto Check() -> bool;

  auto GetRadix() const -> Radix { return radix_; }

  auto GetMantissa() -> llvm::APInt;

  auto GetExponent() -> llvm::APInt;

 private:
  struct CheckDigitSequenceResult {
    bool ok;
    bool has_digit_separators = false;
  };

  // 检查给定的文本是否是一个有效的数字序列。
  auto CheckDigitSequence(llvm::StringRef text, Radix radix,
                          bool allow_digit_separators = true)
      -> CheckDigitSequenceResult;

  // 检查数字分隔符在文本中的位置是否正确。
  auto CheckDigitSeparatorPlacement(llvm::StringRef text, Radix radix,
                                    int num_digit_separators) -> void;
  // 检查数值字面量是否有前导零。
  auto CheckLeadingZero() -> bool;
  // 检查数值字面量的整数部分是否有效。
  auto CheckIntPart() -> bool;
  // 检查数值字面量的小数部分是否有效。
  auto CheckFractionalPart() -> bool;
  // 检查数值字面量的指数部分是否有效。
  auto CheckExponentPart() -> bool;

  DiagnosticEmitter<const char*>& emitter_;
  NumericLiteral literal_;

  // 基数默认为10，可以为 2 或 10 或 16，前缀分别为'0b'、无前缀、'0x'。
  Radix radix_ = Radix::Decimal;

  // 词法结构：[radix] int_part [. fract_part [[ep] [+-] exponent_part]]
  llvm::StringRef int_part_;       // 整数部分
  llvm::StringRef fract_part_;     // 小数部分
  llvm::StringRef exponent_part_;  // 指数部分

  // 对应数据是否需要清除`_`或`.`符号，默认为false。
  bool mantissa_needs_cleaning_ = false;
  bool exponent_needs_cleaning_ = false;

  // 是否在`exponent`部分后面发现了`-`符号。
  bool exponent_is_negative_ = false;
};

NumericLiteral::Parser::Parser(DiagnosticEmitter<const char*>& emitter,
                               NumericLiteral literal)
    : emitter_(emitter), literal_(literal) {
  int_part_ = literal.text_.substr(0, literal.radix_point_);
  if (int_part_.consume_front("0x")) {
    radix_ = Radix::Hexadecimal;
  } else if (int_part_.consume_front("0b")) {
    radix_ = Radix::Binary;
  }

  // radix_point_+1跳过小数点字符。
  fract_part_ = literal.text_.substr(
      literal.radix_point_ + 1, literal.exponent_ - literal.radix_point_ - 1);

  // exponent_+1跳过指数字符。
  exponent_part_ = literal.text_.substr(literal.exponent_ + 1);
  if (!exponent_part_.consume_front("+")) {
    exponent_is_negative_ = exponent_part_.consume_front("-");
  }
}

auto NumericLiteral::Parser::Check() -> bool {
  return CheckLeadingZero() && CheckIntPart() && CheckFractionalPart() &&
         CheckExponentPart();
}

// 将一个已知有小数的字符串解析为APInt，如果needs_cleaning为true，
// 在字符串包含 '_' 和 '.' 字符时，这些字符会被忽略。
// 当解析一个实数字面量时，会忽略 '.'。例如，当解析`123.456e7`时，
// 我们希望将其分解为一个整数尾数（123456）和一个指数（7 - 3 = 4），
// 并将 "123.456" 传递给此函数作为尾数进行解析。
static auto ParseInteger(llvm::StringRef digits, NumericLiteral::Radix radix,
                         bool needs_cleaning) -> llvm::APInt {
  llvm::SmallString<32> cleaned;
  if (needs_cleaning) {
    cleaned.reserve(digits.size());
    std::remove_copy_if(digits.begin(), digits.end(),
                        std::back_inserter(cleaned),
                        [](char c) { return c == '_' || c == '.'; });
    digits = cleaned;
  }

  llvm::APInt value;
  if (digits.getAsInteger(static_cast<int>(radix), value)) {
    llvm_unreachable("should never fail");
  }
  return value;
}

// 获取数值字面量的尾数。
auto NumericLiteral::Parser::GetMantissa() -> llvm::APInt {
  // 检查数值字面量是否是整数。如果是整数，end指向整数部分的结束，否则指向小数部分的结束。
  const char* end = IsInteger() ? int_part_.end() : fract_part_.end();
  llvm::StringRef digits(int_part_.begin(), end - int_part_.begin());
  return ParseInteger(digits, radix_, mantissa_needs_cleaning_);
}

// 获取数值字面量的指数。
auto NumericLiteral::Parser::GetExponent() -> llvm::APInt {
  llvm::APInt exponent(64, 0);
  if (!exponent_part_.empty()) {
    // 解析指数部分。
    exponent =
        ParseInteger(exponent_part_, Radix::Decimal, exponent_needs_cleaning_);

    // 确保解析得到的指数有足够的位宽来包含符号位，并确保它不会因溢出而丢失信息。
    if (exponent.isSignBitSet() || exponent.getBitWidth() < 64) {
      exponent = exponent.zext(std::max(64U, exponent.getBitWidth() + 1));
    }

    // 如果指数是负数，将其取反。
    if (exponent_is_negative_) {
      exponent.negate();
    }
  }

  // 计算小数部分中的字符数量，这会影响有效的指数值。
  // 例如，对于十六进制数，每个小数字符对应4个指数位。

  /// TODO: move to note.
  // 在浮点数表示中，指数部分表示数值的大小级别，而尾数部分表示数值的精确值。
  // 当我们有一个小数部分时，它实际上是在表示一个较小的值，
  // 因此我们需要调整指数来反映这一点。
  //
  // 为什么每个小数字符对应4个指数位呢？这与十六进制的表示有关。
  // 
  // 在十进制中，每当我们向右移动一个小数点位置，我们实际上是将数值除以10。
  // 例如，123.45 可以看作是 12345 × 10^-2。
  // 
  // 在二进制中，每当我们向右移动一个小数点位置，我们实际上是将数值除以2。
  // 例如，二进制的 101.1 可以看作是 1011 × 2^-1。
  // 
  // 在十六进制中，情况有些不同。十六进制是基数16的系统，但每个十六进制数字
  // 实际上表示4个二进制位。因此，每当我们在十六进制中向右移动一个小数点位置，
  // 我们实际上是将数值除以 2^4，即16。这就是为什么每个小数字符对应4个指数位的原因。
  // 
  // 具体数据的例子：
  // 
  // 考虑十六进制数 A.B。这可以转换为二进制形式 1010.1011。
  // 
  // 在十六进制中，A.B 可以表示为 AB × 16^-1。
  // 
  // 在二进制中，1010.1011 可以表示为 10101011 × 2^-4。
  // 
  // 十六进制的指数是 -1，而二进制的指数是 -4
  int excess_exponent = fract_part_.size();
  if (radix_ == Radix::Hexadecimal) {
    excess_exponent *= 4;
  }
  exponent -= excess_exponent;
  // 如果指数是负数并且其值不是负数（即它从负数溢出到正数），
  // 则扩展指数的位宽并设置其符号位。
  if (exponent_is_negative_ && !exponent.isNegative()) {
    exponent = exponent.zext(exponent.getBitWidth() + 1);
    exponent.setSignBit();
  }
  return exponent;
}

// 检查一个数字序列是否有效。
// 涉及到几个方面的检查，包括是否所有的数字都是在指定的基数范围内，
// 是否允许使用数字分隔符，以及数字分隔符是否正确放置。
auto NumericLiteral::Parser::CheckDigitSequence(llvm::StringRef text,
                                                Radix radix,
                                                bool allow_digit_separators)
    -> CheckDigitSequenceResult {
  // 使用 std::bitset 来标记有效的数字字符。
  std::bitset<256> valid_digits;
  switch (radix) {
    case Radix::Binary:
      for (char c : "01") {
        valid_digits[static_cast<unsigned char>(c)] = true;
      }
      break;
    case Radix::Decimal:
      for (char c : "0123456789") {
        valid_digits[static_cast<unsigned char>(c)] = true;
      }
      break;
    case Radix::Hexadecimal:
      for (char c : "0123456789ABCDEF") {
        valid_digits[static_cast<unsigned char>(c)] = true;
      }
      break;
  }

  int num_digit_separators = 0;

  // 遍历 text 中的每个字符，并检查它是否是一个有效的数字或数字分隔符。
  for (int i = 0, n = text.size(); i != n; ++i) {
    char c = text[i];
    // 如果字符是一个有效的数字，它会继续下一个字符。
    if (valid_digits[static_cast<unsigned char>(c)]) {
      continue;
    }

    // 如果字符是一个数字分隔符 _，它会检查该分隔符是否
    // 正确放置（不在序列的开始或结束，不在另一个分隔符旁边）。
    if (c == '_') {
      if (!allow_digit_separators || i == 0 || text[i - 1] == '_' ||
          i + 1 == n) {
        COCKTAIL_DIAGNOSTIC(InvalidDigitSeparator, Error,
                            "Misplaced digit separator in numeric literal.");
        emitter_.Emit(text.begin() + 1, InvalidDigitSeparator);
      }
      ++num_digit_separators;
      continue;
    }

    // 既不是有效的数字也不是数字分隔符。
    COCKTAIL_DIAGNOSTIC(InvalidDigit, Error,
                        "Invalid digit '{0}' in {1} numeric literal.", char,
                        NumericLiteral::Radix);
    emitter_.Emit(text.begin() + i, InvalidDigit, c, radix);
    return {.ok = false};
  }

  if (num_digit_separators == static_cast<int>(text.size())) {
    COCKTAIL_DIAGNOSTIC(EmptyDigitSequence, Error,
                        "Empty digit sequence in numeric literal.");
    emitter_.Emit(text.begin(), EmptyDigitSequence);
    return {.ok = false};
  }

  // 如果文本中有数字分隔符，这个函数会被调用来确保它们正确放置。
  if (num_digit_separators) {
    CheckDigitSeparatorPlacement(text, radix, num_digit_separators);
  }

  // 检查给定的文本是否可以被词法分析为一个整数。
  if (!CanLexInteger(emitter_, text)) {
    return {.ok = false};
  }

  return {.ok = true, .has_digit_separators = (num_digit_separators != 0)};
}

// 检查数字序列中的数字分隔符 _ 是否正确放置。
// 对于二进制数字，没有特定的放置要求。
// 但对于十进制，分隔符必须每3个数字之间放置一个。
// 十六进制数字必须每4个数字之间放置一个。
auto NumericLiteral::Parser::CheckDigitSeparatorPlacement(
    llvm::StringRef text, Radix radix, int num_digit_separators) -> void {
  // 使用一个断言来确保文本中的数字分隔符数量与传入的num_digit_separators参数匹配。
  COCKTAIL_CHECK(std::count(text.begin(), text.end(), '_') ==
                 num_digit_separators)
      << "given wrong number of digit separators: " << num_digit_separators;

  // 如果基数是二进制，函数直接返回，因为二进制数字没有数字分隔符的放置要求。
  if (radix == Radix::Binary) {
    return;
  }

  // 用于发出一个错误诊断，表示数字分隔符的位置不正确。
  auto diagnose_irregular_digit_separators = [&]() {
    COCKTAIL_DIAGNOSTIC(
        IrregularDigitSeparators, Error,
        "Digit separators in {0} number should appear every {1} characters "
        "from the right.",
        NumericLiteral::Radix, int);
    emitter_.Emit(text.begin(), IrregularDigitSeparators, radix,
                  radix == Radix::Decimal ? 3 : 4);
  };

  // 对于十进制数字，每3个数字之间有一个分隔符，所以步长是4。
  // 对于十六进制数字，每4个数字之间有一个分隔符，所以步长是5。
  int stride = (radix == Radix::Decimal ? 4 : 5);
  // 检查数字分隔符的位置。
  // 从文本的末尾开始，每次向前移动一个步长，并检查是否有一个数字分隔符。
  // 如果在预期的位置没有找到分隔符，或者找到了其他字符，则发出一个错误诊断。
  int remaining_digit_separators = num_digit_separators;
  const auto* pos = text.end();
  while (pos - text.begin() >= stride) {
    pos -= stride;
    if (*pos != '_') {
      diagnose_irregular_digit_separators();
      return;
    }

    --remaining_digit_separators;
  }

  // 循环结束后仍有未检查的数字分隔符，意味着它们的位置不正确，
  // 所以发出一个错误诊断。
  if (remaining_digit_separators) {
    diagnose_irregular_digit_separators();
  }
}

// 检查非零十进制整数上没有'0'前缀。
auto NumericLiteral::Parser::CheckLeadingZero() -> bool {
  if (radix_ == Radix::Decimal && int_part_.startswith("0") &&
      int_part_ != "0") {
    COCKTAIL_DIAGNOSTIC(UnknownBaseSpecifier, Error,
                        "Unknown base specifier in numeric literal.");
    emitter_.Emit(int_part_.begin(), UnknownBaseSpecifier);
    return false;
  }
  return true;
}

// 检查数字字面量的整数部分（即小数点前的部分，如果有的话）是否有效。
auto NumericLiteral::Parser::CheckIntPart() -> bool {
  auto int_result = CheckDigitSequence(int_part_, radix_);
  // 如果整数部分包含数字分隔符，那么mantissa_needs_cleaning_将被设置为true，
  // 表示在解析数字的尾数时需要清除这些分隔符。
  mantissa_needs_cleaning_ |= int_result.has_digit_separators;
  return int_result.ok;
}

// 函数检查数字字面量的小数部分（即小数点后的部分，如果有的话）是否有效。
auto NumericLiteral::Parser::CheckFractionalPart() -> bool {
  // 数字字面量是一个整数（没有小数部分），那么函数直接返回true。
  if (IsInteger()) {
    return true;
  }

  // 如果基数是二进制，那么发出一个错误诊断，表示不支持二进制实数字面量。
  if (radix_ == Radix::Binary) {
    COCKTAIL_DIAGNOSTIC(BinaryRealLiteral, Error,
                        "Binary real number literals are not supported.");
    emitter_.Emit(literal_.text_.begin() + literal_.radix_point_,
                  BinaryRealLiteral);
  }

  // 由于小数部分包含一个小数点，所以在解析数字的尾数时需要清除这个小数点。
  mantissa_needs_cleaning_ = true;

  // 检查小数部分是否包含有效的数字，并返回结果。
  return CheckDigitSequence(fract_part_, radix_,
                            /*allow_digit_separators=*/false)
      .ok;
}

// 检查数字字面量的指数部分（如果有的话）是否有效。
auto NumericLiteral::Parser::CheckExponentPart() -> bool {
  // 如果数字字面量没有指数部分，那么函数直接返回true。
  if (literal_.exponent_ == static_cast<int>(literal_.text_.size())) {
    return true;
  }

  // 根据基数确定期望的指数字符。对于十进制数字，期望的字符是 e；
  // 对于二进制或十六进制数字，期望的字符是 p。
  char expected_exponent_kind = (radix_ == Radix::Decimal ? 'e' : 'p');
  if (literal_.text_[literal_.exponent_] != expected_exponent_kind) {
    COCKTAIL_DIAGNOSTIC(WrongRealLiteralExponent, Error,
                        "Expected '{0}' to introduce exponent.", char);
    emitter_.Emit(literal_.text_.begin() + literal_.exponent_,
                  WrongRealLiteralExponent, expected_exponent_kind);
    return false;
  }

  auto exponent_result = CheckDigitSequence(exponent_part_, Radix::Decimal);
  exponent_needs_cleaning_ = exponent_result.has_digit_separators;
  return exponent_result.ok;
}

// 解析token并计算值。
auto NumericLiteral::ComputeValue(DiagnosticEmitter<const char*>& emitter) const
    -> Value {
  Parser parser(emitter, *this);

  if (!parser.Check()) {
    return UnrecoverableError();
  }

  if (parser.IsInteger()) {
    return IntegerValue{.value = parser.GetMantissa()};
  }

  return RealValue{
      .radix = (parser.GetRadix() == Radix::Decimal ? Radix::Decimal
                                                    : Radix::Binary),
      .mantissa = parser.GetMantissa(),
      .exponent = parser.GetExponent()};
}

}  // namespace Cocktail::Lex