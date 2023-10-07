#include "Cocktail/SemIR/Node.h"

namespace Cocktail::SemIR {

static auto PrintArgs(llvm::raw_ostream& /*out*/,
                      const Node::NoArgs /*no_args*/) -> void {}

template <typename T>
static auto PrintArgs(llvm::raw_ostream& out, T arg) -> void {
  out << ", arg0: " << arg;
}

template <typename T0, typename T1>
static auto PrintArgs(llvm::raw_ostream& out, std::pair<T0, T1> args) -> void {
  PrintArgs(out, args.first);
  out << ", arg1: " << args.second;
}

auto Node::Print(llvm::raw_ostream& out) const -> void {
  out << "{kind: " << kind_;
  // clang warns on unhandled enum values; clang-tidy is incorrect here.
  // NOLINTNEXTLINE(bugprone-switch-missing-default-case)
  switch (kind_) {
#define COCKTAIL_SEMANTICS_NODE_KIND(Name) \
  case NodeKind::Name:                     \
    PrintArgs(out, GetAs##Name());         \
    break;
#include "Cocktail/SemIR/NodeKind.def"
  }
  if (type_id_.is_valid()) {
    out << ", type: " << type_id_;
  }
  out << "}";
}

}  // namespace Cocktail::SemIR