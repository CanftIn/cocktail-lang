#include "Cocktail/LexNew/StringLiteral.h"

#include "Cocktail/Common/CharacterSet.h"
#include "Cocktail/Common/Check.h"
#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Cocktail/Lex/LexHelpers.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/ConvertUTF.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormatVariadic.h"

namespace Cocktail::Lex {

using LexerDiagnosticEmitter = DiagnosticEmitter<const char*>;

static constexpr char MultiLineIndicator[] = R"(''')";
static constexpr char DoubleQuotedMultiLineIndicator[] = R"(""")";

struct StringLiteral::Introducer {
  // 表示字符串的种类。
  MultiLineKind kind;
  // 表示字符串字面量的终止符。
  llvm::StringRef terminator;
  // 表示引入部分的长度。
  int prefix_size;

  // 从给定的源文本中词法分析字符串字面量的引入部分。
  static auto Lex(llvm::StringRef source_text) -> std::optional<Introducer>;
};

auto StringLiteral::Introducer::Lex(llvm::StringRef source_text)
    -> std::optional<Introducer> {
  MultiLineKind kind = NotMultiLine;
  llvm::StringRef indicator;
  if (source_text.startswith(MultiLineIndicator)) {
    kind = MultiLine;
    indicator = llvm::StringRef(MultiLineIndicator);
  } else if (source_text.startswith(DoubleQuotedMultiLineIndicator)) {
    kind = MultiLineWithDoubleQuotes;
    indicator = llvm::StringRef(DoubleQuotedMultiLineIndicator);
  }

  if (kind != NotMultiLine) {
    // 检查其余部分是否是一个有效的文件类型指示符。
    // 有效的文件类型指示符是一个不包含 # 或 " 的字符序列，后面跟着一个换行符。
    auto prefix_end = source_text.find_first_of("#\n\"", indicator.size());
    if (prefix_end != llvm::StringRef::npos &&
        source_text[prefix_end] == '\n') {
      return Introducer{.kind = kind,
                        .terminator = indicator,
                        .prefix_size = static_cast<int>(prefix_end + 1)};
    }
  }

  // 源文本是常规字符串字面量（即以双引号开始）。
  if (!source_text.empty() && source_text[0] == '"') {
    return Introducer{
        .kind = NotMultiLine, .terminator = "\"", .prefix_size = 1};
  }

  return std::nullopt;
}

namespace {
struct alignas(8) CharSet {
  bool Elements[UCHAR_MAX + 1];

  constexpr CharSet(std::initializer_list<char> chars) : Elements() {
    for (char c : chars) {
      Elements[static_cast<unsigned char>(c)] = true;
    }
  }

  constexpr auto operator[](char c) const -> bool {
    return Elements[static_cast<unsigned char>(c)];
  }
};
}  // namespace

auto StringLiteral::Lex(llvm::StringRef source_text)
    -> std::optional<StringLiteral> {
  int64_t cursor = 0;
  const int64_t source_text_size = source_text.size();

  // 确定前缀中的#数量。
  while (cursor < source_text_size && source_text[cursor] == '#') {
    ++cursor;
  }
  const int hash_level = cursor;

  const std::optional<Introducer> introducer =
      Introducer::Lex(source_text.substr(hash_level));
  if (!introducer) {
    return std::nullopt;
  }

  cursor += introducer->prefix_size;
  const int prefix_len = cursor;

  // 初始化终结符和转义序列标记。
  llvm::SmallString<16> terminator(introducer->terminator);
  llvm::SmallString<16> escape("\\");

  // 调整终结符和转义序列的大小。
  // 如果终结符为'''，那么根据hash_level的大小改变终结符，如果hash_level为2，
  // 则终结符为'''##，那么转义序列则为\##。
  terminator.resize(terminator.size() + hash_level, '#');
  escape.resize(escape.size() + hash_level, '#');

  /// TODO: 在找到终结符之前检测多行字符串字面量的缩进/反缩进。
  for (; cursor < source_text_size; ++cursor) {
    // 快速跳过不感兴趣的字符。
    static constexpr CharSet InterestingChars = {'\\', '\n', '"', '\''};
    if (!InterestingChars[source_text[cursor]]) {
      continue;
    }

    // 多字符的终结符和转义序列都以可预测的字符开始，
    // 并且不包含嵌入的、未转义的终结符或换行符。
    switch (source_text[cursor]) {
      case '\\':  // 处理转义字符。
        if (escape.size() == 1 ||
            source_text.substr(cursor + 1).startswith(escape.substr(1))) {
          cursor += escape.size();
          // 单行字符串且转义字符是换行符。
          if (cursor >= source_text_size || (introducer->kind == NotMultiLine &&
                                             source_text[cursor] == '\n')) {
            llvm::StringRef text = source_text.take_front(cursor);
            return StringLiteral(text, text.drop_front(prefix_len), hash_level,
                                 introducer->kind,
                                 /*is_terminated=*/false);
          }
        }
        break;
      case '\n':
        // 单行字符串。
        if (introducer->kind == NotMultiLine) {
          llvm::StringRef text = source_text.take_front(cursor);
          return StringLiteral(text, text.drop_front(prefix_len), hash_level,
                               introducer->kind,
                               /*is_terminated=*/false);
        }
        break;
      case '"':
      case '\'':
        if (source_text.substr(cursor).startswith(terminator)) {
          llvm::StringRef text =
              source_text.substr(0, cursor + terminator.size());
          llvm::StringRef content =
              source_text.substr(prefix_len, cursor - prefix_len);
          return StringLiteral(text, content, hash_level, introducer->kind,
                               /*is_terminated=*/true);
        }
        break;
      default:
        // 对于非终结符，不执行任何操作。
        break;
    }
  }

  return StringLiteral(source_text, source_text.drop_front(prefix_len),
                       hash_level, introducer->kind,
                       /*is_terminated=*/false);
}

// 计算给定文本中最后一行的缩进（即最后一行前面的水平空白字符序列）。
static auto ComputeIndentOfFinalLine(llvm::StringRef text) -> llvm::StringRef {
  int indent_end = text.size();  // 从文本的末尾开始，逐字符向前检查。
  for (int i = indent_end - 1; i >= 0; --i) {
    if (text[i] ==
        '\n') {  // 如果遇到换行符\n，则该位置之后的所有字符都是最后一行的缩进。
      int indent_start = i + 1;
      return text.substr(indent_start, indent_end - indent_start);
    }
    if (!IsSpace(text[i])) {  // 如果遇到非空格字符，则更新缩进的结束位置。
      indent_end = i;
    }
  }
  // 如果没有找到换行符，这意味着给定的文本不包含换行符，这是一个错误情况。
  llvm_unreachable("Given text is required to contain a newline.");
}

// 检查多行字符串字面量是否正确缩进，并找到应从每行字符串中删除的前导空格。
static auto CheckIndent(LexerDiagnosticEmitter& emitter, llvm::StringRef text,
                        llvm::StringRef content) -> llvm::StringRef {
  llvm::StringRef indent = ComputeIndentOfFinalLine(text);

  if (indent.end() != content.end()) {
    COCKTAIL_DIAGNOSTIC(
        ContentBeforeStringTerminator, Error,
        "Only whitespace is permitted before the closing `\"\"\"` of a "
        "multi-line string.");
    emitter.Emit(indent.end(), ContentBeforeStringTerminator);
  }

  return indent;
}

// 将 `\u{HHHHHH}` 转义序列扩展为UTF-8编码单元序列。
static auto ExpandUnicodeEscapeSequence(LexerDiagnosticEmitter& emitter,
                                        llvm::StringRef digits,
                                        std::string& result) -> bool {
  unsigned code_point;
  // 首先检查给定的数字是否有效。
  if (!CanLexInteger(emitter, digits)) {
    return false;
  }
  // 将这些数字从十六进制转换为整数形式的码点。
  if (digits.getAsInteger(16, code_point) || code_point > 0x10FFFF) {
    COCKTAIL_DIAGNOSTIC(
        UnicodeEscapeTooLarge, Error,
        "Code point specified by `\\u{{...}}` escape is greater "
        "than 0x10FFFF.");
    emitter.Emit(digits.begin(), UnicodeEscapeTooLarge);
    return false;
  }

  // 检查码点是否在有效的Unicode范围内。
  if (code_point >= 0xD800 && code_point < 0xE000) {
    COCKTAIL_DIAGNOSTIC(UnicodeEscapeSurrogate, Error,
                        "Code point specified by `\\u{{...}}` escape is a "
                        "surrogate character.");
    emitter.Emit(digits.begin(), UnicodeEscapeSurrogate);
    return false;
  }

  // 使用LLVM的ConvertUTF32toUTF8函数将码点转换为UTF-8编码单元。
  const llvm::UTF32 utf32_code_units[1] = {code_point};
  llvm::UTF8 utf8_code_units[6];
  const llvm::UTF32* src_pos = utf32_code_units;
  llvm::UTF8* dest_pos = utf8_code_units;
  llvm::ConversionResult conv_result = llvm::ConvertUTF32toUTF8(
      &src_pos, src_pos + 1, &dest_pos, dest_pos + 6, llvm::strictConversion);
  if (conv_result != llvm::conversionOK) {
    llvm_unreachable("conversion of valid code point to UTF-8 cannot fail");
  }
  // 将UTF-8编码单元添加到结果字符串中。
  result.insert(result.end(), reinterpret_cast<char*>(utf8_code_units),
                reinterpret_cast<char*>(dest_pos));
  return true;
}

// 这个函数扩展转义序列，并将扩展的值追加到给定的result字符串中。
static auto ExpandAndConsumeEscapeSequence(LexerDiagnosticEmitter& emitter,
                                           llvm::StringRef& content,
                                           std::string& result) -> void {
  COCKTAIL_CHECK(!content.empty()) << "should have escaped closing delimiter";
  char first = content.front();
  content = content.drop_front(1);

  // 根据转义序列的第一个字符确定转义序列的类型。
  switch (first) {
    case 't':
      result += '\t';
      return;
    case 'n':
      result += '\n';
      return;
    case 'r':
      result += '\r';
      return;
    case '"':
      result += '"';
      return;
    case '\'':
      result += '\'';
      return;
    case '\\':
      result += '\\';
      return;
    case '0':  // 检查其后是否有数字，如果有，则发出错误。
      result += '\0';
      if (!content.empty() && IsDecimalDigit(content.front())) {
        COCKTAIL_DIAGNOSTIC(DecimalEscapeSequence, Error,
                            "Decimal digit follows `\\0` escape sequence. "
                            "Use `\\x00` instead "
                            "of `\\0` if the next character is a digit.");
        emitter.Emit(content.begin(), DecimalEscapeSequence);
        return;
      }
      return;
    case 'x':  // 检查其后是否有两个十六进制数字，如果有，则将其转换为相应的字符。
      if (content.size() >= 2 && IsUpperHexDigit(content[0]) &&
          IsUpperHexDigit(content[1])) {
        result +=
            static_cast<char>(llvm::hexFromNibbles(content[0], content[1]));
        content = content.drop_front(2);
        return;
      }
      COCKTAIL_DIAGNOSTIC(HexadecimalEscapeMissingDigits, Error,
                          "Escape sequence `\\x` must be followed by two "
                          "uppercase hexadecimal digits, for example `\\x0F`.");
      emitter.Emit(content.begin(), HexadecimalEscapeMissingDigits);
      break;
    case 'u': {  // 使用`ExpandUnicodeEscapeSequence`函数扩展它。
      llvm::StringRef remaining = content;
      if (remaining.consume_front("{")) {
        llvm::StringRef digits = remaining.take_while(IsUpperHexDigit);
        remaining = remaining.drop_front(digits.size());
        if (!digits.empty() && remaining.consume_front("}")) {
          if (!ExpandUnicodeEscapeSequence(emitter, digits, result)) {
            break;
          }
          content = remaining;
          return;
        }
      }
      COCKTAIL_DIAGNOSTIC(
          UnicodeEscapeMissingBracedDigits, Error,
          "Escape sequence `\\u` must be followed by a braced sequence of "
          "uppercase hexadecimal digits, for example `\\u{{70AD}}`.");
      emitter.Emit(content.begin(), UnicodeEscapeMissingBracedDigits);
      break;
    }
    default:
      COCKTAIL_DIAGNOSTIC(UnknownEscapeSequence, Error,
                          "Unrecognized escape sequence `{0}`.", char);
      emitter.Emit(content.begin() - 1, UnknownEscapeSequence, first);
      break;
  }

  result += first;
}

// 处理字符串字面量中的转义序列并移除字符串的缩进。
static auto ExpandEscapeSequencesAndRemoveIndent(
    LexerDiagnosticEmitter& emitter, llvm::StringRef contents, int hash_level,
    llvm::StringRef indent) -> std::string {
  // 存储处理后的结果。
  std::string result;
  result.reserve(contents.size());

  // 表示转义序列的引导字符（例如\或#）。
  llvm::SmallString<16> escape("\\");
  escape.resize(1 + hash_level, '#');

  // 处理contents中的每一行。
  while (true) {
    // 如果当前行的开头不匹配indent，则移除所有前导空白，并发出一个关于不匹配的缩进的诊断消息。
    if (!contents.consume_front(indent)) {
      const char* line_start = contents.begin();
      contents = contents.drop_while(IsHorizontalWhitespace);
      if (!contents.startswith("\n")) {
        COCKTAIL_DIAGNOSTIC(
            MismatchedIndentInString, Error,
            "Indentation does not match that of the closing `'''` in "
            "multi-line string literal.");
        emitter.Emit(line_start, MismatchedIndentInString);
      }
    }

    // 在最后一次展开转义时跟踪结果的长度，以确保在回溯时不会将其误解为未转义。
    size_t last_escape_length = 0;

    // 处理每一行的内容，直到遇到换行符或行结束。
    while (true) {
      // 追加下一段纯文本。
      auto end_of_regular_text = contents.find_if([](char c) {
        return c == '\n' || c == '\\' ||
               (IsHorizontalWhitespace(c) && c != ' ');
      });
      result += contents.substr(0, end_of_regular_text);
      contents = contents.substr(end_of_regular_text);

      if (contents.empty()) {
        return result;
      }

      // 如果遇到换行符，移除尾随空白并添加一个换行符到result。
      if (contents.consume_front("\n")) {
        while (!result.empty() && result.back() != '\n' &&
               IsSpace(result.back()) && result.length() > last_escape_length) {
          result.pop_back();
        }
        result += '\n';
        // 继续下一行。
        break;
      }

      // 如果遇到除空格以外的水平空白，检查它是否位于行尾。
      // 如果不是，发出一个关于无效水平空白的诊断消息。
      if (IsHorizontalWhitespace(contents.front())) {
        COCKTAIL_CHECK(contents.front() != ' ')
            << "should not have stopped at a plain space";
        auto after_space = contents.find_if_not(IsHorizontalWhitespace);
        if (after_space == llvm::StringRef::npos ||
            contents[after_space] != '\n') {
          COCKTAIL_DIAGNOSTIC(
              InvalidHorizontalWhitespaceInString, Error,
              "Whitespace other than plain space must be expressed with an "
              "escape sequence in a string literal.");
          emitter.Emit(contents.begin(), InvalidHorizontalWhitespaceInString);
          result += contents.substr(0, after_space);
        }
        contents = contents.substr(after_space);
        continue;
      }

      // 如果遇到反斜杠但不是转义序列的引导字符，将其添加到result。
      if (!contents.consume_front(escape)) {
        result += contents.front();
        contents = contents.drop_front(1);
        continue;
      }

      if (contents.consume_front("\n")) {
        break;
      }

      // 处理并消耗转义序列。
      ExpandAndConsumeEscapeSequence(emitter, contents, result);
      last_escape_length = result.length();
    }
  }
}

// 计算字符串字面量的值。
auto StringLiteral::ComputeValue(LexerDiagnosticEmitter& emitter) const
    -> std::string {
  // 如果字符串没有正确终止，则返回空字符串。
  if (!is_terminated_) {
    return "";
  }
  // 如果字符串使用了不正确的多行分隔符，则发出错误。
  if (multi_line_ == MultiLineWithDoubleQuotes) {
    COCKTAIL_DIAGNOSTIC(
        MultiLineStringWithDoubleQuotes, Error,
        "Use `'''` delimiters for a multi-line string literal, not `\"\"\"`.");
    emitter.Emit(text_.begin(), MultiLineStringWithDoubleQuotes);
  }
  // 使用CheckIndent函数检查缩进。
  llvm::StringRef indent =
      multi_line_ ? CheckIndent(emitter, text_, content_) : llvm::StringRef();
  // 使用ExpandEscapeSequencesAndRemoveIndent函数扩展转义序列并删除缩进。
  return ExpandEscapeSequencesAndRemoveIndent(emitter, content_, hash_level_,
                                              indent);
}

}  // namespace Cocktail::Lex