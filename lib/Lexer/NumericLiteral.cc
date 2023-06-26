#include "Cocktail/Lexer/NumericLiteral.h"

#include <bitset>

#include "Cocktail/Common/CharacterSet.h"
#include "Cocktail/Common/Check.h"
#include "Cocktail/Lexer/LexHelpers.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/FormatVariadic.h"

namespace Cocktail {

static auto operator<<(llvm::raw_ostream& out, LexedNumericLiteral::Radix radix)
    -> llvm::raw_ostream& {
  switch (radix) {
    case LexedNumericLiteral::Radix::Binary:
      out << "binary";
      break;
    case LexedNumericLiteral::Radix::Decimal:
      out << "decimal";
      break;
    case LexedNumericLiteral::Radix::Hexadecimal:
      out << "hexadecimal";
      break;
  }
  return out;
}

auto LexedNumericLiteral::Lex(llvm::StringRef source_text)
    -> llvm::Optional<LexedNumericLiteral> {
  LexedNumericLiteral result;

  if (source_text.empty() || !IsDecimalDigit(source_text.front())) {
    return llvm::None;
  }

  bool seen_plus_minus = false;
  bool seen_radix_point = false;
  bool seen_potential_exponent = false;

  int i = 1;
  for (int n = source_text.size(); i != n; ++i) {
    char c = source_text[i];
    if (IsAlnum(c) || c == '_') {
      if (IsLower(c) && seen_radix_point && !seen_plus_minus) {
        result.exponent_ = i;
        seen_potential_exponent = true;
      }
      continue;
    }

    if (c == '.' && i + 1 != n && IsAlnum(source_text[i + 1]) &&
        !seen_radix_point) {
      result.radix_point_ = i;
      seen_radix_point = true;
      continue;
    }

    if ((c == '+' || c == '-') && seen_potential_exponent &&
        result.exponent_ == i - 1 && i + 1 != n &&
        IsAlnum(source_text[i + 1])) {
      assert(!seen_plus_minus && "should only consume one + or -");
      seen_plus_minus = true;
      continue;
    }
    break;
  }

  result.text_ = source_text.substr(0, i);
  if (!seen_radix_point) {
    result.radix_point_ = i;
  }
  if (!seen_potential_exponent) {
    result.exponent_ = i;
  }

  return result;
}

class LexedNumericLiteral::Parser {
 public:
  Parser(DiagnosticEmitter<const char*>& emitter, LexedNumericLiteral literal);

  auto IsInteger() -> bool {
    return literal_.radix_point_ == static_cast<int>(literal_.text_.size());
  }

  auto Check() -> bool;

  auto GetRadix() const -> Radix { return radix_; }

  auto GetMantissa() -> llvm::APInt;

  auto GetExponent() -> llvm::APInt;

 private:
  struct CheckDigitSequenceResult {
    bool ok;
    bool has_digit_separators = false;
  };

  auto CheckDigitSequence(llvm::StringRef text, Radix radix,
                          bool allow_digit_separators = true)
      -> CheckDigitSequenceResult;

  auto CheckDigitSeparatorPlacement(llvm::StringRef text, Radix radix,
                                    int num_digit_separators) -> void;
  auto CheckLeadingZero() -> bool;
  auto CheckIntPart() -> bool;
  auto CheckFractionalPart() -> bool;
  auto CheckExponentPart() -> bool;

  DiagnosticEmitter<const char*>& emitter_;
  LexedNumericLiteral literal_;

  // The radix of the literal: 2, 10, or 16
  Radix radix_ = Radix::Decimal;

  // [radix] int_part [. fract_part [[ep] [+-] exponent_part]]
  llvm::StringRef int_part_;
  llvm::StringRef fract_part_;
  llvm::StringRef exponent_part_;

  bool mantissa_needs_cleaning_ = false;
  bool exponent_needs_cleaning_ = false;

  // True if we found a `-` before `exponent_part`.
  bool exponent_is_negative_ = false;
};

LexedNumericLiteral::Parser::Parser(DiagnosticEmitter<const char*>& emitter,
                                    LexedNumericLiteral literal)
    : emitter_(emitter), literal_(literal) {
  int_part_ = literal.text_.substr(0, literal.radix_point_);
  if (int_part_.consume_front("0x")) {
    radix_ = Radix::Hexadecimal;
  } else if (int_part_.consume_front("0b")) {
    radix_ = Radix::Binary;
  }

  fract_part_ = literal.text_.substr(
      literal.radix_point_ + 1, literal.exponent_ - literal.radix_point_ - 1);

  exponent_part_ = literal.text_.substr(literal.exponent_ + 1);
  if (!exponent_part_.consume_front("+")) {
    exponent_is_negative_ = exponent_part_.consume_front("-");
  }
}

auto LexedNumericLiteral::Parser::Check() -> bool {
  return CheckLeadingZero() && CheckIntPart() && CheckFractionalPart() &&
         CheckExponentPart();
}

static auto ParseInteger(llvm::StringRef digits,
                         LexedNumericLiteral::Radix radix, bool needs_cleaning)
    -> llvm::APInt {
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

auto LexedNumericLiteral::Parser::GetMantissa() -> llvm::APInt {
  const char* end = IsInteger() ? int_part_.end() : fract_part_.end();
  llvm::StringRef digits(int_part_.begin(), end - int_part_.begin());
  return ParseInteger(digits, radix_, mantissa_needs_cleaning_);
}

auto LexedNumericLiteral::Parser::GetExponent() -> llvm::APInt {
  llvm::APInt exponent(64, 0);
  if (!exponent_part_.empty()) {
    exponent =
        ParseInteger(exponent_part_, Radix::Decimal, exponent_needs_cleaning_);

    if (exponent.isSignBitSet() || exponent.getBitWidth() < 64) {
      exponent = exponent.zext(std::max(64U, exponent.getBitWidth() + 1));
    }

    if (exponent_is_negative_) {
      exponent.negate();
    }
  }

  int excess_exponent = fract_part_.size();
  if (radix_ == Radix::Hexadecimal) {
    excess_exponent *= 4;
  }
  exponent -= excess_exponent;
  if (exponent_is_negative_ && !exponent.isNegative()) {
    exponent = exponent.zext(exponent.getBitWidth() + 1);
    exponent.setSignBit();
  }
  return exponent;
}

auto LexedNumericLiteral::Parser::CheckDigitSequence(
    llvm::StringRef text, Radix radix, bool allow_digit_separators)
    -> CheckDigitSequenceResult {
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

  for (int i = 0, n = text.size(); i != n; ++i) {
    char c = text[i];
    if (valid_digits[static_cast<unsigned char>(c)]) {
      continue;
    }

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

    COCKTAIL_DIAGNOSTIC(InvalidDigit, Error,
                        "Invalid digit '{0}' in {1} numeric literal.", char,
                        LexedNumericLiteral::Radix);
    emitter_.Emit(text.begin() + i, InvalidDigit, c, radix);
    return {.ok = false};
  }

  if (num_digit_separators == static_cast<int>(text.size())) {
    COCKTAIL_DIAGNOSTIC(EmptyDigitSequence, Error,
                        "Empty digit sequence in numeric literal.");
    emitter_.Emit(text.begin(), EmptyDigitSequence);
    return {.ok = false};
  }

  if (num_digit_separators) {
    CheckDigitSeparatorPlacement(text, radix, num_digit_separators);
  }

  if (!CanLexInteger(emitter_, text)) {
    return {.ok = false};
  }

  return {.ok = true, .has_digit_separators = (num_digit_separators != 0)};
}

auto LexedNumericLiteral::Parser::CheckDigitSeparatorPlacement(
    llvm::StringRef text, Radix radix, int num_digit_separators) -> void {
  COCKTAIL_CHECK(std::count(text.begin(), text.end(), '_') ==
                 num_digit_separators)
      << "given wrong number of digit separators: " << num_digit_separators;

  if (radix == Radix::Binary) {
    return;
  }

  auto diagnose_irregular_digit_separators = [&]() {
    COCKTAIL_DIAGNOSTIC(
        IrregularDigitSeparators, Error,
        "Digit separators in {0} number should appear every {1} characters "
        "from the right.",
        LexedNumericLiteral::Radix, int);
    emitter_.Emit(text.begin(), IrregularDigitSeparators, radix,
                  radix == Radix::Decimal ? 3 : 4);
  };

  int stride = (radix == Radix::Decimal ? 4 : 5);
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

  if (remaining_digit_separators) {
    diagnose_irregular_digit_separators();
  }
}

auto LexedNumericLiteral::Parser::CheckLeadingZero() -> bool {
  if (radix_ == Radix::Decimal && int_part_.startswith("0") &&
      int_part_ != "0") {
    COCKTAIL_DIAGNOSTIC(UnknownBaseSpecifier, Error,
                        "Unknown base specifier in numeric literal.");
    emitter_.Emit(int_part_.begin(), UnknownBaseSpecifier);
    return false;
  }
  return true;
}

auto LexedNumericLiteral::Parser::CheckIntPart() -> bool {
  auto int_result = CheckDigitSequence(int_part_, radix_);
  mantissa_needs_cleaning_ |= int_result.has_digit_separators;
  return int_result.ok;
}

auto LexedNumericLiteral::Parser::CheckFractionalPart() -> bool {
  if (IsInteger()) {
    return true;
  }

  if (radix_ == Radix::Binary) {
    COCKTAIL_DIAGNOSTIC(BinaryRealLiteral, Error,
                        "Binary real number literals are not supported.");
    emitter_.Emit(literal_.text_.begin() + literal_.radix_point_,
                  BinaryRealLiteral);
  }

  // We need to remove a '.' from the mantissa.
  mantissa_needs_cleaning_ = true;

  return CheckDigitSequence(fract_part_, radix_,
                            /*allow_digit_separators=*/false)
      .ok;
}

auto LexedNumericLiteral::Parser::CheckExponentPart() -> bool {
  if (literal_.exponent_ == static_cast<int>(literal_.text_.size())) {
    return true;
  }

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

// Parse the token and compute its value.
auto LexedNumericLiteral::ComputeValue(
    DiagnosticEmitter<const char*>& emitter) const -> Value {
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

}  // namespace Cocktail