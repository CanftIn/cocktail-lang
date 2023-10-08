#include "Cocktail/Check/Context.h"
#include "Cocktail/Check/Convert.h"
#include "Cocktail/SemIR/Node.h"
#include "llvm/ADT/STLExtras.h"

namespace Cocktail::Check {

auto HandleMemberAccessExpression(Context& context, Parse::Node parse_node)
    -> bool {
  SemIR::StringId name_id = context.node_stack().Pop<Parse::NodeKind::Name>();

  auto base_id = context.node_stack().PopExpression();

  auto base =
      context.semantics_ir().GetNode(context.FollowNameReferences(base_id));
  if (base.kind() == SemIR::NodeKind::Namespace) {
    // For a namespace, just resolve the name.
    auto node_id =
        context.LookupName(parse_node, name_id, base.GetAsNamespace(),
                           /*print_diagnostics=*/true);
    context.node_stack().Push(parse_node, node_id);
    return true;
  }

  // Materialize a temporary for the base expression if necessary.
  base_id = ConvertToValueOrReferenceExpression(context, base_id);

  auto base_type = context.semantics_ir().GetNode(
      context.semantics_ir().GetTypeAllowBuiltinTypes(base.type_id()));

  switch (base_type.kind()) {
    case SemIR::NodeKind::StructType: {
      auto refs =
          context.semantics_ir().GetNodeBlock(base_type.GetAsStructType());
      // TODO: Do we need to optimize this with a lookup table for O(1)?
      for (auto item : llvm::enumerate(refs)) {
        auto i = item.index();
        auto ref_id = item.value();
        auto ref = context.semantics_ir().GetNode(ref_id);
        if (auto [field_name_id, field_type_id] = ref.GetAsStructTypeField();
            name_id == field_name_id) {
          context.AddNodeAndPush(
              parse_node,
              SemIR::Node::StructAccess::Make(parse_node, field_type_id,
                                              base_id, SemIR::MemberIndex(i)));
          return true;
        }
      }
      COCKTAIL_DIAGNOSTIC(QualifiedExpressionNameNotFound, Error,
                          "Type `{0}` does not have a member `{1}`.",
                          std::string, llvm::StringRef);
      context.emitter().Emit(
          parse_node, QualifiedExpressionNameNotFound,
          context.semantics_ir().StringifyType(base.type_id()),
          context.semantics_ir().GetString(name_id));
      break;
    }
    default: {
      if (base.type_id() != SemIR::TypeId::Error) {
        COCKTAIL_DIAGNOSTIC(
            QualifiedExpressionUnsupported, Error,
            "Type `{0}` does not support qualified expressions.", std::string);
        context.emitter().Emit(
            parse_node, QualifiedExpressionUnsupported,
            context.semantics_ir().StringifyType(base.type_id()));
      }
      break;
    }
  }

  // Should only be reached on error.
  context.node_stack().Push(parse_node, SemIR::NodeId::BuiltinError);
  return true;
}

auto HandlePointerMemberAccessExpression(Context& context,
                                         Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandlePointerMemberAccessExpression");
}

auto HandleName(Context& context, Parse::Node parse_node) -> bool {
  auto name_str = context.parse_tree().GetNodeText(parse_node);
  auto name_id = context.semantics_ir().AddString(name_str);
  // The parent is responsible for binding the name.
  context.node_stack().Push(parse_node, name_id);
  return true;
}

auto HandleNameExpression(Context& context, Parse::Node parse_node) -> bool {
  auto name_str = context.parse_tree().GetNodeText(parse_node);
  auto name_id = context.semantics_ir().AddString(name_str);
  auto value_id =
      context.LookupName(parse_node, name_id, SemIR::NameScopeId::Invalid,
                         /*print_diagnostics=*/true);
  auto value = context.semantics_ir().GetNode(value_id);
  if (value.kind().value_kind() == SemIR::NodeValueKind::Typed) {
    // This is a reference to a name binding that has a value and a type.
    context.AddNodeAndPush(parse_node,
                           SemIR::Node::NameReference::Make(
                               parse_node, value.type_id(), name_id, value_id));
  } else {
    // This is something like a namespace name, that can be found by name lookup
    // but isn't a first-class value with a type.
    context.AddNodeAndPush(parse_node,
                           SemIR::Node::NameReferenceUntyped::Make(
                               parse_node, value.type_id(), name_id, value_id));
  }
  return true;
}

auto HandleQualifiedDeclaration(Context& context, Parse::Node parse_node)
    -> bool {
  auto pop_and_apply_first_child = [&]() {
    if (context.parse_tree().node_kind(context.node_stack().PeekParseNode()) !=
        Parse::NodeKind::QualifiedDeclaration) {
      // First QualifiedDeclaration in a chain.
      auto [parse_node1, node_id1] =
          context.node_stack().PopExpressionWithParseNode();
      context.declaration_name_stack().ApplyExpressionQualifier(
          parse_node1, context.FollowNameReferences(node_id1));
      // Add the QualifiedDeclaration so that it can be used for bracketing.
      context.node_stack().Push(parse_node);
    } else {
      // Nothing to do: the QualifiedDeclaration remains as a bracketing node
      // for later QualifiedDeclarations.
    }
  };

  Parse::Node parse_node2 = context.node_stack().PeekParseNode();
  if (context.parse_tree().node_kind(parse_node2) == Parse::NodeKind::Name) {
    SemIR::StringId name_id2 =
        context.node_stack().Pop<Parse::NodeKind::Name>();
    pop_and_apply_first_child();
    context.declaration_name_stack().ApplyNameQualifier(parse_node2, name_id2);
  } else {
    SemIR::NodeId node_id2 = context.node_stack().PopExpression();
    pop_and_apply_first_child();
    context.declaration_name_stack().ApplyExpressionQualifier(parse_node2,
                                                              node_id2);
  }

  return true;
}

auto HandleSelfTypeNameExpression(Context& context, Parse::Node parse_node)
    -> bool {
  return context.TODO(parse_node, "HandleSelfTypeNameExpression");
}

auto HandleSelfValueName(Context& context, Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandleSelfValueName");
}

}  // namespace Cocktail::Check