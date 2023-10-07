#include "Cocktail/Parse/Context.h"

namespace Cocktail::Parse {

auto HandleCodeBlock(Context& context) -> void {
  context.PopAndDiscardState();

  context.PushState(State::CodeBlockFinish);
  if (context.ConsumeAndAddLeafNodeIf(Lex::TokenKind::OpenCurlyBrace,
                                      NodeKind::CodeBlockStart)) {
    context.PushState(State::StatementScopeLoop);
  } else {
    context.AddLeafNode(NodeKind::CodeBlockStart, *context.position(),
                        /*has_error=*/true);

    // Recover by parsing a single statement.
    COCKTAIL_DIAGNOSTIC(ExpectedCodeBlock, Error,
                        "Expected braced code block.");
    context.emitter().Emit(*context.position(), ExpectedCodeBlock);

    context.PushState(State::Statement);
  }
}

auto HandleCodeBlockFinish(Context& context) -> void {
  auto state = context.PopState();

  // If the block started with an open curly, this is a close curly.
  if (context.tokens().GetKind(state.token) == Lex::TokenKind::OpenCurlyBrace) {
    context.AddNode(NodeKind::CodeBlock, context.Consume(), state.subtree_start,
                    state.has_error);
  } else {
    context.AddNode(NodeKind::CodeBlock, state.token, state.subtree_start,
                    /*has_error=*/true);
  }
}

}  // namespace Cocktail::Parse