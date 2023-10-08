#ifndef COCKTAIL_PARSE_NODE_KIND_H
#define COCKTAIL_PARSE_NODE_KIND_H

#include <cstdint>

#include "Cocktail/Common/EnumBase.h"

namespace Cocktail::Parse {

COCKTAIL_DEFINE_RAW_ENUM_CLASS(NodeKind, uint8_t) {
#define COCKTAIL_PARSE_NODE_KIND(Name) COCKTAIL_RAW_ENUM_ENUMERATOR(Name)
#include "Cocktail/Parse/NodeKind.def"
};

// 包装了解析树中不同类型节点的枚举。
class NodeKind : public COCKTAIL_ENUM_BASE(NodeKind) {
 public:
#define COCKTAIL_PARSE_NODE_KIND(Name) COCKTAIL_ENUM_CONSTANT_DECLARATION(Name)
#include "Cocktail/Parse/NodeKind.def"

  // 如果节点被括起来，则返回true，否则使用child_count。
  auto has_bracket() const -> bool;

  // 返回当前节点类型的括号节点类型。要求has_bracket为真。
  auto bracket() const -> NodeKind;

  // 返回节点必须具有的子节点数，通常为0。要求has_bracket为假。
  auto child_count() const -> int32_t;

  using EnumBase::Create;
};

#define COCKTAIL_PARSE_NODE_KIND(Name) \
  COCKTAIL_ENUM_CONSTANT_DEFINITION(NodeKind, Name)
#include "Cocktail/Parse/NodeKind.def"

static_assert(sizeof(NodeKind) == 1, "Kind objects include padding!");

}  // namespace Cocktail::Parse

#endif  // COCKTAIL_PARSE_NODE_KIND_H