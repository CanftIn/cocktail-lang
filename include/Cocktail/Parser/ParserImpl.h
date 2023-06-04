#ifndef COCKTAIL_PARSER_PARSE_IMPL_H
#define COCKTAIL_PARSER_PARSE_IMPL_H

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Cocktail/Lexer/TokenKind.h"
#include "Cocktail/Lexer/TokenizedBuffer.h"
#include "Cocktail/Parser/ParseNodeKind.h"
#include "Cocktail/Parser/ParseTree.h"
#include "Cocktail/Parser/Precedence.h"
#include "llvm/ADT/Optional.h"

namespace Cocktail {

class ParseTree::Parser {
 public:
  static auto Parse(TokenizedBuffer& tokens, TokenDiagnosticEmitter& emitter)
      -> ParseTree;

 private:
  struct SubtreeStart;

  explicit Parser(ParseTree& tree_arg, TokenizedBuffer& tokens_arg,
                  TokenDiagnosticEmitter& emitter);

  auto AtEndOfFile() -> bool {
    return tokens.GetKind(*position) == TokenKind::EndOfFile();
  }

  [[nodiscard]] auto NextTokenKind() const -> TokenKind {
    return tokens.GetKind(*position);
  }

  [[nodiscard]] auto NextTokenIs(TokenKind kind) const -> bool {
    return NextTokenKind() == kind;
  }

  [[nodiscard]] auto NextTokenIsOneOf(
      std::initializer_list<TokenKind> kinds) const -> bool {
    return NextTokenKind().IsOneOf(kinds);
  }

  auto Consume(TokenKind kind) -> TokenizedBuffer::Token;

  auto ConsumeIf(TokenKind kind) -> llvm::Optional<TokenizedBuffer::Token>;

  auto AddLeafNode(ParseNodeKind kind, TokenizedBuffer::Token token) -> Node;

  // Composes `consumeIf` and `addLeafNode`
  auto ConsumeAndAddLeafNodeIf(TokenKind t_kind, ParseNodeKind n_kind)
      -> llvm::Optional<Node>;

  auto MarkNodeError(Node n) -> void;

  auto GetSubtreeStartPosition() -> SubtreeStart;

  auto AddNode(ParseNodeKind n_kind, TokenizedBuffer::Token t,
               SubtreeStart start, bool has_error = false) -> Node;

  auto SkipMatchingGroup() -> bool;

  auto SkipTo(TokenizedBuffer::Token t) -> void;

  auto FindNextOf(std::initializer_list<TokenKind> desired_kinds)
      -> llvm::Optional<TokenizedBuffer::Token>;

  using SemiHandler = llvm::function_ref<
      auto(TokenizedBuffer::Token semi)->llvm::Optional<Node>>;

  auto SkipPastLikelyEnd(TokenizedBuffer::Token skip_root, SemiHandler on_semi)
      -> llvm::Optional<Node>;

  auto ParseCloseParen(TokenizedBuffer::Token open_paren, ParseNodeKind kind)
      -> llvm::Optional<Node>;

  template <typename ListElementParser, typename ListCompletionHandler>
  auto ParseParenList(ListElementParser list_element_parser,
                      ParseNodeKind comma_kind,
                      ListCompletionHandler list_handler)
      -> llvm::Optional<Node>;

  auto ParseFunctionParameter() -> llvm::Optional<Node>;

  auto ParseFunctionSignature() -> bool;

  auto ParseCodeBlock() -> Node;

  auto ParseFunctionDeclaration() -> Node;

  auto ParseVariableDeclaration() -> Node;

  auto ParseEmptyDeclaration() -> Node;

  auto ParseDeclaration() -> llvm::Optional<Node>;

  auto ParseParenExpression() -> llvm::Optional<Node>;

  auto ParsePrimaryExpression() -> llvm::Optional<Node>;

  // Parses a designator expression suffix starting with `.`.
  auto ParseDesignatorExpression(SubtreeStart start, bool has_errors)
      -> llvm::Optional<Node>;

  // Parses a call expression suffix starting with `(`.
  auto ParseCallExpression(SubtreeStart start, bool has_errors)
      -> llvm::Optional<Node>;

  auto ParsePostfixExpression() -> llvm::Optional<Node>;

  enum class OperatorFixity { Prefix, Infix, Postfix };

  auto IsLexicallyValidInfixOperator() -> bool;

  auto DiagnoseOperatorFixity(OperatorFixity fixity) -> void;

  auto IsTrailingOperatorInfix() -> bool;

  auto ParseOperatorExpression(PrecedenceGroup precedence)
      -> llvm::Optional<Node>;

  auto ParseExpression() -> llvm::Optional<Node>;

  auto ParseType() -> llvm::Optional<Node>;

  auto ParseExpressionStatement() -> llvm::Optional<Node>;

  auto ParseParenCondition(TokenKind introducer) -> llvm::Optional<Node>;

  auto ParseIfStatement() -> llvm::Optional<Node>;

  auto ParseWhileStatement() -> llvm::Optional<Node>;

  enum class KeywordStatementArgument {
    None,
    Optional,
    Mandatory,
  };

  auto ParseKeywordStatement(ParseNodeKind kind,
                             KeywordStatementArgument argument)
      -> llvm::Optional<Node>;

  auto ParseStatement() -> llvm::Optional<Node>;

  enum class PatternKind {
    Parameter,
    Variable,
  };

  auto ParsePattern(PatternKind kind) -> llvm::Optional<Node>;

  ParseTree& tree;
  TokenizedBuffer& tokens;
  TokenDiagnosticEmitter& emitter;

  TokenizedBuffer::TokenIterator position;
  TokenizedBuffer::TokenIterator end;
};

}  // namespace Cocktail

#endif  // COCKTAIL_PARSER_PARSE_IMPL_H