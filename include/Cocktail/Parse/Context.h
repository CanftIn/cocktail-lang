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

    auto Print(llvm::raw_ostream& output) const -> void {
      output << state << " @" << token << " subtree_start=" << subtree_start
             << " has_error=" << has_error;
    };

    // 表示状态。
    State state;
    // 找到错误则设置为 true。这意味着可能需要上下文错误恢复。
    bool has_error = false;

    // 用于存储表达式状态中的优先级信息，以确定操作符的优先级。
    // 其中，ambient_precedence处理表达式如何与外部上下文交互，
    // 而lhs_precedence适用于表示操作符表达式的左侧。
    PrecedenceGroup ambient_precedence;
    PrecedenceGroup lhs_precedence;

    // 基于子树提供上下文的token。
    // 这通常是子树中的第一个token，但有时可能是子树内的一个token。
    // 它通常用于子树的根节点。
    Lex::Token token;
    // 子树在 Tree 中的起始偏移量。
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

  // 打印堆栈转储的信息。
  auto PrintForStackDump(llvm::raw_ostream& output) const -> void;

  // 获取器函数:

  // 返回解析树的引用。
  auto tree() const -> const Tree& { return *tree_; }
  // 返回令牌化的缓冲区的引用。
  auto tokens() const -> const Lex::TokenizedBuffer& { return *tokens_; }
  // 返回诊断发射器的引用。
  auto emitter() -> Lex::TokenDiagnosticEmitter& { return *emitter_; }
  // 返回令牌缓冲区中的当前位置。
  auto position() -> Lex::TokenIterator& { return position_; }
  auto position() const -> Lex::TokenIterator { return position_; }
  // 返回状态堆栈的引用。
  auto state_stack() -> llvm::SmallVector<StateStackEntry>& {
    return state_stack_;
  }
  auto state_stack() const -> const llvm::SmallVector<StateStackEntry>& {
    return state_stack_;
  }

 private:
  // 用于 PrintForStackDump 打印单个令牌。
  auto PrintTokenForStackDump(llvm::raw_ostream& output, Lex::Token token) const
      -> void;

  // 指向解析树的指针。
  Tree* tree_;
  // 指向token buffer的指针。
  Lex::TokenizedBuffer* tokens_;
  // 指向诊断发射器的指针。
  Lex::TokenDiagnosticEmitter* emitter_;
  // 是否输出详细（verbose）信息。
  llvm::raw_ostream* vlog_stream_;
  // token buffer中的当前位置。
  Lex::TokenIterator position_;
  // 文件结束(EOF)token。
  Lex::TokenIterator end_;
  // 状态堆栈。
  llvm::SmallVector<StateStackEntry> state_stack_;
};

}  // namespace Cocktail::Parse

#endif  // COCKTAIL_PARSE_CONTEXT_H