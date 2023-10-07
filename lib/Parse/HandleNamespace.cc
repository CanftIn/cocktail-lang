#include "Cocktail/Parse/Context.h"

namespace Cocktail::Parse {

auto HandleNamespace(Context& context) -> void {
  auto state = context.PopState();

  context.AddLeafNode(NodeKind::NamespaceStart, context.Consume());

  state.state = State::NamespaceFinish;
  context.PushState(state);

  context.PushState(State::DeclarationNameAndParamsAsNone, state.token);
}

auto HandleNamespaceFinish(Context& context) -> void {
  auto state = context.PopState();

  if (state.has_error) {
    context.RecoverFromDeclarationError(state, NodeKind::Namespace,
                                        /*skip_past_likely_end=*/true);
    return;
  }

  if (auto semi = context.ConsumeIf(Lex::TokenKind::Semi)) {
    context.AddNode(NodeKind::Namespace, *semi, state.subtree_start,
                    state.has_error);
  } else {
    context.EmitExpectedDeclarationSemi(Lex::TokenKind::Namespace);
    context.RecoverFromDeclarationError(state, NodeKind::Namespace,
                                        /*skip_past_likely_end=*/true);
  }
}

}  // namespace Cocktail::Parse