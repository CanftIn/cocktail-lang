#ifndef COCKTAIL_LEX_TOKENIZED_BUFFER_H
#define COCKTAIL_LEX_TOKENIZED_BUFFER_H

#include <cstdint>
#include <iterator>

#include "Cocktail/Common/IndexBase.h"
#include "Cocktail/Common/Ostream.h"
#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Cocktail/Lex/TokenKind.h"
#include "Cocktail/Source/SourceBuffer.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail::Lex {

class TokenizedBuffer;

// Token 是一个轻量级的句柄，用于代表在 TokenizedBuffer 中进行词法分析的标记。
// Token对象设计为按值传递，而不是按引用或指针传递。它被设计为在存储时小巧且高效。
// 来自同一个 TokenizedBuffer 的 Token对象可以相互比较，既可以确定它们在缓冲区中
// 是否为相同的标记，也可以确定它们在从缓冲区进行词法分析的标记流中的相对位置。
// 来自不同 TokenizedBuffer 的 Token 对象不能有意义地进行比较。
struct Token : public ComparableIndexBase {
  using ComparableIndexBase::ComparableIndexBase;
};

// Line代表在 TokenizedBuffer 中进行词法分析的行。
struct Line : public ComparableIndexBase {
  using ComparableIndexBase::ComparableIndexBase;
};

// Identifier代表在 TokenizedBuffer 中进行词法分析的标识符。
struct Identifier : public IndexBase {
  using IndexBase::IndexBase;

  static const Identifier Invalid;
};

constexpr Identifier Identifier::Invalid = Identifier(Identifier::InvalidIndex);

// 随机访问迭代器，用于在缓冲区内部迭代token。
class TokenIterator
    : public llvm::iterator_facade_base<
          TokenIterator, std::random_access_iterator_tag, const Token, int>,
      public Printable<TokenIterator> {
 public:
  TokenIterator() = delete;

  explicit TokenIterator(Token token) : token_(token) {}

  auto operator==(const TokenIterator& rhs) const -> bool {
    return token_ == rhs.token_;
  }
  auto operator<(const TokenIterator& rhs) const -> bool {
    return token_ < rhs.token_;
  }

  auto operator*() const -> const Token& { return token_; }

  using iterator_facade_base::operator-;
  auto operator-(const TokenIterator& rhs) const -> int {
    return token_.index - rhs.token_.index;
  }

  auto operator+=(int n) -> TokenIterator& {
    token_.index += n;
    return *this;
  }
  auto operator-=(int n) -> TokenIterator& {
    token_.index -= n;
    return *this;
  }

  auto Print(llvm::raw_ostream& output) const -> void;

 private:
  friend class TokenizedBuffer;

  Token token_;
};

// 表示一个实数字面值的值。
// 可以是一个二进制分数（mantissa * 2^exponent）或一个十进制分数（mantissa *
// 10^exponent）。
class RealLiteralValue : public Printable<RealLiteralValue> {
 public:
  auto Print(llvm::raw_ostream& output_stream) const -> void {
    mantissa.print(output_stream, /*isSigned=*/false);
    output_stream << "*" << (is_decimal ? "10" : "2") << "^" << exponent;
  }

  // 尾数，表示为一个无符号整数。
  const llvm::APInt& mantissa;

  // 指数，表示为一个有符号整数。
  const llvm::APInt& exponent;

  // If false, the value is mantissa * 2^exponent.
  // If true, the value is mantissa * 10^exponent.
  bool is_decimal;
};

class TokenLocationTranslator : public DiagnosticLocationTranslator<Token> {
 public:
  explicit TokenLocationTranslator(const TokenizedBuffer* buffer)
      : buffer_(buffer) {}

  // 将给定的标记映射到诊断位置。
  auto GetLocation(Token token) -> DiagnosticLocation override;

 private:
  const TokenizedBuffer* buffer_;
};

// 将源代码文本进行词法分析得到一系列token来构造的源代码缓冲区。
class TokenizedBuffer {
 public:
  // lex将源代码的缓冲区转换为标记化的缓冲区。
  static auto Lex(SourceBuffer& source, DiagnosticConsumer& consumer)
      -> TokenizedBuffer;

  // 获取给定标记的种类。
  [[nodiscard]] auto GetKind(Token token) const -> TokenKind;
  // 获取给定标记所在的行。
  [[nodiscard]] auto GetLine(Token token) const -> Line;
  // 返回基于1的列号。
  [[nodiscard]] auto GetLineNumber(Token token) const -> int;
  // 返回基于1的行号。
  [[nodiscard]] auto GetLineNumber(Line line) const -> int;
  // 返回基于1的列号。
  [[nodiscard]] auto GetColumnNumber(Token token) const -> int;
  // 返回基于1的缩进列号。
  [[nodiscard]] auto GetIndentColumnNumber(Line line) const -> int;
  // 返回此标记中的源文本。
  [[nodiscard]] auto GetTokenText(Token token) const -> llvm::StringRef;
  // 返回与此标记关联的标识符。标记的类型必须是 Identifier。
  [[nodiscard]] auto GetIdentifier(Token token) const -> Identifier;
  // 返回 IntegerLiteral() 标记的值。
  [[nodiscard]] auto GetIntegerLiteral(Token token) const -> const llvm::APInt&;
  // 返回 RealLiteral() 标记的值。
  [[nodiscard]] auto GetRealLiteral(Token token) const -> RealLiteralValue;
  // 返回 StringLiteral() 标记的值。
  [[nodiscard]] auto GetStringLiteral(Token token) const -> llvm::StringRef;
  // 返回在 *TypeLiteral() 标记中指定的大小。
  [[nodiscard]] auto GetTypeLiteralSize(Token token) const
      -> const llvm::APInt&;
  // 返回与给定的开放标记匹配的关闭标记。
  [[nodiscard]] auto GetMatchedClosingToken(Token opening_token) const -> Token;
  // 返回与给定的关闭标记匹配的开放标记。
  [[nodiscard]] auto GetMatchedOpeningToken(Token closing_token) const -> Token;
  // 返回给定标记是否有前导空白。
  [[nodiscard]] auto HasLeadingWhitespace(Token token) const -> bool;
  // 返回给定标记是否有尾随空白。
  [[nodiscard]] auto HasTrailingWhitespace(Token token) const -> bool;
  // 返回标记是否是作为错误恢复工作的一部分创建的。
  [[nodiscard]] auto IsRecoveryToken(Token token) const -> bool;
  // 返回标识符的文本。
  [[nodiscard]] auto GetIdentifierText(Identifier id) const -> llvm::StringRef;
  // 将标记化流的描述打印。
  auto Print(llvm::raw_ostream& output_stream) const -> void;
  // 打印单个标记的描述。
  auto PrintToken(llvm::raw_ostream& output_stream, Token token) const -> void;
  // 如果缓冲区存在在lexing时可检测到的错误，则返回true。
  [[nodiscard]] auto has_errors() const -> bool { return has_errors_; }
  // 返回表示缓冲区中所有标记的迭代器范围。
  [[nodiscard]] auto tokens() const -> llvm::iterator_range<TokenIterator> {
    return llvm::make_range(TokenIterator(Token(0)),
                            TokenIterator(Token(token_infos_.size())));
  }
  // 返回缓冲区中的标记数量。
  [[nodiscard]] auto size() const -> int { return token_infos_.size(); }
  // 返回为缓冲区中的标记创建的预期解析树节点的数量。
  [[nodiscard]] auto expected_parse_tree_size() const -> int {
    return expected_parse_tree_size_;
  }
  // 返回源文件的文件名。
  auto filename() const -> llvm::StringRef { return source_->filename(); }

 private:
  class Lexer;
  friend Lexer;

  friend class TokenLocationTranslator;

  class SourceBufferLocationTranslator
      : public DiagnosticLocationTranslator<const char*> {
   public:
    explicit SourceBufferLocationTranslator(const TokenizedBuffer* buffer)
        : buffer_(buffer) {}

    auto GetLocation(const char* loc) -> DiagnosticLocation override;

   private:
    const TokenizedBuffer* buffer_;
  };

  struct PrintWidths {
    auto Widen(const PrintWidths& widths) -> void;

    int index;
    int kind;
    int column;
    int line;
    int indent;
  };

  // 包含了关于单个标记的信息。
  struct TokenInfo {
    TokenKind kind;

    bool has_trailing_space = false;

    bool is_recovery = false;

    Line token_line;

    int32_t column;

    union {
      static_assert(
          sizeof(Token) <= sizeof(int32_t),
          "Unable to pack token and identifier index into the same space!");

      Identifier id = Identifier::Invalid;
      int32_t literal_index;
      Token closing_token;
      Token opening_token;
      int32_t error_length;
    };
  };

  // 包含关于源代码中的单行的信息。
  struct LineInfo {
    explicit LineInfo(int64_t start)
        : start(start),
          length(static_cast<int32_t>(llvm::StringRef::npos)),
          indent(0) {}

    int64_t start;  // 源缓冲区内行开始的以零作为开始的字节偏移量。
    int32_t length;  // 行的字节长度。
    int32_t indent;  // 从第一个非空格行开始的字节偏移量。
  };

  // 包含关于标识符的信息。
  struct IdentifierInfo {
    llvm::StringRef text;
  };

  // 构造函数仅负责成员的简单初始化。
  explicit TokenizedBuffer(SourceBuffer& source) : source_(&source) {}

  // 返回给定行的信息。
  auto GetLineInfo(Line line) -> LineInfo&;
  [[nodiscard]] auto GetLineInfo(Line line) const -> const LineInfo&;
  // 添加一行信息并返回该行。
  auto AddLine(LineInfo info) -> Line;
  // 返回给定标记的信息。
  auto GetTokenInfo(Token token) -> TokenInfo&;
  [[nodiscard]] auto GetTokenInfo(Token token) const -> const TokenInfo&;
  // 添加一个标记信息并返回该标记。
  auto AddToken(TokenInfo info) -> Token;
  [[nodiscard]] auto GetTokenPrintWidths(Token token) const -> PrintWidths;
  // 使用给定的宽度打印一个标记。
  auto PrintToken(llvm::raw_ostream& output_stream, Token token,
                  PrintWidths widths) const -> void;
  // 指向源代码缓冲区的指针。
  SourceBuffer* source_;
  // 存储所有标记信息。
  llvm::SmallVector<TokenInfo, 16> token_infos_;
  // 存储所有行信息。
  llvm::SmallVector<LineInfo, 16> line_infos_;
  // 存储所有标识符信息。
  llvm::SmallVector<IdentifierInfo, 16> identifier_infos_;
  // 存储整数字面值。
  llvm::SmallVector<llvm::APInt, 16> literal_int_storage_;
  // 存储字符串字面值。
  llvm::SmallVector<std::string, 16> literal_string_storage_;
  // 将标识符文本映射到标识符对象。
  llvm::DenseMap<llvm::StringRef, Identifier> identifier_map_;
  // 预期为缓冲区中的标记创建的解析树节点的数量。
  int expected_parse_tree_size_ = 0;
  // 表示缓冲区是否有错误的布尔值。
  bool has_errors_ = false;
};

// 使用源缓冲区文本中的位置作为其位置信息来源的诊断发射器。
using LexerDiagnosticEmitter = DiagnosticEmitter<const char*>;

// 使用标记作为其位置信息来源的诊断发射器。
using TokenDiagnosticEmitter = DiagnosticEmitter<Token>;

}  // namespace Cocktail::Lex

#endif  // COCKTAIL_LEX_TOKENIZED_BUFFER_H
