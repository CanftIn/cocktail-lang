#include "Cocktail/Parser/ParserImpl.h"

#include <cstdlib>

#include "Cocktail/Lexer/TokenKind.h"
#include "Cocktail/Lexer/TokenizedBuffer.h"
#include "Cocktail/Parser/ParseNodeKind.h"
#include "Cocktail/Parser/ParseTree.h"
#include "llvm/ADT/Optional.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

auto ParseTree::Parser::Parse(TokenizedBuffer& tokens) -> ParseTree {
  ParseTree tree(tokens);
  tree.node_impls.reserve(tokens.Size());

  Parser parser(tree, tokens);
  while (parser.position != parser.end) {
    parser.ParseDeclaration();
  }

  assert(tree.Verify() && "Parse tree built but does not verify!");
  return tree;
}

auto ParseTree::Parser::Consume(TokenKind kind) -> TokenizedBuffer::Token {
  TokenizedBuffer::Token token = *position;
  assert(tokens.GetKind(token) == kind &&
         "The current token is the wrong kind!");
  ++position;
  return token;
}

auto ParseTree::Parser::ConsumeIf(TokenKind kind)
    -> llvm::Optional<TokenizedBuffer::Token> {
  if (tokens.GetKind(*position) != kind) {
    return {};
  }
  return *position++;
}

auto ParseTree::Parser::AddLeafNode(ParseNodeKind kind,
                                    TokenizedBuffer::Token token) -> Node {
  Node n(tree.node_impls.size());
  tree.node_impls.emplace_back(NodeImpl(kind, token, 1));
  return n;
}

auto ParseTree::Parser::ConsumeAndAddLeafNodeIf(TokenKind t_kind,
                                                ParseNodeKind n_kind)
    -> llvm::Optional<Node> {
  auto token = ConsumeIf(t_kind);
  if (!token) {
    return {};
  }
  return AddLeafNode(n_kind, *token);
}

auto ParseTree::Parser::MarkNodeError(Node n) -> void {
  tree.node_impls[n.index].has_error = true;
  tree.has_errors = true;
}

struct ParseTree::Parser::SubtreeStart {
  int tree_size;
  bool node_added = false;

  ~SubtreeStart() {
    assert(node_added && "Never added a node for a subtree region!");
  }
};

auto ParseTree::Parser::StartSubtree() -> SubtreeStart {
  return {static_cast<int>(tree.node_impls.size())};
}

auto ParseTree::Parser::AddNode(ParseNodeKind n_kind,
                                TokenizedBuffer::Token token,
                                SubtreeStart& start, bool has_error) -> Node {
  int tree_stop_size = tree.node_impls.size() + 1;
  int subtree_size = tree_stop_size - start.tree_size;

  Node n(tree.node_impls.size());
  tree.node_impls.emplace_back(NodeImpl(n_kind, token, subtree_size));
  if (has_error) {
    MarkNodeError(n);
  }

  start.node_added = true;
  return n;
}

auto ParseTree::Parser::SkipMatchingGroup() -> bool {
  assert(position != end && "Cannot skip at the end!");
  TokenizedBuffer::Token t = *position;
  TokenKind t_kind = tokens.GetKind(t);
  if (!t_kind.IsOpeningSymbol()) {
    return false;
  }

  position = std::next(
      TokenizedBuffer::TokenIterator(tokens.GetMatchedClosingToken(t)));
  return true;
}

auto ParseTree::Parser::SkipPastLikelyDeclarationEnd(
    TokenizedBuffer::Token skip_root, bool is_inside_declaration)
    -> llvm::Optional<Node> {
  if (position == end) {
    return {};
  }

  TokenizedBuffer::Line root_line = tokens.GetLine(skip_root);
  int root_line_indent = tokens.GetIndentColumnNumber(root_line);

  auto is_same_line_or_indent_greater_than_root =
      [&](TokenizedBuffer::Token t) {
        TokenizedBuffer::Line line = tokens.GetLine(t);
        if (line == root_line) {
          return true;
        }

        return tokens.GetIndentColumnNumber(line) > root_line_indent;
      };

  do {
    TokenKind current_kind = tokens.GetKind(*position);
    if (current_kind == TokenKind::CloseCurlyBrace()) {
      return {};
    }

    if (current_kind == TokenKind::Semi()) {
      TokenizedBuffer::Token semi = *position++;

      return AddLeafNode(is_inside_declaration
                             ? ParseNodeKind::DeclarationEnd()
                             : ParseNodeKind::EmptyDeclaration(),
                         semi);
    }

    if (SkipMatchingGroup()) {
      continue;
    }

    ++position;
  } while (position != end &&
           is_same_line_or_indent_greater_than_root(*position));

  return {};
}

auto ParseTree::Parser::ParseFunctionSignature() -> Node {
  assert(position != end && "Cannot parse past the end!");

  TokenizedBuffer::Token open_paren = Consume(TokenKind::OpenParen());
  assert(position != end &&
         "The lexer ensures we always have a closing paren!");

  auto start = StartSubtree();

  bool has_errors = false;
  auto close_paren = ConsumeIf(TokenKind::CloseParen());
  if (!close_paren) {
    llvm::errs() << "ERROR: unexpected token before the close of the"
                    "parameters on line "
                 << tokens.GetLineNumber(*position) << "!\n";
    has_errors = true;

    close_paren = tokens.GetMatchedClosingToken(open_paren);
    position = std::next(TokenizedBuffer::TokenIterator(*close_paren));
  }
  AddLeafNode(ParseNodeKind::ParameterListEnd(), *close_paren);

  return AddNode(ParseNodeKind::ParameterList(), open_paren, start, has_errors);
}

auto ParseTree::Parser::ParseCodeBlock() -> Node {
  assert(position != end && "Cannot parse past the end!");

  TokenizedBuffer::Token open_curly = Consume(TokenKind::OpenCurlyBrace());
  assert(position != end &&
         "The lexer ensures we always have a closing curly brace!");
  auto start = StartSubtree();

  bool has_errors = false;

  for (;;) {
    switch (tokens.GetKind(*position)) {
      case TokenKind::CloseCurlyBrace():
        break;
      case TokenKind::OpenCurlyBrace():
        ParseCodeBlock();
        continue;
      default:
        llvm::errs() << "ERROR: unexpected token before the close of the "
                        "function definition on line "
                     << tokens.GetLineNumber(*position) << "!\n";

        has_errors = true;
        position = TokenizedBuffer::TokenIterator(
            tokens.GetMatchedClosingToken(open_curly));
    }
    break;
  }
  AddLeafNode(ParseNodeKind::CodeBlockEnd(),
              Consume(TokenKind::CloseCurlyBrace()));

  return AddNode(ParseNodeKind::CodeBlock(), open_curly, start, has_errors);
}

auto ParseTree::Parser::ParseFunctionDeclaration() -> Node {
  assert(position != end && "Cannot parse past the end!");

  TokenizedBuffer::Token function_intro_token = Consume(TokenKind::FnKeyword());
  auto start = StartSubtree();
  auto add_error_function_node = [&] {
    return AddNode(ParseNodeKind::FunctionDeclaration(), function_intro_token,
                   start, true);
  };

  if (position == end) {
    llvm::errs() << "ERROR: File ended with a function declaration on line "
                 << tokens.GetLineNumber(function_intro_token) << "!\n";
    return add_error_function_node();
  }

  auto name_n = ConsumeAndAddLeafNodeIf(TokenKind::Identifier(),
                                        ParseNodeKind::Identifier());
  if (!name_n) {
    llvm::errs() << "ERROR: Function declaration with no name on line "
                 << tokens.GetLineNumber(function_intro_token) << "!\n";
    SkipPastLikelyDeclarationEnd(function_intro_token);
    return add_error_function_node();
  }
  if (position == end) {
    llvm::errs() << "ERROR: File ended after a function introducer and "
                    "identifier on line "
                 << tokens.GetLineNumber(function_intro_token) << "!\n";
    return add_error_function_node();
  }

  TokenizedBuffer::Token open_paren = *position;
  if (tokens.GetKind(open_paren) != TokenKind::OpenParen()) {
    llvm::errs()
        << "ERROR: Missing open parentheses in declaration of function '"
        << tokens.GetTokenText(tree.GetNodeToken(*name_n)) << "' on line "
        << tokens.GetLineNumber(function_intro_token) << "!\n";
    SkipPastLikelyDeclarationEnd(function_intro_token);
    return add_error_function_node();
  }

  assert(std::next(position) != end &&
         "Unbalanced parentheses should be rejected by the lexer.");
  TokenizedBuffer::Token close_paren =
      tokens.GetMatchedClosingToken(open_paren);

  Node signature_n = ParseFunctionSignature();
  assert(*std::prev(position) == close_paren &&
         "Should have parsed through the close paren, whether successfully "
         "or with errors.");
  if (tree.node_impls[signature_n.index].has_error) {
    SkipPastLikelyDeclarationEnd(function_intro_token);
    return add_error_function_node();
  }

  if (tokens.GetKind(*position) == TokenKind::OpenCurlyBrace()) {
    ParseCodeBlock();
  } else if (!ConsumeAndAddLeafNodeIf(TokenKind::Semi(),
                                      ParseNodeKind::DeclarationEnd())) {
    llvm::errs() << "ERROR: Function declaration not terminated by a "
                    "semicolon on line "
                 << tokens.GetLineNumber(close_paren) << "!\n";
    if (tokens.GetLine(*position) == tokens.GetLine(close_paren)) {
      SkipPastLikelyDeclarationEnd(function_intro_token);
    }
    return add_error_function_node();
  }

  return AddNode(ParseNodeKind::FunctionDeclaration(), function_intro_token,
                 start);
}

auto ParseTree::Parser::ParseEmptyDeclaration() -> Node {
  assert(position != end && "Cannot parse past the end!");
  return AddLeafNode(ParseNodeKind::EmptyDeclaration(),
                     Consume(TokenKind::Semi()));
}

auto ParseTree::Parser::ParseDeclaration() -> llvm::Optional<Node> {
  assert(position != end && "Cannot parse past the end!");
  TokenizedBuffer::Token token = *position;
  switch (tokens.GetKind(token)) {
    case TokenKind::FnKeyword():
      return ParseFunctionDeclaration();
    case TokenKind::Semi():
      return ParseEmptyDeclaration();
  }

  llvm::errs() << "ERROR: Unrecognized declaration introducer '"
               << tokens.GetTokenText(token) << "' on line "
               << tokens.GetLineNumber(token) << "!\n";

  if (auto found_semi_n = SkipPastLikelyDeclarationEnd(token, true)) {
    MarkNodeError(*found_semi_n);
    return *found_semi_n;
  }

  tree.has_errors = true;
  return {};
}

}  // namespace Cocktail