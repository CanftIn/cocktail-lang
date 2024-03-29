#include "Cocktail/Parse/Context.h"

namespace Cocktail::Parse {

auto HandleArrayExpression(Context& context) -> void {
  auto state = context.PopState();
  state.state = State::ArrayExpressionSemi;
  context.AddLeafNode(NodeKind::ArrayExpressionStart,
                      context.ConsumeChecked(Lex::TokenKind::OpenSquareBracket),
                      state.has_error);
  context.PushState(state);
  context.PushState(State::Expression);
}

auto HandleArrayExpressionSemi(Context& context) -> void {
  auto state = context.PopState();
  auto semi = context.ConsumeIf(Lex::TokenKind::Semi);
  if (!semi) {
    context.AddNode(NodeKind::ArrayExpressionSemi, *context.position(),
                    state.subtree_start, true);
    COCKTAIL_DIAGNOSTIC(ExpectedArraySemi, Error,
                        "Expected `;` in array type.");
    context.emitter().Emit(*context.position(), ExpectedArraySemi);
    state.has_error = true;
  } else {
    context.AddNode(NodeKind::ArrayExpressionSemi, *semi, state.subtree_start,
                    state.has_error);
  }
  state.state = State::ArrayExpressionFinish;
  context.PushState(state);
  if (!context.PositionIs(Lex::TokenKind::CloseSquareBracket)) {
    context.PushState(State::Expression);
  }
}

auto HandleArrayExpressionFinish(Context& context) -> void {
  auto state = context.PopState();
  context.ConsumeAndAddCloseSymbol(*(Lex::TokenIterator(state.token)), state,
                                   NodeKind::ArrayExpression);
}

}  // namespace Cocktail::Parse