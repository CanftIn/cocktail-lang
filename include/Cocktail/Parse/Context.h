#ifndef COCKTAIL_PARSE_CONTEXT_H
#define COCKTAIL_PARSE_CONTEXT_H

#include <optional>

#include "Cocktail/Common/Check.h"
#include "Cocktail/Common/VLog.h"
#include "Cocktail/Lex/TokenKind.h"
#include "Cocktail/Lex/TokenizedBuffer.h"
#include "Cocktail/Parse/NodeKind.h"
#include "Cocktail/Parse/Precedence.h"
#include "Cocktail/Parse/State.h"
#include "Cocktail/Parse/Tree.h"

namespace Cocktail::Parse {

// 为解析器处理程序提供上下文和共享功能的。
class Context {
 public:
  // 表示操作符的结合性，如前缀、中缀和后缀。
  enum class OperatorFixity : int8_t { Prefix, Infix, Postfix };

  // 表示列表中的token类型，如逗号、关闭或逗号关闭。
  enum class ListTokenKind : int8_t { Comma, Close, CommaClose };

  // 表示模式的种类，如推断的参数、参数、变量和Let。
  enum class PatternKind : int8_t {
    DeducedParameter,
    Parameter,
    Variable,
    Let
  };

  // 表示声明的上下文，如文件、类、接口和命名约束。
  enum class DeclarationContext : int8_t {
    File,  // Top-level context.
    Class,
    Interface,
    NamedConstraint,
  };

  // 用于跟踪state_stack_上的状态。
  // 它包含了与状态相关的各种信息，如状态、优先级、令牌等。
  struct StateStackEntry : public Printable<StateStackEntry> {
    explicit StateStackEntry(State state, PrecedenceGroup ambient_precedence,
                             PrecedenceGroup lhs_precedence, Lex::Token token,
                             int32_t subtree_start)
        : state(state),
          ambient_precedence(ambient_precedence),
          lhs_precedence(lhs_precedence),
          token(token),
          subtree_start(subtree_start) {}

    // Prints state information for verbose output.
    auto Print(llvm::raw_ostream& output) const -> void {
      output << state << " @" << token << " subtree_start=" << subtree_start
             << " has_error=" << has_error;
    };

    // The state.
    State state;
    // Set to true to indicate that an error was found, and that contextual
    // error recovery may be needed.
    bool has_error = false;

    // Precedence information used by expression states in order to determine
    // operator precedence. The ambient_precedence deals with how the expression
    // should interact with outside context, while the lhs_precedence is
    // specific to the lhs of an operator expression.
    PrecedenceGroup ambient_precedence;
    PrecedenceGroup lhs_precedence;

    // A token providing context based on the subtree. This will typically be
    // the first token in the subtree, but may sometimes be a token within. It
    // will typically be used for the subtree's root node.
    Lex::Token token;
    // The offset within the Tree of the subtree start.
    int32_t subtree_start;
  };

  // 确保StateStackEntry的大小为12字节。
  // 如果它变得更大，可能需要更好的打包策略。
  //   state = 1 byte
  //   has_error = 1 byte
  //   ambient_precedence = 1 byte
  //   lhs_precedence = 1 byte
  //   token = 4 bytes
  //   subtree_start = 4 bytes
  static_assert(sizeof(StateStackEntry) == 12,
                "StateStackEntry has unexpected size!");

  explicit Context(Tree& tree, Lex::TokenizedBuffer& tokens,
                   Lex::TokenDiagnosticEmitter& emitter,
                   llvm::raw_ostream* vlog_stream);

  // 为解析树添加没有子节点的节点
  auto AddLeafNode(NodeKind kind, Lex::Token token, bool has_error = false)
      -> void;

  // 解析树添加有子节点的节点。
  auto AddNode(NodeKind kind, Lex::Token token, int subtree_start,
               bool has_error) -> void;

  // 返回当前位置并越过它。
  auto Consume() -> Lex::Token { return *(position_++); }

 private:
  // Prints a single token for a stack dump. Used by PrintForStackDump.
  auto PrintTokenForStackDump(llvm::raw_ostream& output, Lex::Token token) const
      -> void;

  Tree* tree_;
  Lex::TokenizedBuffer* tokens_;
  Lex::TokenDiagnosticEmitter* emitter_;

  // 是否输出详细（verbose）信息。
  llvm::raw_ostream* vlog_stream_;

  // token buffer中的当前位置。
  Lex::TokenIterator position_;
  // EndOfFile token.
  Lex::TokenIterator end_;

  llvm::SmallVector<StateStackEntry> state_stack_;
};

}  // namespace Cocktail::Parse

#endif  // COCKTAIL_PARSE_CONTEXT_H