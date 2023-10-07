#include "Cocktail/Lower/FunctionContext.h"

namespace Cocktail::Lower {

auto HandleArrayType(FunctionContext& context, SemIR::NodeId node_id,
                     SemIR::Node /*node*/) -> void {
  context.SetLocal(node_id, context.GetTypeAsValue());
}

auto HandleConstType(FunctionContext& context, SemIR::NodeId node_id,
                     SemIR::Node /*node*/) -> void {
  context.SetLocal(node_id, context.GetTypeAsValue());
}

auto HandlePointerType(FunctionContext& context, SemIR::NodeId node_id,
                       SemIR::Node /*node*/) -> void {
  context.SetLocal(node_id, context.GetTypeAsValue());
}

auto HandleStructType(FunctionContext& context, SemIR::NodeId node_id,
                      SemIR::Node /*node*/) -> void {
  context.SetLocal(node_id, context.GetTypeAsValue());
}

auto HandleTupleType(FunctionContext& context, SemIR::NodeId node_id,
                     SemIR::Node /*node*/) -> void {
  context.SetLocal(node_id, context.GetTypeAsValue());
}

}  // namespace Cocktail::Lower