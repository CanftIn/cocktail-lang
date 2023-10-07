#include "Cocktail/Lower/FunctionContext.h"
#include "Cocktail/SemIR/File.h"

namespace Cocktail::Lower {

auto HandleBindValue(FunctionContext& context, SemIR::NodeId node_id,
                     SemIR::Node node) -> void {
  switch (auto rep = SemIR::GetValueRepresentation(context.semantics_ir(),
                                                   node.type_id());
          rep.kind) {
    case SemIR::ValueRepresentation::None:
      // Nothing should use this value, but StubReference needs a value to
      // propagate.
      // TODO: Remove this now the StubReferences are gone.
      context.SetLocal(node_id,
                       llvm::PoisonValue::get(context.GetType(node.type_id())));
      break;
    case SemIR::ValueRepresentation::Copy:
      context.SetLocal(node_id, context.builder().CreateLoad(
                                    context.GetType(node.type_id()),
                                    context.GetLocal(node.GetAsBindValue())));
      break;
    case SemIR::ValueRepresentation::Pointer:
      context.SetLocal(node_id, context.GetLocal(node.GetAsBindValue()));
      break;
    case SemIR::ValueRepresentation::Custom:
      COCKTAIL_FATAL()
          << "TODO: Add support for BindValue with custom value rep";
  }
}

auto HandleTemporary(FunctionContext& context, SemIR::NodeId node_id,
                     SemIR::Node node) -> void {
  auto [temporary_id, init_id] = node.GetAsTemporary();
  context.FinishInitialization(node.type_id(), temporary_id, init_id);
  context.SetLocal(node_id, context.GetLocal(temporary_id));
}

auto HandleTemporaryStorage(FunctionContext& context, SemIR::NodeId node_id,
                            SemIR::Node node) -> void {
  context.SetLocal(
      node_id, context.builder().CreateAlloca(context.GetType(node.type_id()),
                                              nullptr, "temp"));
}

auto HandleValueAsReference(FunctionContext& context, SemIR::NodeId node_id,
                            SemIR::Node node) -> void {
  COCKTAIL_CHECK(SemIR::GetExpressionCategory(context.semantics_ir(),
                                              node.GetAsValueAsReference()) ==
                 SemIR::ExpressionCategory::Value);
  COCKTAIL_CHECK(
      SemIR::GetValueRepresentation(context.semantics_ir(), node.type_id())
          .kind == SemIR::ValueRepresentation::Pointer);
  context.SetLocal(node_id, context.GetLocal(node.GetAsValueAsReference()));
}

}  // namespace Cocktail::Lower