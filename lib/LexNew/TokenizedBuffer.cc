#include "Cocktail/LexNew/TokenizedBuffer.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "Cocktail/Common/CharacterSet.h"
#include "Cocktail/Common/Check.h"
#include "Cocktail/Common/StringHelpers.h"
#include "Cocktail/LexNew/LexHelpers.h"
#include "Cocktail/LexNew/NumericLiteral.h"
#include "Cocktail/LexNew/StringLiteral.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#if __x86_64__
#include <x86intrin.h>
#endif

namespace Cocktail::Lex {

// 允许将多个函数对象组合成一个重载集。
// 例如：
// auto overloaded = Overload{
//     [] (int) { return "int"; }, [] (float) { return "float"; }};
// std::cout << overloaded(42) << std::endl;    // 输出 "int"
// std::cout << overloaded(42.0f) << std::endl; // 输出 "float"
template <typename... Fs>
struct Overload : Fs... {
  using Fs::operator()...;
};
template <typename... Fs>
Overload(Fs...) -> Overload<Fs...>;

// 允许对 std::variant 中存储的值进行模式匹配。
// 例如：
// std::variant<int, float, std::string> var = 42;
// VariantMatch(var,
//     [] (int i) { std::cout << "It's an int: " << i << std::endl; },
//     [] (float f) { std::cout << "It's a float: " << f << std::endl; },
//     [] (const std::string& s) {
//       std::cout << "It's a string: " << s << std::endl; }
// );
template <typename V, typename... Fs>
auto VariantMatch(V&& v, Fs&&... fs) -> decltype(auto) {
  return std::visit(Overload{std::forward<Fs>(fs)...}, std::forward<V>(v));
}

// 从给定的文本中扫描并返回一个连续的标识符前缀。
// 当前的实现只考虑 ASCII 字符 [0-9A-Za-z_] 作为有效的标识符字符。
// 这是一个性能敏感的函数，因此使用了向量化的代码序列来优化其扫描过程。
static auto ScanForIdentifierPrefix(llvm::StringRef text) -> llvm::StringRef {
  // 这是一个静态常量数组，用于分类字节是否为有效的标识符字符。
  // 这在非向量化的通用回退代码中用于扫描标识符的长度。
  static constexpr std::array<bool, 256> IsIdByteTable = ([]() constexpr {
    std::array<bool, 256> table = {};
    for (char c = '0'; c <= '9'; ++c) {
      table[c] = true;
    }
    for (char c = 'A'; c <= 'Z'; ++c) {
      table[c] = true;
    }
    for (char c = 'a'; c <= 'z'; ++c) {
      table[c] = true;
    }
    table['_'] = true;
    return table;
  })();

  // 使用 SIMD (单指令多数据) 指令集来加速标识符的扫描过程。
#if __x86_64__
  // This code uses a scheme derived from the techniques in Geoff Langdale and
  // Daniel Lemire's work on parsing JSON[1]. Specifically, that paper outlines
  // a technique of using two 4-bit indexed in-register look-up tables (LUTs) to
  // classify bytes in a branchless SIMD code sequence.
  //
  // [1]: https://arxiv.org/pdf/1902.08318.pdf
  //
  // The goal is to get a bit mask classifying different sets of bytes. For each
  // input byte, we first test for a high bit indicating a UTF-8 encoded Unicode
  // character. Otherwise, we want the mask bits to be set with the following
  // logic derived by inspecting the high nibble and low nibble of the input:
  // bit0 = 1 for `_`: high `0x5` and low `0xF`
  // bit1 = 1 for `0-9`: high `0x3` and low `0x0` - `0x9`
  // bit2 = 1 for `A-O` and `a-o`: high `0x4` or `0x6` and low `0x1` - `0xF`
  // bit3 = 1 for `P-Z` and 'p-z': high `0x5` or `0x7` and low `0x0` - `0xA`
  // bit4 = unused
  // bit5 = unused
  // bit6 = unused
  // bit7 = unused
  //
  // No bits set means definitively non-ID ASCII character.
  //
  // bits 4-7 remain unused if we need to classify more characters.
  const auto high_lut = _mm_setr_epi8(
      /* __b0=*/0b0000'0000,
      /* __b1=*/0b0000'0000,
      /* __b2=*/0b0000'0000,
      /* __b3=*/0b0000'0010,
      /* __b4=*/0b0000'0100,
      /* __b5=*/0b0000'1001,
      /* __b6=*/0b0000'0100,
      /* __b7=*/0b0000'1000,
      /* __b8=*/0b0000'0000,
      /* __b9=*/0b0000'0000,
      /*__b10=*/0b0000'0000,
      /*__b11=*/0b0000'0000,
      /*__b12=*/0b0000'0000,
      /*__b13=*/0b0000'0000,
      /*__b14=*/0b0000'0000,
      /*__b15=*/0b0000'0000);
  const auto low_lut = _mm_setr_epi8(
      /* __b0=*/0b0000'1010,
      /* __b1=*/0b0000'1110,
      /* __b2=*/0b0000'1110,
      /* __b3=*/0b0000'1110,
      /* __b4=*/0b0000'1110,
      /* __b5=*/0b0000'1110,
      /* __b6=*/0b0000'1110,
      /* __b7=*/0b0000'1110,
      /* __b8=*/0b0000'1110,
      /* __b9=*/0b0000'1110,
      /*__b10=*/0b0000'1100,
      /*__b11=*/0b0000'0100,
      /*__b12=*/0b0000'0100,
      /*__b13=*/0b0000'0100,
      /*__b14=*/0b0000'0100,
      /*__b15=*/0b0000'0101);

  // Use `ssize_t` for performance here as we index memory in a tight loop.
  ssize_t i = 0;
  const ssize_t size = text.size();
  while ((i + 16) <= size) {
    __m128i input =
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(text.data() + i));

    // The high bits of each byte indicate a non-ASCII character encoded using
    // UTF-8. Test those and fall back to the scalar code if present. These
    // bytes will also cause spurious zeros in the LUT results, but we can
    // ignore that because we track them independently here.
#if __SSE4_1__
    if (!_mm_test_all_zeros(_mm_set1_epi8(0x80), input)) {
      break;
    }
#else
    if (_mm_movemask_epi8(input) != 0) {
      break;
    }
#endif

    // Do two LUT lookups and mask the results together to get the results for
    // both low and high nibbles. Note that we don't need to mask out the high
    // bit of input here because we track that above for UTF-8 handling.
    __m128i low_mask = _mm_shuffle_epi8(low_lut, input);
    // Note that the input needs to be masked to only include the high nibble or
    // we could end up with bit7 set forcing the result to a zero byte.
    __m128i input_high =
        _mm_and_si128(_mm_srli_epi32(input, 4), _mm_set1_epi8(0x0f));
    __m128i high_mask = _mm_shuffle_epi8(high_lut, input_high);
    __m128i mask = _mm_and_si128(low_mask, high_mask);

    // Now compare to find the completely zero bytes.
    __m128i id_byte_mask_vec = _mm_cmpeq_epi8(mask, _mm_setzero_si128());
    int tail_ascii_mask = _mm_movemask_epi8(id_byte_mask_vec);

    // Check if there are bits in the tail mask, which means zero bytes and the
    // end of the identifier. We could do this without materializing the scalar
    // mask on more recent CPUs, but we generally expect the median length we
    // encounter to be <16 characters and so we avoid the extra instruction in
    // that case and predict this branch to succeed so it is laid out in a
    // reasonable way.
    if (LLVM_LIKELY(tail_ascii_mask != 0)) {
      // Move past the definitively classified bytes that are part of the
      // identifier, and return the complete identifier text.
      i += __builtin_ctz(tail_ascii_mask);
      return text.substr(0, i);
    }
    i += 16;
  }

  // Fallback to scalar loop. We only end up here when we don't have >=16
  // bytes to scan or we find a UTF-8 unicode character.
  // TODO: This assumes all Unicode characters are non-identifiers.
  while (i < size && IsIdByteTable[static_cast<unsigned char>(text[i])]) {
    ++i;
  }

  return text.substr(0, i);
#else
  // TODO: Optimize this with SIMD for other architectures.
  return text.take_while(
      [](char c) { return IsIdByteTable[static_cast<unsigned char>(c)]; });
#endif
}

// 表示词法分析器的实现。
// 循环遍历源缓冲区，通过调用此类API将其转化为Token。
class TokenizedBuffer::Lexer {
 public:
  // 表示词法分析操作的结果。
  // 代表是否成功地词法分析了一个Token，或者是否应尝试其他词法分析操作。
  class LexResult {
   public:
    // Token到LexResult是隐式转化。
    LexResult(Token /*unused*/) : LexResult(true) {}
    // 返回一个表示没有生成Token的结果。
    static auto NoMatch() -> LexResult { return LexResult(false); }
    // 测试词法分析例程是否生成了一个Token，并且词法分析器是否可以继续生成Token。
    explicit operator bool() const { return formed_token_; }

   private:
    explicit LexResult(bool formed_token) : formed_token_(formed_token) {}

    bool formed_token_;
  };

  using DispatchFunctionT = auto(Lexer& lexer, llvm::StringRef& source_text)
      -> LexResult;
  using DispatchTableT = std::array<DispatchFunctionT*, 256>;

  Lexer(TokenizedBuffer& buffer, DiagnosticConsumer& consumer)
      : buffer_(&buffer),
        translator_(&buffer),
        emitter_(translator_, consumer),
        token_translator_(&buffer),
        token_emitter_(token_translator_, consumer),
        current_line_(buffer.AddLine(LineInfo(0))),
        current_line_info_(&buffer.GetLineInfo(current_line_)) {}

  // 处理源代码中的新行字符。
  auto HandleNewline() -> void {
    // 设置当前行的长度为当前列的值。
    current_line_info_->length = current_column_;
    // 添加一个新行
    current_line_ = buffer_->AddLine(
        LineInfo(current_line_info_->start + current_column_ + 1));
    current_line_info_ = &buffer_->GetLineInfo(current_line_);
    current_column_ = 0;  // 重置列计数器
    set_indent_ = false;  // 重置缩进标志
  }

  // 标记上一个Token后面有空白字符。
  auto NoteWhitespace() -> void {
    if (!buffer_->token_infos_.empty()) {
      buffer_->token_infos_.back().has_trailing_space = true;
    }
  }

  // 跳过源代码中的空白字符和注释。
  auto SkipWhitespace(llvm::StringRef& source_text) -> bool {
    const char* const whitespace_start = source_text.begin();

    while (!source_text.empty()) {
      if (source_text.startswith("//")) {
        // 注释必须是其所在行上唯一的非空白内容。
        if (set_indent_) {
          COCKTAIL_DIAGNOSTIC(TrailingComment, Error,
                              "Trailing comments are not permitted.");

          emitter_.Emit(source_text.begin(), TrailingComment);
        }
        // 处理注释，确保它们后面有空白字符。
        if (source_text.size() > 2 && !IsSpace(source_text[2])) {
          COCKTAIL_DIAGNOSTIC(NoWhitespaceAfterCommentIntroducer, Error,
                              "Whitespace is required after '//'.");
          emitter_.Emit(source_text.begin() + 2,
                        NoWhitespaceAfterCommentIntroducer);
        }
        // 跳过单行注释的内容，并正确地更新列的计数，直到遇到行尾或文件尾。
        while (!source_text.empty() && source_text.front() != '\n') {
          ++current_column_;
          source_text = source_text.drop_front();
        }
        if (source_text.empty()) {
          break;
        }
      }

      switch (source_text.front()) {
        default:
          // 对于非空白字符，函数检查它是否确实是非空白字符，
          // 然后记录有空白字符，并返回true。
          COCKTAIL_CHECK(!IsSpace(source_text.front()));
          if (whitespace_start != source_text.begin()) {
            NoteWhitespace();
          }
          return true;

        case '\n':  // 处理新行，更新行信息，并继续循环。
          source_text = source_text.drop_front();
          if (source_text.empty()) {
            current_line_info_->length = current_column_;
            return false;
          }
          HandleNewline();
          continue;

        case ' ':
        case '\t':  // 增加列计数器并跳过这些字符。
          ++current_column_;
          source_text = source_text.drop_front();
          continue;
      }
    }

    COCKTAIL_CHECK(source_text.empty())
        << "Cannot reach here without finishing the text!";
    current_line_info_->length = current_column_;
    return false;
  }

  // 词法分析数值字面量。
  auto LexNumericLiteral(llvm::StringRef& source_text) -> LexResult {
    std::optional<NumericLiteral> literal = NumericLiteral::Lex(source_text);
    if (!literal) {
      return LexError(source_text);
    }
    // 更新列计数和源文本。
    int int_column = current_column_;  // 当前Token开始的列位置。
    int token_size = literal->text().size();
    current_column_ += token_size;
    source_text = source_text.drop_front(token_size);
    // 设置缩进。
    if (!set_indent_) {
      current_line_info_->indent = int_column;
      set_indent_ = true;
    }
    // 处理数值字面量的值。
    return VariantMatch(
        literal->ComputeValue(emitter_),
        [&](NumericLiteral::IntegerValue&& value) {  // 整数值处理。
          auto token = buffer_->AddToken({.kind = TokenKind::IntegerLiteral,
                                          .token_line = current_line_,
                                          .column = int_column});
          buffer_->GetTokenInfo(token).literal_index =
              buffer_->literal_int_storage_.size();
          buffer_->literal_int_storage_.push_back(std::move(value.value));
          return token;
        },
        [&](NumericLiteral::RealValue&& value) {  // 实数值处理。
          auto token = buffer_->AddToken({.kind = TokenKind::RealLiteral,
                                          .token_line = current_line_,
                                          .column = int_column});
          buffer_->GetTokenInfo(token).literal_index =
              buffer_->literal_int_storage_.size();
          buffer_->literal_int_storage_.push_back(std::move(value.mantissa));
          buffer_->literal_int_storage_.push_back(std::move(value.exponent));
          COCKTAIL_CHECK(buffer_->GetRealLiteral(token).is_decimal ==
                         (value.radix == NumericLiteral::Radix::Decimal));
          return token;
        },
        [&](NumericLiteral::UnrecoverableError) {  // 错误处理。
          auto token = buffer_->AddToken({
              .kind = TokenKind::Error,
              .token_line = current_line_,
              .column = int_column,
              .error_length = token_size,
          });
          return token;
        });
  }

  // 词法分析字符串字面量。
  auto LexStringLiteral(llvm::StringRef& source_text) -> LexResult {
    std::optional<StringLiteral> literal = StringLiteral::Lex(source_text);
    if (!literal) {
      return LexError(source_text);
    }

    // 设置字符串的位置信息。
    Line string_line = current_line_;
    int string_column = current_column_;
    int literal_size = literal->text().size();
    source_text = source_text.drop_front(literal_size);

    if (!set_indent_) {
      current_line_info_->indent = string_column;
      set_indent_ = true;
    }

    // 更新行和列信息。
    if (!literal->is_multi_line()) {
      current_column_ += literal_size;
    } else {
      for (char c : literal->text()) {
        if (c == '\n') {
          HandleNewline();
          current_line_info_->indent = string_column;
          set_indent_ = true;
        } else {
          ++current_column_;
        }
      }
    }

    // 处理字符串字面量的值
    if (literal->is_terminated()) {
      auto token =
          buffer_->AddToken({.kind = TokenKind::StringLiteral,
                             .token_line = string_line,
                             .column = string_column,
                             .literal_index = static_cast<int32_t>(
                                 buffer_->literal_string_storage_.size())});
      buffer_->literal_string_storage_.push_back(
          literal->ComputeValue(emitter_));
      return token;
    } else {
      COCKTAIL_DIAGNOSTIC(UnterminatedString, Error,
                          "String is missing a terminator.");
      emitter_.Emit(literal->text().begin(), UnterminatedString);
      return buffer_->AddToken({.kind = TokenKind::Error,
                                .token_line = string_line,
                                .column = string_column,
                                .error_length = literal_size});
    }
  }

  // 词法分析符号Token（例如括号、运算符等）。
  auto LexSymbolToken(llvm::StringRef& source_text,
                      TokenKind kind = TokenKind::Error) -> LexResult {
    // 用于根据给定的源代码片段计算符号的类型。
    auto compute_symbol_kind = [](llvm::StringRef source_text) {
      return llvm::StringSwitch<TokenKind>(source_text)
#define COCKTAIL_SYMBOL_TOKEN(Name, Spelling) \
  .StartsWith(Spelling, TokenKind::Name)
#include "Cocktail/LexNew/TokenKind.def"
          .Default(TokenKind::Error);
    };

    // 如果传入的符号类型为TokenKind::Error，则使用上面的lambda函数重新计算符号类型。
    // 如果计算结果仍然是TokenKind::Error，则返回一个错误。
    if (LLVM_UNLIKELY(kind == TokenKind::Error)) {
      kind = compute_symbol_kind(source_text);
      if (kind == TokenKind::Error) {
        return LexError(source_text);
      }
    } else {
      // 验证传入的符号类型是否与计算出的类型匹配。
      COCKTAIL_DCHECK(kind == compute_symbol_kind(source_text))
          << "Incoming token kind '" << kind
          << "' does not match computed kind '"
          << compute_symbol_kind(source_text) << "'!";
    }

    // 设置缩进。
    if (!set_indent_) {
      current_line_info_->indent = current_column_;
      set_indent_ = true;
    }

    // 关闭无效的开放组。
    CloseInvalidOpenGroups(kind);

    // 将新的符号令牌添加到缓冲区，并更新当前列和源代码片段。
    const char* location = source_text.begin();
    Token token = buffer_->AddToken(
        {.kind = kind, .token_line = current_line_, .column = current_column_});
    current_column_ += kind.fixed_spelling().size();
    source_text = source_text.drop_front(kind.fixed_spelling().size());

    // 处理开放符号：推入队列。
    if (kind.is_opening_symbol()) {
      open_groups_.push_back(token);
      return token;
    }

    // 处理关闭符号：进行进一步的处理。
    if (!kind.is_closing_symbol()) {
      return token;
    }

    TokenInfo& closing_token_info = buffer_->GetTokenInfo(token);

    // 检查是否有匹配的开放符号。
    if (open_groups_.empty()) {
      closing_token_info.kind = TokenKind::Error;
      closing_token_info.error_length = kind.fixed_spelling().size();

      COCKTAIL_DIAGNOSTIC(
          UnmatchedClosing, Error,
          "Closing symbol without a corresponding opening symbol.");
      emitter_.Emit(location, UnmatchedClosing);
      return token;
    }

    // 将关闭符号与其匹配的开放符号关联起来。
    Token opening_token = open_groups_.pop_back_val();
    TokenInfo& opening_token_info = buffer_->GetTokenInfo(opening_token);
    opening_token_info.closing_token = token;
    closing_token_info.opening_token = opening_token;
    return token;
  }

  // 将一个已经被词法分析的单词解释为类型字面量。
  auto LexWordAsTypeLiteralToken(llvm::StringRef word, int column)
      -> LexResult {
    // 如果单词的长度小于2，那么它太短，不能形成Token。
    if (word.size() < 2) {
      return LexResult::NoMatch();
    }
    // 第二个字符不是一个有效的数字（1到9）。
    if (word[1] < '1' || word[1] > '9') {
      return LexResult::NoMatch();
    }

    // 根据单词的第一个字符，确定它可能代表的类型字面量的类型。
    std::optional<TokenKind> kind;
    switch (word.front()) {
      case 'i':  // 整数类型字面量。
        kind = TokenKind::IntegerTypeLiteral;
        break;
      case 'u':  // 无符号整数类型字面量。
        kind = TokenKind::UnsignedIntegerTypeLiteral;
        break;
      case 'f':  // 浮点类型字面量。
        kind = TokenKind::FloatingPointTypeLiteral;
        break;
      default:
        return LexResult::NoMatch();
    };

    // 尝试将后缀转换为一个整数值。
    llvm::StringRef suffix = word.substr(1);
    if (!CanLexInteger(emitter_, suffix)) {
      return buffer_->AddToken(
          {.kind = TokenKind::Error,
           .token_line = current_line_,
           .column = column,
           .error_length = static_cast<int32_t>(word.size())});
    }
    llvm::APInt suffix_value;
    if (suffix.getAsInteger(10, suffix_value)) {
      return LexResult::NoMatch();
    }

    // 将新的类型字面量Token添加到缓冲区，并更新Token的信息。
    // 然后将后缀的整数值添加到literal_int_storage_向量中。
    auto token = buffer_->AddToken(
        {.kind = *kind, .token_line = current_line_, .column = column});
    buffer_->GetTokenInfo(token).literal_index =
        buffer_->literal_int_storage_.size();
    buffer_->literal_int_storage_.push_back(std::move(suffix_value));
    return token;
  }

  // 关闭符号kidn上所有不能保持打开状态的打开组。
  // 如果用户传递了Error，则关闭所有打开的组。
  auto CloseInvalidOpenGroups(TokenKind kind) -> void {
    // 检查是否为关闭符号。
    if (!kind.is_closing_symbol() && kind != TokenKind::Error) {
      return;
    }

    // 处理打开的组。
    while (!open_groups_.empty()) {
      // 获取最近的开放符号，并确定其类型。
      Token opening_token = open_groups_.back();
      TokenKind opening_kind = buffer_->GetTokenInfo(opening_token).kind;
      // 如果传入的kind与最近的开放符号匹配，那么直接返回。
      if (kind == opening_kind.closing_symbol()) {
        return;
      }

      // 处理不匹配的情况。
      open_groups_.pop_back();
      COCKTAIL_DIAGNOSTIC(
          MismatchedClosing, Error,
          "Closing symbol does not match most recent opening symbol.");
      token_emitter_.Emit(opening_token, MismatchedClosing);

      COCKTAIL_CHECK(!buffer_->tokens().empty())
          << "Must have a prior opening token!";
      Token prev_token = buffer_->tokens().end()[-1];

      // 添加关闭符号。
      Token closing_token = buffer_->AddToken(
          {.kind = opening_kind.closing_symbol(),
           .has_trailing_space = buffer_->HasTrailingWhitespace(prev_token),
           .is_recovery = true,
           .token_line = current_line_,
           .column = current_column_});
      // 更新开放和关闭令牌的信息，使它们相互引用。
      TokenInfo& opening_token_info = buffer_->GetTokenInfo(opening_token);
      TokenInfo& closing_token_info = buffer_->GetTokenInfo(closing_token);
      opening_token_info.closing_token = closing_token;
      closing_token_info.opening_token = opening_token;
    }
  }

  // 获取或创建一个标识符。
  auto GetOrCreateIdentifier(llvm::StringRef text) -> Identifier {
    // 将文本text作为键插入到标识符映射中。
    // 如果该文本不存在于映射中，将创建一个新的标识符。
    auto insert_result = buffer_->identifier_map_.insert(
        {text, Identifier(buffer_->identifier_infos_.size())});
    if (insert_result.second) {  // 成功插入了新的键值对。
      buffer_->identifier_infos_.push_back({text});
    }
    return insert_result.first->second;
  }

  // 从源文本中词法分析关键字或标识符。
  auto LexKeywordOrIdentifier(llvm::StringRef& source_text) -> LexResult {
    // 检查Unicode字符，ASCII值（7位的字符编码）大于0x7F（0111
    // 1111）就是Unicode。
    if (static_cast<unsigned char>(source_text.front()) > 0x7F) {
      // TODO: Need to add support for Unicode lexing.
      return LexError(source_text);
    }
    COCKTAIL_CHECK(IsAlpha(source_text.front()) || source_text.front() == '_');

    // 设置缩进。
    if (!set_indent_) {
      current_line_info_->indent = current_column_;
      set_indent_ = true;
    }

    // 扫描标识符。
    llvm::StringRef identifier_text = ScanForIdentifierPrefix(source_text);
    COCKTAIL_CHECK(!identifier_text.empty())
        << "Must have at least one character!";
    int identifier_column = current_column_;
    current_column_ += identifier_text.size();
    source_text = source_text.drop_front(identifier_text.size());

    // 检查类型字面量。
    if (LexResult result =
            LexWordAsTypeLiteralToken(identifier_text, identifier_column)) {
      return result;
    }

    // 检查关键字。
    TokenKind kind = llvm::StringSwitch<TokenKind>(identifier_text)
#define COCKTAIL_KEYWORD_TOKEN(Name, Spelling) .Case(Spelling, TokenKind::Name)
#include "Cocktail/LexNew/TokenKind.def"
                         .Default(TokenKind::Error);
    if (kind != TokenKind::Error) {
      return buffer_->AddToken({.kind = kind,
                                .token_line = current_line_,
                                .column = identifier_column});
    }

    // 如果文本既不是类型字面量也不是关键字，则将其视为通用标识符。
    return buffer_->AddToken({.kind = TokenKind::Identifier,
                              .token_line = current_line_,
                              .column = identifier_column,
                              .id = GetOrCreateIdentifier(identifier_text)});
  }

  // 处理词法分析器遇到的错误。
  auto LexError(llvm::StringRef& source_text) -> LexResult {
    llvm::StringRef error_text = source_text.take_while([](char c) {
      // 字母、数字、下划线、制表符或换行符通常不被视为错误。
      if (IsAlnum(c)) {
        return false;
      }
      switch (c) {
        case '_':
        case '\t':
        case '\n':
          return false;
        default:
          break;
      }
      // 检查字符是否是已知的符号。
      return llvm::StringSwitch<bool>(llvm::StringRef(&c, 1))
#define COCKTAIL_SYMBOL_TOKEN(Name, Spelling) .StartsWith(Spelling, false)
#include "Cocktail/LexNew/TokenKind.def"
          .Default(true);
    });
    // 如果error_text为空，意味着没有找到任何错误字符。
    if (error_text.empty()) {
      error_text = source_text.take_front(1);
    }

    // 添加错误Token。
    auto token = buffer_->AddToken(
        {.kind = TokenKind::Error,
         .token_line = current_line_,
         .column = current_column_,
         .error_length = static_cast<int32_t>(error_text.size())});
    COCKTAIL_DIAGNOSTIC(UnrecognizedCharacters, Error,
                        "Encountered unrecognized characters while parsing.");
    emitter_.Emit(error_text.begin(), UnrecognizedCharacters);

    // 更新列和源文本。
    current_column_ += error_text.size();
    source_text = source_text.drop_front(error_text.size());
    return token;
  }

  auto AddEndOfFileToken() -> void {
    buffer_->AddToken({.kind = TokenKind::EndOfFile,
                       .token_line = current_line_,
                       .column = current_column_});
  }

  // 构建并返回一个分派表（DispatchTableT），用于根据输入字符决定应该调用
  // 哪个词法分析函数。这是一个优化技巧，允许词法分析器快速决定如何处理给
  // 定的字符，而不是使用一系列的if-else语句或switch语句。
  constexpr static auto MakeDispatchTable() -> DispatchTableT {
    // 初始整个分派表被设置为错误处理函数。这意味着除非我们明确为某个字符
    // 设置了其他处理函数，否则默认行为是处理错误。
    // 使用+符号可以将lambda函数转换为函数指针，前提是该lambda不捕获任何外部变量。
    DispatchTableT table = {};
    auto dispatch_lex_error = +[](Lexer& lexer, llvm::StringRef& source_text) {
      return lexer.LexError(source_text);
    };
    for (int i = 0; i < 256; ++i) {
      table[i] = dispatch_lex_error;
    }

    // 对于符号，只根据其首字符进行分派，然后由符号词法分析器确定其确切的类型。
    auto dispatch_lex_symbol = +[](Lexer& lexer, llvm::StringRef& source_text) {
      return lexer.LexSymbolToken(source_text);
    };
// 使用宏设置分派表。
#define COCKTAIL_SYMBOL_TOKEN(TokenName, Spelling) \
  table[(Spelling)[0]] = dispatch_lex_symbol;
#include "Cocktail/LexNew/TokenKind.def"

    // 单字符符号是独立的，不会与其他符号结合，因此可以直接确定它们的类型。
#define COCKTAIL_ONE_CHAR_SYMBOL_TOKEN(TokenName, Spelling)                \
  table[(Spelling)[0]] = +[](Lexer& lexer, llvm::StringRef& source_text) { \
    return lexer.LexSymbolToken(source_text, TokenKind::TokenName);        \
  };
#include "Cocktail/LexNew/TokenKind.def"

    // 处理关键字或标识符
    auto dispatch_lex_word = +[](Lexer& lexer, llvm::StringRef& source_text) {
      return lexer.LexKeywordOrIdentifier(source_text);
    };
    table['_'] = dispatch_lex_word;
    // 这里因为这个函数是constexpr，无法使用llvm::seq，如果用代码会像：
    // for (auto c : llvm::seq('a', 'z')) {
    //   table[c] = dispatch_lex_word;
    // }
    for (unsigned char c = 'a'; c <= 'z'; ++c) {
      table[c] = dispatch_lex_word;
    }
    for (unsigned char c = 'A'; c <= 'Z'; ++c) {
      table[c] = dispatch_lex_word;
    }
    // 为所有非ASCII的UTF-8字符设置了分派函数。这是基于以下假设：由于
    // 空白字符已经被跳过，所以剩下的有效的Unicode字符应该是标识符的一部分。
    for (int i = 0x80; i < 0x100; ++i) {
      table[i] = dispatch_lex_word;
    }

    // 处理数字字面量。
    auto dispatch_lex_numeric =
        +[](Lexer& lexer, llvm::StringRef& source_text) {
          return lexer.LexNumericLiteral(source_text);
        };
    for (unsigned char c = '0'; c <= '9'; ++c) {
      table[c] = dispatch_lex_numeric;
    }

    // 处理字符串字面量。
    auto dispatch_lex_string = +[](Lexer& lexer, llvm::StringRef& source_text) {
      return lexer.LexStringLiteral(source_text);
    };
    table['\''] = dispatch_lex_string;
    table['"'] = dispatch_lex_string;
    table['#'] = dispatch_lex_string;

    return table;
  };

 private:
  TokenizedBuffer* buffer_;
  // 将源代码缓冲区的位置转换为更高级的表示。
  SourceBufferLocationTranslator translator_;
  LexerDiagnosticEmitter emitter_;  // 针对词法分析的诊断。
  // 将令牌的位置信息转换为其他格式表示。
  TokenLocationTranslator token_translator_;
  TokenDiagnosticEmitter token_emitter_;  // 针对token发出诊断信息。

  Line current_line_;            // 表示当前正在词法分析的行。
  LineInfo* current_line_info_;  // 表示当前行的附加信息。

  int current_column_ = 0;   // 表示当前正在词法分析的列号。
  bool set_indent_ = false;  // 用于标记是否设置了缩进。
  // 用于跟踪开放的括号以确保它们正确匹配。
  llvm::SmallVector<Token, 8> open_groups_;
};

auto TokenizedBuffer::Lex(SourceBuffer& source, DiagnosticConsumer& consumer)
    -> TokenizedBuffer {
  // 初始化词法分析器。
  TokenizedBuffer buffer(source);
  ErrorTrackingDiagnosticConsumer error_tracking_consumer(consumer);
  Lexer lexer(buffer, error_tracking_consumer);

  // 基于源文本的第一个字节，构建一个函数指针表，
  // 可以使用它来分派到正确的词法分析函数。
  constexpr Lexer::DispatchTableT DispatchTable = Lexer::MakeDispatchTable();

  llvm::StringRef source_text = source.text();
  while (lexer.SkipWhitespace(source_text)) {
    Lexer::LexResult result =
        DispatchTable[static_cast<unsigned char>(source_text.front())](
            lexer, source_text);
    COCKTAIL_CHECK(result) << "Failed to form a token!";
  }

  // 文件结束标记（EOF）总是被认为是空白。
  lexer.NoteWhitespace();

  lexer.CloseInvalidOpenGroups(TokenKind::Error);
  lexer.AddEndOfFileToken();

  if (error_tracking_consumer.seen_error()) {
    buffer.has_errors_ = true;
  }

  return buffer;
}

auto TokenizedBuffer::GetKind(Token token) const -> TokenKind {
  return GetTokenInfo(token).kind;
}

auto TokenizedBuffer::GetLine(Token token) const -> Line {
  return GetTokenInfo(token).token_line;
}

auto TokenizedBuffer::GetLineNumber(Token token) const -> int {
  return GetLineNumber(GetLine(token));
}

auto TokenizedBuffer::GetLineNumber(Line line) const -> int {
  return line.index + 1;
}

auto TokenizedBuffer::GetColumnNumber(Token token) const -> int {
  return GetTokenInfo(token).column + 1;
}

auto TokenizedBuffer::GetIndentColumnNumber(Line line) const -> int {
  return GetLineInfo(line).indent + 1;
}

auto TokenizedBuffer::GetTokenText(Token token) const -> llvm::StringRef {
  const auto& token_info = GetTokenInfo(token);
  llvm::StringRef fixed_spelling = token_info.kind.fixed_spelling();
  if (!fixed_spelling.empty()) {
    return fixed_spelling;
  }

  if (token_info.kind == TokenKind::Error) {
    const auto& line_info = GetLineInfo(token_info.token_line);
    int64_t token_start = line_info.start + token_info.column;
    return source_->text().substr(token_start, token_info.error_length);
  }

  if (token_info.kind == TokenKind::IntegerLiteral ||
      token_info.kind == TokenKind::RealLiteral) {
    const auto& line_info = GetLineInfo(token_info.token_line);
    int64_t token_start = line_info.start + token_info.column;
    std::optional<NumericLiteral> relexed_token =
        NumericLiteral::Lex(source_->text().substr(token_start));
    COCKTAIL_CHECK(relexed_token) << "Could not reform numeric literal token.";
    return relexed_token->text();
  }

  if (token_info.kind == TokenKind::StringLiteral) {
    const auto& line_info = GetLineInfo(token_info.token_line);
    int64_t token_start = line_info.start + token_info.column;
    std::optional<StringLiteral> relexed_token =
        StringLiteral::Lex(source_->text().substr(token_start));
    COCKTAIL_CHECK(relexed_token) << "Could not reform string literal token.";
    return relexed_token->text();
  }

  if (token_info.kind.is_sized_type_literal()) {
    const auto& line_info = GetLineInfo(token_info.token_line);
    int64_t token_start = line_info.start + token_info.column;
    llvm::StringRef suffix =
        source_->text().substr(token_start + 1).take_while(IsDecimalDigit);
    return llvm::StringRef(suffix.data() - 1, suffix.size() + 1);
  }

  if (token_info.kind == TokenKind::EndOfFile) {
    return llvm::StringRef();
  }

  COCKTAIL_CHECK(token_info.kind == TokenKind::Identifier) << token_info.kind;
  return GetIdentifierText(token_info.id);
}

auto TokenizedBuffer::GetIdentifier(Token token) const -> Identifier {
  const auto& token_info = GetTokenInfo(token);
  COCKTAIL_CHECK(token_info.kind == TokenKind::Identifier) << token_info.kind;
  return token_info.id;
}

auto TokenizedBuffer::GetIntegerLiteral(Token token) const
    -> const llvm::APInt& {
  const auto& token_info = GetTokenInfo(token);
  COCKTAIL_CHECK(token_info.kind == TokenKind::IntegerLiteral)
      << token_info.kind;
  return literal_int_storage_[token_info.literal_index];
}

auto TokenizedBuffer::GetRealLiteral(Token token) const -> RealLiteralValue {
  const auto& token_info = GetTokenInfo(token);
  COCKTAIL_CHECK(token_info.kind == TokenKind::RealLiteral) << token_info.kind;

  const auto& line_info = GetLineInfo(token_info.token_line);
  int64_t token_start = line_info.start + token_info.column;
  char second_char = source_->text()[token_start + 1];
  bool is_decimal = second_char != 'x' && second_char != 'b';

  return {.mantissa = literal_int_storage_[token_info.literal_index],
          .exponent = literal_int_storage_[token_info.literal_index + 1],
          .is_decimal = is_decimal};
}

auto TokenizedBuffer::GetStringLiteral(Token token) const -> llvm::StringRef {
  const auto& token_info = GetTokenInfo(token);
  COCKTAIL_CHECK(token_info.kind == TokenKind::StringLiteral)
      << token_info.kind;
  return literal_string_storage_[token_info.literal_index];
}

auto TokenizedBuffer::GetTypeLiteralSize(Token token) const
    -> const llvm::APInt& {
  const auto& token_info = GetTokenInfo(token);
  COCKTAIL_CHECK(token_info.kind.is_sized_type_literal()) << token_info.kind;
  return literal_int_storage_[token_info.literal_index];
}

auto TokenizedBuffer::GetMatchedClosingToken(Token opening_token) const
    -> Token {
  const auto& opening_token_info = GetTokenInfo(opening_token);
  COCKTAIL_CHECK(opening_token_info.kind.is_opening_symbol())
      << opening_token_info.kind;
  return opening_token_info.closing_token;
}

auto TokenizedBuffer::GetMatchedOpeningToken(Token closing_token) const
    -> Token {
  const auto& closing_token_info = GetTokenInfo(closing_token);
  COCKTAIL_CHECK(closing_token_info.kind.is_closing_symbol())
      << closing_token_info.kind;
  return closing_token_info.opening_token;
}

auto TokenizedBuffer::HasLeadingWhitespace(Token token) const -> bool {
  auto it = TokenIterator(token);
  return it == tokens().begin() || GetTokenInfo(*(it - 1)).has_trailing_space;
}

auto TokenizedBuffer::HasTrailingWhitespace(Token token) const -> bool {
  return GetTokenInfo(token).has_trailing_space;
}

auto TokenizedBuffer::IsRecoveryToken(Token token) const -> bool {
  return GetTokenInfo(token).is_recovery;
}

auto TokenizedBuffer::GetIdentifierText(Identifier identifier) const
    -> llvm::StringRef {
  return identifier_infos_[identifier.index].text;
}

auto TokenizedBuffer::PrintWidths::Widen(const PrintWidths& widths) -> void {
  index = std::max(widths.index, index);
  kind = std::max(widths.kind, kind);
  column = std::max(widths.column, column);
  line = std::max(widths.line, line);
  indent = std::max(widths.indent, indent);
}
static auto ComputeDecimalPrintedWidth(int number) -> int {
  COCKTAIL_CHECK(number >= 0) << "Negative numbers are not supported.";
  if (number == 0) {
    return 1;
  }

  return static_cast<int>(std::log10(number)) + 1;
}

auto TokenizedBuffer::GetTokenPrintWidths(Token token) const -> PrintWidths {
  PrintWidths widths = {};
  widths.index = ComputeDecimalPrintedWidth(token_infos_.size());
  widths.kind = GetKind(token).name().size();
  widths.line = ComputeDecimalPrintedWidth(GetLineNumber(token));
  widths.column = ComputeDecimalPrintedWidth(GetColumnNumber(token));
  widths.indent =
      ComputeDecimalPrintedWidth(GetIndentColumnNumber(GetLine(token)));
  return widths;
}

auto TokenizedBuffer::Print(llvm::raw_ostream& output_stream) const -> void {
  if (tokens().begin() == tokens().end()) {
    return;
  }

  output_stream << "- filename: " << source_->filename() << "\n"
                << "  tokens: [\n";

  PrintWidths widths = {};
  widths.index = ComputeDecimalPrintedWidth((token_infos_.size()));
  for (Token token : tokens()) {
    widths.Widen(GetTokenPrintWidths(token));
  }

  for (Token token : tokens()) {
    PrintToken(output_stream, token, widths);
    output_stream << "\n";
  }
  output_stream << "  ]\n";
}

auto TokenizedBuffer::PrintToken(llvm::raw_ostream& output_stream,
                                 Token token) const -> void {
  PrintToken(output_stream, token, {});
}

auto TokenizedBuffer::PrintToken(llvm::raw_ostream& output_stream, Token token,
                                 PrintWidths widths) const -> void {
  widths.Widen(GetTokenPrintWidths(token));
  int token_index = token.index;
  const auto& token_info = GetTokenInfo(token);
  llvm::StringRef token_text = GetTokenText(token);

  output_stream << llvm::formatv(
      "    { index: {0}, kind: {1}, line: {2}, column: {3}, indent: {4}, "
      "spelling: '{5}'",
      llvm::format_decimal(token_index, widths.index),
      llvm::right_justify(llvm::formatv("'{0}'", token_info.kind.name()).str(),
                          widths.kind + 2),
      llvm::format_decimal(GetLineNumber(token_info.token_line), widths.line),
      llvm::format_decimal(GetColumnNumber(token), widths.column),
      llvm::format_decimal(GetIndentColumnNumber(token_info.token_line),
                           widths.indent),
      token_text);

  switch (token_info.kind) {
    case TokenKind::Identifier:
      output_stream << ", identifier: " << GetIdentifier(token).index;
      break;
    case TokenKind::IntegerLiteral:
      output_stream << ", value: `";
      GetIntegerLiteral(token).print(output_stream, /*isSigned=*/false);
      output_stream << "`";
      break;
    case TokenKind::RealLiteral:
      output_stream << ", value: `" << GetRealLiteral(token) << "`";
      break;
    case TokenKind::StringLiteral:
      output_stream << ", value: `" << GetStringLiteral(token) << "`";
      break;
    default:
      if (token_info.kind.is_opening_symbol()) {
        output_stream << ", closing_token: "
                      << GetMatchedClosingToken(token).index;
      } else if (token_info.kind.is_closing_symbol()) {
        output_stream << ", opening_token: "
                      << GetMatchedOpeningToken(token).index;
      }
      break;
  }

  if (token_info.has_trailing_space) {
    output_stream << ", has_trailing_space: true";
  }
  if (token_info.is_recovery) {
    output_stream << ", recovery: true";
  }

  output_stream << " },";
}

auto TokenizedBuffer::GetLineInfo(Line line) -> LineInfo& {
  return line_infos_[line.index];
}

auto TokenizedBuffer::GetLineInfo(Line line) const -> const LineInfo& {
  return line_infos_[line.index];
}

auto TokenizedBuffer::AddLine(LineInfo info) -> Line {
  line_infos_.push_back(info);
  return Line(static_cast<int>(line_infos_.size()) - 1);
}

auto TokenizedBuffer::GetTokenInfo(Token token) -> TokenInfo& {
  return token_infos_[token.index];
}

auto TokenizedBuffer::GetTokenInfo(Token token) const -> const TokenInfo& {
  return token_infos_[token.index];
}

auto TokenizedBuffer::AddToken(TokenInfo info) -> Token {
  token_infos_.push_back(info);
  expected_parse_tree_size_ += info.kind.expected_parse_tree_size();
  return Token(static_cast<int>(token_infos_.size()) - 1);
}

auto TokenIterator::Print(llvm::raw_ostream& output) const -> void {
  output << token_.index;
}

auto TokenizedBuffer::SourceBufferLocationTranslator::GetLocation(
    const char* loc) -> DiagnosticLocation {
  // 检查位置是否在缓冲区内。
  COCKTAIL_CHECK(StringRefContainsPointer(buffer_->source_->text(), loc))
      << "location not within buffer";
  int64_t offset = loc - buffer_->source_->text().begin();

  // 查找第一个在给定位置之后开始的行。
  const auto* line_it = std::partition_point(
      buffer_->line_infos_.begin(), buffer_->line_infos_.end(),
      [offset](const LineInfo& line) { return line.start <= offset; });

  // 退回一行以找到包含给定位置的行。
  COCKTAIL_CHECK(line_it != buffer_->line_infos_.begin())
      << "location precedes the start of the first line";
  --line_it;
  int line_number = line_it - buffer_->line_infos_.begin();
  int column_number = offset - line_it->start;

  // 从缓冲区中获取该行的文本。如果该行在词法分析时还没有完全分析，它会调整行的长度。
  llvm::StringRef line =
      buffer_->source_->text().substr(line_it->start, line_it->length);
  if (line_it->length == static_cast<int32_t>(llvm::StringRef::npos)) {
    COCKTAIL_CHECK(line.take_front(column_number).count('\n') == 0)
        << "Currently we assume no unlexed newlines prior to the error column, "
           "but there was one when erroring at "
        << buffer_->source_->filename() << ":" << line_number << ":"
        << column_number;
    auto end_newline_pos = line.find('\n', column_number);
    if (end_newline_pos != llvm::StringRef::npos) {
      line = line.take_front(end_newline_pos);
    }
  }

  return {.file_name = buffer_->source_->filename(),
          .line = line,
          .line_number = line_number + 1,
          .column_number = column_number + 1};
}

auto TokenLocationTranslator::GetLocation(Token token) -> DiagnosticLocation {
  const auto& token_info = buffer_->GetTokenInfo(token);
  const auto& line_info = buffer_->GetLineInfo(token_info.token_line);
  const char* token_start =
      buffer_->source_->text().begin() + line_info.start + token_info.column;

  return TokenizedBuffer::SourceBufferLocationTranslator(buffer_).GetLocation(
      token_start);
}

}  // namespace Cocktail::Lex