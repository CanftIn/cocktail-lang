#include "Cocktail/Parse/Context.h"

namespace Cocktail::Parse {

auto HandleIndexExpression(Context& context) -> void {
  auto state = context.PopState();
  state.state = State::IndexExpressionFinish;
  context.PushState(state);
  context.AddNode(NodeKind::IndexExpressionStart,
                  context.ConsumeChecked(Lex::TokenKind::OpenSquareBracket),
                  state.subtree_start, state.has_error);
  context.PushState(State::Expression);
}

auto HandleIndexExpressionFinish(Context& context) -> void {
  auto state = context.PopState();

  context.ConsumeAndAddCloseSymbol(state.token, state,
                                   NodeKind::IndexExpression);
}

}  // namespace Cocktail::Parse