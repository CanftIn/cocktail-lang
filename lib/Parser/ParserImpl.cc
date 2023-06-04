#include "Cocktail/Parser/ParserImpl.h"

#include <cstdlib>

#include "Cocktail/Lexer/TokenKind.h"
#include "Cocktail/Lexer/TokenizedBuffer.h"
#include "Cocktail/Parser/ParseNodeKind.h"
#include "Cocktail/Parser/ParseTree.h"
#include "llvm/ADT/Optional.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

struct UnexpectedTokenInCodeBlock
    : SimpleDiagnostic<UnexpectedTokenInCodeBlock> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr llvm::StringLiteral Message =
      "Unexpected token in code block.";
};

struct ExpectedFunctionName : SimpleDiagnostic<ExpectedFunctionName> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr llvm::StringLiteral Message =
      "Expected function name after `fn` keyword.";
};

struct ExpectedFunctionParams : SimpleDiagnostic<ExpectedFunctionParams> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr llvm::StringLiteral Message =
      "Expected `(` after function name.";
};

struct ExpectedFunctionBodyOrSemi
    : SimpleDiagnostic<ExpectedFunctionBodyOrSemi> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr llvm::StringLiteral Message =
      "Expected function definition or `;` after function declaration.";
};

struct ExpectedVariableName : SimpleDiagnostic<ExpectedVariableName> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr llvm::StringLiteral Message =
      "Expected pattern in `var` declaration.";
};

struct ExpectedParameterName : SimpleDiagnostic<ExpectedParameterName> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr llvm::StringLiteral Message =
      "Expected parameter declaration.";
};

struct UnrecognizedDeclaration : SimpleDiagnostic<UnrecognizedDeclaration> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr llvm::StringLiteral Message =
      "Unrecognized declaration introducer.";
};

struct ExpectedExpression : SimpleDiagnostic<ExpectedExpression> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr llvm::StringLiteral Message = "Expected expression.";
};

struct ExpectedParenAfter : SimpleDiagnostic<ExpectedParenAfter> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr const char* Message = "Expected `(` after `{0}`.";

  TokenKind introducer;

  auto Format() -> std::string {
    return llvm::formatv(Message, introducer.GetFixedSpelling()).str();
  }
};

struct ExpectedCloseParen : SimpleDiagnostic<ExpectedCloseParen> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr llvm::StringLiteral Message =
      "Unexpected tokens before `)`.";

  // TODO: Include the location of the matching open paren in the diagnostic.
  TokenizedBuffer::Token open_paren;
};

struct ExpectedSemiAfterExpression
    : SimpleDiagnostic<ExpectedSemiAfterExpression> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr llvm::StringLiteral Message =
      "Expected `;` after expression.";
};

struct ExpectedSemiAfter : SimpleDiagnostic<ExpectedSemiAfter> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr const char* Message = "Expected `;` after `{0}`.";

  TokenKind preceding;

  auto Format() -> std::string {
    return llvm::formatv(Message, preceding.GetFixedSpelling()).str();
  }
};

struct ExpectedIdentifierAfterDot
    : SimpleDiagnostic<ExpectedIdentifierAfterDot> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr llvm::StringLiteral Message =
      "Expected identifier after `.`.";
};

struct UnexpectedTokenAfterListElement
    : SimpleDiagnostic<UnexpectedTokenAfterListElement> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr llvm::StringLiteral Message = "Expected `,` or `)`.";
};

struct BinaryOperatorRequiresWhitespace
    : SimpleDiagnostic<BinaryOperatorRequiresWhitespace> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr const char* Message =
      "Whitespace missing {0} binary operator.";

  bool has_leading_space;
  bool has_trailing_space;

  auto Format() -> std::string {
    const char* where = "around";
    // clang-format off
    if (has_leading_space) {
      where = "after";
    } else if (has_trailing_space) {
      where = "before";
    }
    // clang-format on
    return llvm::formatv(Message, where);
  }
};

struct UnaryOperatorHasWhitespace
    : SimpleDiagnostic<UnaryOperatorHasWhitespace> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr const char* Message =
      "Whitespace is not allowed {0} this unary operator.";

  bool prefix;

  auto Format() -> std::string {
    return llvm::formatv(Message, prefix ? "after" : "before");
  }
};

struct UnaryOperatorRequiresWhitespace
    : SimpleDiagnostic<UnaryOperatorRequiresWhitespace> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr const char* Message =
      "Whitespace is required {0} this unary operator.";

  bool prefix;

  auto Format() -> std::string {
    return llvm::formatv(Message, prefix ? "before" : "after");
  }
};

struct OperatorRequiresParentheses
    : SimpleDiagnostic<OperatorRequiresParentheses> {
  static constexpr llvm::StringLiteral ShortName = "syntax-error";
  static constexpr llvm::StringLiteral Message =
      "Parentheses are required to disambiguate operator precedence.";
};

ParseTree::Parser::Parser(ParseTree& tree_arg, TokenizedBuffer& tokens_arg,
                          TokenDiagnosticEmitter& emitter)
    : tree(tree_arg),
      tokens(tokens_arg),
      emitter(emitter),
      position(tokens.Tokens().begin()),
      end(tokens.Tokens().end()) {
  assert(std::find_if(position, end,
                      [&](TokenizedBuffer::Token t) {
                        return tokens.GetKind(t) == TokenKind::EndOfFile();
                      }) != end &&
         "No EndOfFileToken in token buffer.");
}

auto ParseTree::Parser::Parse(TokenizedBuffer& tokens,
                              TokenDiagnosticEmitter& emitter) -> ParseTree {
  ParseTree tree(tokens);

  tree.node_impls.reserve(tokens.Size());

  Parser parser(tree, tokens, emitter);
  while (!parser.AtEndOfFile()) {
    if (!parser.ParseDeclaration()) {
      tree.has_errors = true;
    }
  }

  parser.AddLeafNode(ParseNodeKind::FileEnd(), *parser.position);

  assert(tree.Verify() && "Parse tree built but does not verify!");
  return tree;
}

auto ParseTree::Parser::Consume(TokenKind kind) -> TokenizedBuffer::Token {
  assert(kind != TokenKind::EndOfFile() && "Cannot consume the EOF token!");
  assert(NextTokenIs(kind) && "The current token is the wrong kind!");
  TokenizedBuffer::Token t = *position;
  ++position;
  assert(position != end && "Reached end of tokens without finding EOF token.");
  return t;
}

auto ParseTree::Parser::ConsumeIf(TokenKind kind)
    -> llvm::Optional<TokenizedBuffer::Token> {
  if (!NextTokenIs(kind)) {
    return {};
  }
  return Consume(kind);
}

auto ParseTree::Parser::AddLeafNode(ParseNodeKind kind,
                                    TokenizedBuffer::Token token) -> Node {
  Node n(tree.node_impls.size());
  tree.node_impls.push_back(NodeImpl(kind, token, /*subtree_size_arg=*/1));
  return n;
}

auto ParseTree::Parser::ConsumeAndAddLeafNodeIf(TokenKind t_kind,
                                                ParseNodeKind n_kind)
    -> llvm::Optional<Node> {
  auto t = ConsumeIf(t_kind);
  if (!t) {
    return {};
  }

  return AddLeafNode(n_kind, *t);
}

auto ParseTree::Parser::MarkNodeError(Node n) -> void {
  tree.node_impls[n.index].has_error = true;
  tree.has_errors = true;
}

struct ParseTree::Parser::SubtreeStart {
  int tree_size;
};

auto ParseTree::Parser::GetSubtreeStartPosition() -> SubtreeStart {
  return {static_cast<int>(tree.node_impls.size())};
}

auto ParseTree::Parser::AddNode(ParseNodeKind n_kind, TokenizedBuffer::Token t,
                                SubtreeStart start, bool has_error) -> Node {
  int tree_stop_size = static_cast<int>(tree.node_impls.size()) + 1;
  int subtree_size = tree_stop_size - start.tree_size;

  Node n(tree.node_impls.size());
  tree.node_impls.push_back(NodeImpl(n_kind, t, subtree_size));
  if (has_error) {
    MarkNodeError(n);
  }

  return n;
}

auto ParseTree::Parser::SkipMatchingGroup() -> bool {
  TokenizedBuffer::Token t = *position;
  TokenKind t_kind = tokens.GetKind(t);
  if (!t_kind.IsOpeningSymbol()) {
    return false;
  }

  SkipTo(tokens.GetMatchedClosingToken(t));
  Consume(t_kind.GetClosingSymbol());
  return true;
}

auto ParseTree::Parser::SkipTo(TokenizedBuffer::Token t) -> void {
  assert(t >= *position && "Tried to skip backwards.");
  position = TokenizedBuffer::TokenIterator(t);
  assert(position != end && "Skipped past EOF.");
}

auto ParseTree::Parser::FindNextOf(
    std::initializer_list<TokenKind> desired_kinds)
    -> llvm::Optional<TokenizedBuffer::Token> {
  auto new_position = position;
  while (true) {
    TokenizedBuffer::Token token = *new_position;
    TokenKind kind = tokens.GetKind(token);
    if (kind.IsOneOf(desired_kinds)) {
      return token;
    }

    // Step to the next token at the current bracketing level.
    if (kind.IsClosingSymbol() || kind == TokenKind::EndOfFile()) {
      // There are no more tokens at this level.
      return llvm::None;
    } else if (kind.IsOpeningSymbol()) {
      new_position =
          TokenizedBuffer::TokenIterator(tokens.GetMatchedClosingToken(token));
    } else {
      ++new_position;
    }
  }
}

auto ParseTree::Parser::SkipPastLikelyEnd(TokenizedBuffer::Token skip_root,
                                          SemiHandler on_semi)
    -> llvm::Optional<Node> {
  if (AtEndOfFile()) {
    return llvm::None;
  }

  TokenizedBuffer::Line root_line = tokens.GetLine(skip_root);
  int root_line_indent = tokens.GetIndentColumnNumber(root_line);

  auto is_same_line_or_indent_greater_than_root =
      [&](TokenizedBuffer::Token t) {
        TokenizedBuffer::Line l = tokens.GetLine(t);
        if (l == root_line) {
          return true;
        }

        return tokens.GetIndentColumnNumber(l) > root_line_indent;
      };

  do {
    if (NextTokenKind() == TokenKind::CloseCurlyBrace()) {
      return llvm::None;
    }

    if (auto semi = ConsumeIf(TokenKind::Semi())) {
      return on_semi(*semi);
    }

    // Skip over any matching group of tokens.
    if (SkipMatchingGroup()) {
      continue;
    }

    // Otherwise just step forward one token.
    Consume(NextTokenKind());
  } while (!AtEndOfFile() &&
           is_same_line_or_indent_greater_than_root(*position));

  return llvm::None;
}

auto ParseTree::Parser::ParseCloseParen(TokenizedBuffer::Token open_paren,
                                        ParseNodeKind kind)
    -> llvm::Optional<Node> {
  if (auto close_paren =
          ConsumeAndAddLeafNodeIf(TokenKind::CloseParen(), kind)) {
    return close_paren;
  }

  emitter.EmitError<ExpectedCloseParen>(*position, {.open_paren = open_paren});
  SkipTo(tokens.GetMatchedClosingToken(open_paren));
  AddLeafNode(kind, Consume(TokenKind::CloseParen()));
  return llvm::None;
}

template <typename ListElementParser, typename ListCompletionHandler>
auto ParseTree::Parser::ParseParenList(ListElementParser list_element_parser,
                                       ParseNodeKind comma_kind,
                                       ListCompletionHandler list_handler)
    -> llvm::Optional<Node> {
  // `(` element-list[opt] `)`
  //
  // element-list ::= element
  //              ::= element `,` element-list
  TokenizedBuffer::Token open_paren = Consume(TokenKind::OpenParen());

  bool has_errors = false;

  // Parse elements, if any are specified.
  if (!NextTokenIs(TokenKind::CloseParen())) {
    while (true) {
      bool element_error = !list_element_parser();
      has_errors |= element_error;

      if (!NextTokenIsOneOf({TokenKind::CloseParen(), TokenKind::Comma()})) {
        if (!element_error) {
          emitter.EmitError<UnexpectedTokenAfterListElement>(*position);
        }
        has_errors = true;

        auto end_of_element =
            FindNextOf({TokenKind::Comma(), TokenKind::CloseParen()});
        // The lexer guarantees that parentheses are balanced.
        assert(end_of_element && "missing matching `)` for `(`");
        SkipTo(*end_of_element);
      }

      if (NextTokenIs(TokenKind::CloseParen())) {
        break;
      }

      AddLeafNode(comma_kind, Consume(TokenKind::Comma()));
    }
  }

  return list_handler(open_paren, Consume(TokenKind::CloseParen()), has_errors);
}

auto ParseTree::Parser::ParsePattern(PatternKind kind) -> llvm::Optional<Node> {
  if (NextTokenIs(TokenKind::Identifier()) &&
      tokens.GetKind(*(position + 1)) == TokenKind::Colon()) {
    // identifier `:` type
    auto start = GetSubtreeStartPosition();
    AddLeafNode(ParseNodeKind::DeclaredName(),
                Consume(TokenKind::Identifier()));
    auto colon = Consume(TokenKind::Colon());
    auto type = ParseType();
    return AddNode(ParseNodeKind::PatternBinding(), colon, start,
                   /*has_error=*/!type);
  }

  switch (kind) {
    case PatternKind::Parameter:
      emitter.EmitError<ExpectedParameterName>(*position);
      break;

    case PatternKind::Variable:
      emitter.EmitError<ExpectedVariableName>(*position);
      break;
  }

  return llvm::None;
}

auto ParseTree::Parser::ParseFunctionParameter() -> llvm::Optional<Node> {
  return ParsePattern(PatternKind::Parameter);
}

auto ParseTree::Parser::ParseFunctionSignature() -> bool {
  auto start = GetSubtreeStartPosition();

  auto params = ParseParenList(
      [&] { return ParseFunctionParameter(); },
      ParseNodeKind::ParameterListComma(),
      [&](TokenizedBuffer::Token open_paren, TokenizedBuffer::Token close_paren,
          bool has_errors) {
        AddLeafNode(ParseNodeKind::ParameterListEnd(), close_paren);
        return AddNode(ParseNodeKind::ParameterList(), open_paren, start,
                       has_errors);
      });

  auto start_return_type = GetSubtreeStartPosition();
  if (auto arrow = ConsumeIf(TokenKind::MinusGreater())) {
    auto return_type = ParseType();
    AddNode(ParseNodeKind::ReturnType(), *arrow, start_return_type,
            /*has_error=*/!return_type);
    if (!return_type) {
      return false;
    }
  }

  return params.hasValue();
}

auto ParseTree::Parser::ParseCodeBlock() -> Node {
  TokenizedBuffer::Token open_curly = Consume(TokenKind::OpenCurlyBrace());
  auto start = GetSubtreeStartPosition();

  bool has_errors = false;

  while (!NextTokenIs(TokenKind::CloseCurlyBrace())) {
    if (!ParseStatement()) {
      SkipTo(tokens.GetMatchedClosingToken(open_curly));
      has_errors = true;
      break;
    }
  }

  AddLeafNode(ParseNodeKind::CodeBlockEnd(),
              Consume(TokenKind::CloseCurlyBrace()));

  return AddNode(ParseNodeKind::CodeBlock(), open_curly, start, has_errors);
}

auto ParseTree::Parser::ParseFunctionDeclaration() -> Node {
  TokenizedBuffer::Token function_intro_token = Consume(TokenKind::FnKeyword());
  auto start = GetSubtreeStartPosition();

  auto add_error_function_node = [&] {
    return AddNode(ParseNodeKind::FunctionDeclaration(), function_intro_token,
                   start, /*has_error=*/true);
  };

  auto handle_semi_in_error_recovery = [&](TokenizedBuffer::Token semi) {
    return AddLeafNode(ParseNodeKind::DeclarationEnd(), semi);
  };

  auto name_n = ConsumeAndAddLeafNodeIf(TokenKind::Identifier(),
                                        ParseNodeKind::DeclaredName());
  if (!name_n) {
    emitter.EmitError<ExpectedFunctionName>(*position);
    SkipPastLikelyEnd(function_intro_token, handle_semi_in_error_recovery);
    return add_error_function_node();
  }

  TokenizedBuffer::Token open_paren = *position;
  if (tokens.GetKind(open_paren) != TokenKind::OpenParen()) {
    emitter.EmitError<ExpectedFunctionParams>(open_paren);
    SkipPastLikelyEnd(function_intro_token, handle_semi_in_error_recovery);
    return add_error_function_node();
  }
  TokenizedBuffer::Token close_paren =
      tokens.GetMatchedClosingToken(open_paren);

  if (!ParseFunctionSignature()) {
    SkipPastLikelyEnd(function_intro_token, handle_semi_in_error_recovery);
    return add_error_function_node();
  }

  if (NextTokenIs(TokenKind::OpenCurlyBrace())) {
    ParseCodeBlock();
  } else if (!ConsumeAndAddLeafNodeIf(TokenKind::Semi(),
                                      ParseNodeKind::DeclarationEnd())) {
    emitter.EmitError<ExpectedFunctionBodyOrSemi>(*position);
    if (tokens.GetLine(*position) == tokens.GetLine(close_paren)) {
      SkipPastLikelyEnd(function_intro_token, handle_semi_in_error_recovery);
    }
    return add_error_function_node();
  }

  return AddNode(ParseNodeKind::FunctionDeclaration(), function_intro_token,
                 start);
}

auto ParseTree::Parser::ParseVariableDeclaration() -> Node {
  // `var` pattern [= expression] `;`
  TokenizedBuffer::Token var_token = Consume(TokenKind::VarKeyword());
  auto start = GetSubtreeStartPosition();

  auto pattern = ParsePattern(PatternKind::Variable);
  if (!pattern) {
    if (auto after_pattern =
            FindNextOf({TokenKind::Equal(), TokenKind::Semi()})) {
      SkipTo(*after_pattern);
    }
  }

  auto start_init = GetSubtreeStartPosition();
  if (auto equal_token = ConsumeIf(TokenKind::Equal())) {
    auto init = ParseExpression();
    AddNode(ParseNodeKind::VariableInitializer(), *equal_token, start_init,
            /*has_error=*/!init);
  }

  auto semi = ConsumeAndAddLeafNodeIf(TokenKind::Semi(),
                                      ParseNodeKind::DeclarationEnd());
  if (!semi) {
    emitter.EmitError<ExpectedSemiAfterExpression>(*position);
    SkipPastLikelyEnd(var_token, [&](TokenizedBuffer::Token semi) {
      return AddLeafNode(ParseNodeKind::DeclarationEnd(), semi);
    });
  }

  return AddNode(ParseNodeKind::VariableDeclaration(), var_token, start,
                 /*has_error=*/!pattern || !semi);
}

auto ParseTree::Parser::ParseEmptyDeclaration() -> Node {
  return AddLeafNode(ParseNodeKind::EmptyDeclaration(),
                     Consume(TokenKind::Semi()));
}

auto ParseTree::Parser::ParseDeclaration() -> llvm::Optional<Node> {
  switch (NextTokenKind()) {
    case TokenKind::FnKeyword():
      return ParseFunctionDeclaration();
    case TokenKind::VarKeyword():
      return ParseVariableDeclaration();
    case TokenKind::Semi():
      return ParseEmptyDeclaration();
    case TokenKind::EndOfFile():
      return llvm::None;
    default:
      break;
  }

  emitter.EmitError<UnrecognizedDeclaration>(*position);

  if (auto found_semi_n =
          SkipPastLikelyEnd(*position, [&](TokenizedBuffer::Token semi) {
            return AddLeafNode(ParseNodeKind::EmptyDeclaration(), semi);
          })) {
    MarkNodeError(*found_semi_n);
    return *found_semi_n;
  }

  return llvm::None;
}

auto ParseTree::Parser::ParseParenExpression() -> llvm::Optional<Node> {
  // `(` expression `)`
  auto start = GetSubtreeStartPosition();
  TokenizedBuffer::Token open_paren = Consume(TokenKind::OpenParen());

  auto expr = ParseExpression();

  auto close_paren =
      ParseCloseParen(open_paren, ParseNodeKind::ParenExpressionEnd());

  return AddNode(ParseNodeKind::ParenExpression(), open_paren, start,
                 /*has_error=*/!expr || !close_paren);
}

auto ParseTree::Parser::ParsePrimaryExpression() -> llvm::Optional<Node> {
  llvm::Optional<ParseNodeKind> kind;
  switch (NextTokenKind()) {
    case TokenKind::Identifier():
      kind = ParseNodeKind::NameReference();
      break;

    case TokenKind::IntegerLiteral():
    case TokenKind::RealLiteral():
    case TokenKind::StringLiteral():
      kind = ParseNodeKind::Literal();
      break;

    case TokenKind::OpenParen():
      return ParseParenExpression();

    default:
      emitter.EmitError<ExpectedExpression>(*position);
      return llvm::None;
  }

  return AddLeafNode(*kind, Consume(NextTokenKind()));
}

auto ParseTree::Parser::ParseDesignatorExpression(SubtreeStart start,
                                                  bool has_errors)
    -> llvm::Optional<Node> {
  // `.` identifier
  auto dot = Consume(TokenKind::Period());
  auto name = ConsumeIf(TokenKind::Identifier());
  if (name) {
    AddLeafNode(ParseNodeKind::DesignatedName(), *name);
  } else {
    emitter.EmitError<ExpectedIdentifierAfterDot>(*position);
    if (NextTokenKind().IsKeyword()) {
      Consume(NextTokenKind());
    }
    has_errors = true;
  }
  return AddNode(ParseNodeKind::DesignatorExpression(), dot, start, has_errors);
}

auto ParseTree::Parser::ParseCallExpression(SubtreeStart start, bool has_errors)
    -> llvm::Optional<Node> {
  // `(` expression-list[opt] `)`
  //
  // expression-list ::= expression
  //                 ::= expression `,` expression-list
  return ParseParenList(
      [&] { return ParseExpression(); }, ParseNodeKind::CallExpressionComma(),
      [&](TokenizedBuffer::Token open_paren, TokenizedBuffer::Token close_paren,
          bool has_arg_errors) {
        AddLeafNode(ParseNodeKind::CallExpressionEnd(), close_paren);
        return AddNode(ParseNodeKind::CallExpression(), open_paren, start,
                       has_errors || has_arg_errors);
      });
}

auto ParseTree::Parser::ParsePostfixExpression() -> llvm::Optional<Node> {
  auto start = GetSubtreeStartPosition();
  llvm::Optional<Node> expression = ParsePrimaryExpression();

  while (true) {
    switch (NextTokenKind()) {
      case TokenKind::Period():
        expression = ParseDesignatorExpression(start, !expression);
        break;

      case TokenKind::OpenParen():
        expression = ParseCallExpression(start, !expression);
        break;

      default: {
        return expression;
      }
    }
  }
}

static auto IsAssumedStartOfOperand(TokenKind kind) -> bool {
  return kind.IsOneOf({TokenKind::OpenParen(), TokenKind::Identifier(),
                       TokenKind::IntegerLiteral(), TokenKind::RealLiteral(),
                       TokenKind::StringLiteral()});
}

static auto IsAssumedEndOfOperand(TokenKind kind) -> bool {
  return kind.IsOneOf({TokenKind::CloseParen(), TokenKind::CloseCurlyBrace(),
                       TokenKind::CloseSquareBracket(), TokenKind::Identifier(),
                       TokenKind::IntegerLiteral(), TokenKind::RealLiteral(),
                       TokenKind::StringLiteral()});
}

static auto IsPossibleStartOfOperand(TokenKind kind) -> bool {
  return !kind.IsOneOf({TokenKind::CloseParen(), TokenKind::CloseCurlyBrace(),
                        TokenKind::CloseSquareBracket(), TokenKind::Comma(),
                        TokenKind::Semi(), TokenKind::Colon()});
}

auto ParseTree::Parser::IsLexicallyValidInfixOperator() -> bool {
  assert(!AtEndOfFile() && "Expected an operator token.");

  bool leading_space = tokens.HasLeadingWhitespace(*position);
  bool trailing_space = tokens.HasTrailingWhitespace(*position);

  if (leading_space && trailing_space) {
    return true;
  }

  if (leading_space || trailing_space) {
    return false;
  }

  if (position == tokens.Tokens().begin() ||
      !IsAssumedEndOfOperand(tokens.GetKind(*(position - 1))) ||
      !IsAssumedStartOfOperand(tokens.GetKind(*(position + 1)))) {
    return false;
  }

  return true;
}

auto ParseTree::Parser::DiagnoseOperatorFixity(OperatorFixity fixity) -> void {
  bool is_valid_as_infix = IsLexicallyValidInfixOperator();

  if (fixity == OperatorFixity::Infix) {
    if (!is_valid_as_infix) {
      emitter.EmitError<BinaryOperatorRequiresWhitespace>(
          *position,
          {.has_leading_space = tokens.HasLeadingWhitespace(*position),
           .has_trailing_space = tokens.HasTrailingWhitespace(*position)});
    }
  } else {
    bool prefix = fixity == OperatorFixity::Prefix;

    if (NextTokenKind().IsSymbol() &&
        (prefix ? tokens.HasTrailingWhitespace(*position)
                : tokens.HasLeadingWhitespace(*position))) {
      emitter.EmitError<UnaryOperatorHasWhitespace>(*position,
                                                    {.prefix = prefix});
    }
    if (is_valid_as_infix) {
      emitter.EmitError<UnaryOperatorRequiresWhitespace>(*position,
                                                         {.prefix = prefix});
    }
  }
}

auto ParseTree::Parser::IsTrailingOperatorInfix() -> bool {
  if (AtEndOfFile()) {
    return false;
  }

  if (IsLexicallyValidInfixOperator() &&
      IsPossibleStartOfOperand(tokens.GetKind(*(position + 1)))) {
    return true;
  }

  if (tokens.HasLeadingWhitespace(*position) &&
      IsAssumedStartOfOperand(tokens.GetKind(*(position + 1)))) {
    return true;
  }

  return false;
}

auto ParseTree::Parser::ParseOperatorExpression(
    PrecedenceGroup ambient_precedence) -> llvm::Optional<Node> {
  auto start = GetSubtreeStartPosition();

  llvm::Optional<Node> lhs;
  PrecedenceGroup lhs_precedence = PrecedenceGroup::ForPostfixExpression();

  if (auto operator_precedence = PrecedenceGroup::ForLeading(NextTokenKind());
      !operator_precedence) {
    lhs = ParsePostfixExpression();
  } else {
    if (PrecedenceGroup::GetPriority(ambient_precedence,
                                     *operator_precedence) !=
        OperatorPriority::RightFirst) {
      emitter.EmitError<OperatorRequiresParentheses>(*position);
    } else {
      DiagnoseOperatorFixity(OperatorFixity::Prefix);
    }

    auto operator_token = Consume(NextTokenKind());
    bool has_errors = !ParseOperatorExpression(*operator_precedence);
    lhs = AddNode(ParseNodeKind::PrefixOperator(), operator_token, start,
                  has_errors);
    lhs_precedence = *operator_precedence;
  }

  // Consume a sequence of infix and postfix operators.
  while (auto trailing_operator = PrecedenceGroup::ForTrailing(
             NextTokenKind(), IsTrailingOperatorInfix())) {
    auto [operator_precedence, is_binary] = *trailing_operator;

    if (PrecedenceGroup::GetPriority(ambient_precedence, operator_precedence) !=
        OperatorPriority::RightFirst) {
      return lhs;
    }

    if (PrecedenceGroup::GetPriority(lhs_precedence, operator_precedence) !=
        OperatorPriority::LeftFirst) {
      emitter.EmitError<OperatorRequiresParentheses>(*position);
      lhs = llvm::None;
    } else {
      DiagnoseOperatorFixity(is_binary ? OperatorFixity::Infix
                                       : OperatorFixity::Postfix);
    }

    auto operator_token = Consume(NextTokenKind());

    if (is_binary) {
      auto rhs = ParseOperatorExpression(operator_precedence);
      lhs = AddNode(ParseNodeKind::InfixOperator(), operator_token, start,
                    /*has_error=*/!lhs || !rhs);
    } else {
      lhs = AddNode(ParseNodeKind::PostfixOperator(), operator_token, start,
                    /*has_error=*/!lhs);
    }
    lhs_precedence = operator_precedence;
  }

  return lhs;
}

auto ParseTree::Parser::ParseExpression() -> llvm::Optional<Node> {
  return ParseOperatorExpression(PrecedenceGroup::ForTopLevelExpression());
}

auto ParseTree::Parser::ParseType() -> llvm::Optional<Node> {
  return ParseOperatorExpression(PrecedenceGroup::ForType());
}

auto ParseTree::Parser::ParseExpressionStatement() -> llvm::Optional<Node> {
  TokenizedBuffer::Token start_token = *position;
  auto start = GetSubtreeStartPosition();

  bool has_errors = !ParseExpression();

  if (auto semi = ConsumeIf(TokenKind::Semi())) {
    return AddNode(ParseNodeKind::ExpressionStatement(), *semi, start,
                   has_errors);
  }

  if (!has_errors) {
    emitter.EmitError<ExpectedSemiAfterExpression>(*position);
  }

  if (auto recovery_node =
          SkipPastLikelyEnd(start_token, [&](TokenizedBuffer::Token semi) {
            return AddNode(ParseNodeKind::ExpressionStatement(), semi, start,
                           true);
          })) {
    return recovery_node;
  }

  // Found junk not even followed by a `;`.
  return llvm::None;
}

auto ParseTree::Parser::ParseParenCondition(TokenKind introducer)
    -> llvm::Optional<Node> {
  // `(` expression `)`
  auto start = GetSubtreeStartPosition();
  auto open_paren = ConsumeIf(TokenKind::OpenParen());
  if (!open_paren) {
    emitter.EmitError<ExpectedParenAfter>(*position,
                                          {.introducer = introducer});
  }

  auto expr = ParseExpression();

  if (!open_paren) {
    return llvm::None;
  }

  auto close_paren =
      ParseCloseParen(*open_paren, ParseNodeKind::ConditionEnd());

  return AddNode(ParseNodeKind::Condition(), *open_paren, start,
                 /*has_error=*/!expr || !close_paren);
}

auto ParseTree::Parser::ParseIfStatement() -> llvm::Optional<Node> {
  auto start = GetSubtreeStartPosition();
  auto if_token = Consume(TokenKind::IfKeyword());
  auto cond = ParseParenCondition(TokenKind::IfKeyword());
  auto then_case = ParseStatement();
  bool else_has_errors = false;
  if (ConsumeAndAddLeafNodeIf(TokenKind::ElseKeyword(),
                              ParseNodeKind::IfStatementElse())) {
    else_has_errors = !ParseStatement();
  }
  return AddNode(ParseNodeKind::IfStatement(), if_token, start,
                 /*has_error=*/!cond || !then_case || else_has_errors);
}

auto ParseTree::Parser::ParseWhileStatement() -> llvm::Optional<Node> {
  auto start = GetSubtreeStartPosition();
  auto while_token = Consume(TokenKind::WhileKeyword());
  auto cond = ParseParenCondition(TokenKind::WhileKeyword());
  auto body = ParseStatement();
  return AddNode(ParseNodeKind::WhileStatement(), while_token, start,
                 /*has_error=*/!cond || !body);
}

auto ParseTree::Parser::ParseKeywordStatement(ParseNodeKind kind,
                                              KeywordStatementArgument argument)
    -> llvm::Optional<Node> {
  auto keyword_kind = NextTokenKind();
  assert(keyword_kind.IsKeyword());

  auto start = GetSubtreeStartPosition();
  auto keyword = Consume(keyword_kind);

  bool arg_error = false;
  if ((argument == KeywordStatementArgument::Optional &&
       NextTokenKind() != TokenKind::Semi()) ||
      argument == KeywordStatementArgument::Mandatory) {
    arg_error = !ParseExpression();
  }

  auto semi =
      ConsumeAndAddLeafNodeIf(TokenKind::Semi(), ParseNodeKind::StatementEnd());
  if (!semi) {
    emitter.EmitError<ExpectedSemiAfter>(*position,
                                         {.preceding = keyword_kind});
    // FIXME: Try to skip to a semicolon to recover.
  }
  return AddNode(kind, keyword, start, /*has_error=*/!semi || arg_error);
}

auto ParseTree::Parser::ParseStatement() -> llvm::Optional<Node> {
  switch (NextTokenKind()) {
    case TokenKind::VarKeyword():
      return ParseVariableDeclaration();

    case TokenKind::IfKeyword():
      return ParseIfStatement();

    case TokenKind::WhileKeyword():
      return ParseWhileStatement();

    case TokenKind::ContinueKeyword():
      return ParseKeywordStatement(ParseNodeKind::ContinueStatement(),
                                   KeywordStatementArgument::None);

    case TokenKind::BreakKeyword():
      return ParseKeywordStatement(ParseNodeKind::BreakStatement(),
                                   KeywordStatementArgument::None);

    case TokenKind::ReturnKeyword():
      return ParseKeywordStatement(ParseNodeKind::ReturnStatement(),
                                   KeywordStatementArgument::Optional);

    case TokenKind::OpenCurlyBrace():
      return ParseCodeBlock();

    default:
      return ParseExpressionStatement();
  }
}

}  // namespace Cocktail