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

  //  解析一个开括号令牌，并在必要时进行诊断。
  // 它创建一个指定起始类型的叶子解析节点。如果没有开括号，将使用default_token。
  auto ConsumeAndAddOpenParen(Lex::Token default_token, NodeKind start_kind)
      -> std::optional<Lex::Token>;

  // 解析与 expected_open 对应的关闭符号，并在必要时跳过并进行诊断。
  // 它创建一个指定关闭类型的解析节点。
  auto ConsumeAndAddCloseSymbol(Lex::Token expected_open, StateStackEntry state,
                                NodeKind close_kind) -> void;

  // 组合 ConsumeIf 和 AddLeafNode，当 ConsumeIf 失败时返回 false。
  auto ConsumeAndAddLeafNodeIf(Lex::TokenKind token_kind, NodeKind node_kind)
      -> bool;

  // 返回当前位置的token并将位置移动到下一个token。它要求令牌是预期的类型。
  auto ConsumeChecked(Lex::TokenKind kind) -> Lex::Token;

  // 如果当前位置的令牌与给定的kind匹配，则返回它并前进到下一个位置。
  // 否则返回一个空的可选值。
  auto ConsumeIf(Lex::TokenKind kind) -> std::optional<Lex::Token>;

  // 对于HandlePattern，尝试使用包装关键字。
  auto ConsumeIfPatternKeyword(Lex::TokenKind keyword_token,
                               State keyword_state, int subtree_start) -> void;

  // 用于处理在解析列表（list）时，根据当前位置的标记（token）来确定应该如何消耗标记，
  // 添加特定的标记到解析树，并返回相应的列表标记类型。
  //
  // 1. 函数的参数 comma_kind 表示逗号标记的节点类型，
  //    通常用于创建解析树中的节点，以表示逗号的存在。
  //
  // 2. 函数的参数 close_kind 表示列表结束的标记类型，
  //    通常是闭合括号（如 )）。这个参数用于确定何时应该返回 Close 类型的列表标记。
  // 
  // 3. 参数 already_has_error 是一个布尔值，用于指示是否已经存在解析错误。
  //    如果设置为 true，则函数将抑制重复错误的发生。
  // 
  // 4. 函数首先检查当前位置的标记，如果是逗号（,）标记，则执行以下操作：
  //    - 消耗逗号标记。
  //    - 在解析树中添加一个节点，节点类型为 comma_kind，用于表示逗号的存在。
  //    - 返回 Comma 类型的列表标记，表示已经消耗了逗号。
  //
  // 5. 如果当前位置的标记是列表结束标记（close_kind），则执行以下操作：
  //    - 不消耗当前标记。
  //    - 返回 Close 类型的列表标记，表示列表已经结束。
  //
  // 6. 如果当前位置的标记既不是逗号也不是列表结束标记，则执行以下操作：
  //    - 将当前位置向前移动，跳过当前标记，以处理无效的标记。
  //    - 根据 already_has_error 参数的值，可能会发出错误信息。
  //
  // 7. 函数的返回类型是 ListTokenKind，它是一个枚举类型，可以表示不同的
  //    列表标记类型。可能的返回值包括：
  //    - Comma：表示已经消耗了逗号。
  //    - Close：表示列表已经结束。
  //    - CommaClose：表示既消耗了逗号又遇到了列表结束标记。
  auto ConsumeListToken(NodeKind comma_kind, Lex::TokenKind close_kind,
                        bool already_has_error) -> ListTokenKind;

  // 在当前的括号级别查找给定类型的下一个令牌。
  auto FindNextOf(std::initializer_list<Lex::TokenKind> desired_kinds)
      -> std::optional<Lex::Token>;

  // 如果token是匹配组的开放符号，则跳到匹配的关闭符号并返回true，否则返回false。
  auto SkipMatchingGroup() -> bool;

  // 向前跳过声明或语句的可能结束位置。
  // 这个函数的主要功能是帮助跳过解析错误后，找到一个可能标志着声明或语句结束的位置，
  // 并返回该位置的分号（semicolon）标记（token）。
  // 参数 skip_root 是一个用于开始跳过操作的起始位置的标记（token）。
  // 通常这个标记将用于确定在何处开始跳过。
  //
  // 函数执行的是一种启发式（heuristic）操作，它根据以下策略来识别可能表示声明或语句结束的位置：
  // - 如果在解析过程中遇到了闭合大括号}，
  //   则认为可能是整个上下文（context）的结束。
  //   这意味着可能是一个函数或块的结束。
  // - 如果在解析过程中遇到了分号 ;，则认为这是声明或语句的结束符号。
  // - 如果在解析过程中遇到了换行符，并且换行符的缩进（indentation）
  //   与起始位置的缩进相同或更少，那么可能存在缺少分号的情况。
  //   这种情况通常发生在多行声明或语句没有正确缩进的情况下。
  //   因此，函数会尝试找到缺少的分号。
  auto SkipPastLikelyEnd(Lex::Token skip_root) -> std::optional<Lex::Token>;

  // 向前跳到给定的token。验证它确实是向前的。
  auto SkipTo(Lex::Token t) -> void;

  // 返回当前token是否满足中缀操作符的词法有效性规则。
  auto IsLexicallyValidInfixOperator() -> bool;

  // 确定当前的尾随操作符是否应被视为中缀。
  auto IsTrailingOperatorInfix() -> bool;

  // 诊断当前令牌是否为给定的固定性正确编写。
  // 例如，由于缺少强制的空格。无论是否有错误，都预计解析将继续。
  auto DiagnoseOperatorFixity(OperatorFixity fixity) -> void;

  // 获取下一个要使用的token的类型。
  auto PositionKind() const -> Lex::TokenKind {
    return tokens_->GetKind(*position_);
  }

  // 测试下一个要使用的token是否属于指定的类型。
  auto PositionIs(Lex::TokenKind kind) const -> bool {
    return PositionKind() == kind;
  }

  // 弹出状态并保留其值以供检查。
  auto PopState() -> StateStackEntry {
    auto back = state_stack_.pop_back_val();
    COCKTAIL_VLOG() << "Pop " << state_stack_.size() << ": " << back << "\n";
    return back;
  }

  // 弹出状态并丢弃它。
  auto PopAndDiscardState() -> void {
    COCKTAIL_VLOG() << "PopAndDiscard " << state_stack_.size() - 1 << ": "
                    << state_stack_.back() << "\n";
    state_stack_.pop_back();
  }

  // 使用当前位置作为上下文推送一个新状态。
  auto PushState(State state) -> void {
    PushState(StateStackEntry(state, PrecedenceGroup::ForTopLevelExpression(),
                              PrecedenceGroup::ForTopLevelExpression(),
                              *position_, tree_->size()));
  }

  // 为上下文推送带有特定token的新状态。在使用非子树开头的令牌形成新子树时使用。
  auto PushState(State state, Lex::Token token) -> void {
    PushState(StateStackEntry(state, PrecedenceGroup::ForTopLevelExpression(),
                              PrecedenceGroup::ForTopLevelExpression(), token,
                              tree_->size()));
  }

  // 推送一个具有特定优先级的新表达式状态。
  auto PushStateForExpression(PrecedenceGroup ambient_precedence) -> void {
    PushState(StateStackEntry(State::Expression, ambient_precedence,
                              PrecedenceGroup::ForTopLevelExpression(),
                              *position_, tree_->size()));
  }

  // 推送一个带有详细优先级的新状态，用于表达式恢复状态。
  auto PushStateForExpressionLoop(State state,
                                  PrecedenceGroup ambient_precedence,
                                  PrecedenceGroup lhs_precedence) -> void {
    PushState(StateStackEntry(state, ambient_precedence, lhs_precedence,
                              *position_, tree_->size()));
  }

  // 将构造好的状态推入堆栈。
  auto PushState(StateStackEntry state) -> void {
    COCKTAIL_VLOG() << "Push " << state_stack_.size() << ": " << state << "\n";
    state_stack_.push_back(state);
    COCKTAIL_CHECK(state_stack_.size() < (1 << 20))
        << "Excessive stack size: likely infinite loop";
  }

  /// TODO: fix comment
  // 根据state_stack_返回当前声明上下文。
  // 这将在接近上下文的情况下被调用。
  // 尽管state_stack_的深度看起来可能是O(n)，
  // 但有效的解析应该只需要向下看几个步骤。
  // 这目前假设它是从声明的DeclarationScopeLoop中调用的。
  auto GetDeclarationContext() -> DeclarationContext;

  // 将错误沿状态堆栈向上传播到父状态。
  auto ReturnErrorOnState() -> void { state_stack_.back().has_error = true; }

  // 为缺少semi的声明发出一个诊断。
  auto EmitExpectedDeclarationSemi(Lex::TokenKind expected_kind) -> void;

  // 为缺少semi的声明或定义发出一个诊断。
  auto EmitExpectedDeclarationSemiOrDefinition(Lex::TokenKind expected_kind)
      -> void;

  // 在声明中处理错误恢复，特别是在可能开始的任何定义之前。
  auto RecoverFromDeclarationError(StateStackEntry state,
                                   NodeKind parse_node_kind,
                                   bool skip_past_likely_end) -> void;

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