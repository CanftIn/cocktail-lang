#ifndef COCKTAIL_PARSER_PARSE_IMPL_H
#define COCKTAIL_PARSER_PARSE_IMPL_H

#include "Cocktail/Lexer/TokenKind.h"
#include "Cocktail/Lexer/TokenizedBuffer.h"
#include "Cocktail/Parser/ParseNodeKind.h"
#include "Cocktail/Parser/ParseTree.h"
#include "llvm/ADT/Optional.h"

namespace Cocktail {

class ParseTree::Parser {
 public:
  static auto Parse(TokenizedBuffer& tokens, DiagnosticEmitter& de)
      -> ParseTree;

 private:
  struct SubtreeStart;

  explicit Parser(ParseTree& tree_arg, TokenizedBuffer& tokens_arg)
      : tree(tree_arg),
        tokens(tokens_arg),
        position(tokens.Tokens().begin()),
        end(tokens.Tokens().end()) {}

  auto Consume(TokenKind kind) -> TokenizedBuffer::Token;

  auto ConsumeIf(TokenKind kind) -> llvm::Optional<TokenizedBuffer::Token>;

  auto AddLeafNode(ParseNodeKind kind, TokenizedBuffer::Token token) -> Node;

  auto ConsumeAndAddLeafNodeIf(TokenKind t_kind, ParseNodeKind n_kind)
      -> llvm::Optional<Node>;

  auto MarkNodeError(Node n) -> void;

  auto StartSubtree() -> SubtreeStart;

  auto AddNode(ParseNodeKind n_kind, TokenizedBuffer::Token t,
               SubtreeStart& start, bool has_error = false) -> Node;

  auto SkipMatchingGroup() -> bool;

  auto SkipPastLikelyDeclarationEnd(TokenizedBuffer::Token skip_root,
                                    bool is_inside_declaration = true)
      -> llvm::Optional<Node>;

  auto ParseFunctionSignature() -> Node;

  auto ParseCodeBlock() -> Node;

  auto ParseFunctionDeclaration() -> Node;

  auto ParseEmptyDeclaration() -> Node;

  auto ParseDeclaration() -> llvm::Optional<Node>;

  ParseTree& tree;
  TokenizedBuffer& tokens;

  TokenizedBuffer::TokenIterator position;
  TokenizedBuffer::TokenIterator end;
};

}  // namespace Cocktail

#endif  // COCKTAIL_PARSER_PARSE_IMPL_H