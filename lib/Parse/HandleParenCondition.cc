#include "Cocktail/Parse/Context.h"

namespace Cocktail::Parse {

// Handles ParenConditionAs(If|While).
static auto HandleParenCondition(Context& context, NodeKind start_kind,
                                 State finish_state) -> void {
  auto state = context.PopState();

  std::optional<Lex::Token> open_paren =
      context.ConsumeAndAddOpenParen(state.token, start_kind);
  if (open_paren) {
    state.token = *open_paren;
  }
  state.state = finish_state;
  context.PushState(state);

  if (!open_paren && context.PositionIs(Lex::TokenKind::OpenCurlyBrace)) {
    // For an open curly, assume the condition was completely omitted.
    // Expression parsing would treat the { as a struct, but instead assume it's
    // a code block and just emit an invalid parse.
    context.AddLeafNode(NodeKind::InvalidParse, *context.position(),
                        /*has_error=*/true);
  } else {
    context.PushState(State::Expression);
  }
}

auto HandleParenConditionAsIf(Context& context) -> void {
  HandleParenCondition(context, NodeKind::IfConditionStart,
                       State::ParenConditionFinishAsIf);
}

auto HandleParenConditionAsWhile(Context& context) -> void {
  HandleParenCondition(context, NodeKind::WhileConditionStart,
                       State::ParenConditionFinishAsWhile);
}

auto HandleParenConditionFinishAsIf(Context& context) -> void {
  auto state = context.PopState();

  context.ConsumeAndAddCloseSymbol(state.token, state, NodeKind::IfCondition);
}

auto HandleParenConditionFinishAsWhile(Context& context) -> void {
  auto state = context.PopState();

  context.ConsumeAndAddCloseSymbol(state.token, state,
                                   NodeKind::WhileCondition);
}

}  // namespace Cocktail::Parse