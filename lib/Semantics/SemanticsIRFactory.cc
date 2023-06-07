#include "Cocktail/Semantics/SemanticsIRFactory.h"

#include "Cocktail/Common/Check.h"
#include "Cocktail/Lexer/TokenizedBuffer.h"
#include "Cocktail/Parser/ParseNodeKind.h"
#include "llvm/Support/FormatVariadic.h"

namespace Cocktail {

auto SemanticsIRFactory::Build(const ParseTree& parse_tree) -> SemanticsIR {
  SemanticsIRFactory factory(parse_tree);
  factory.ProcessRoots();
  return factory.semantics_;
}

void SemanticsIRFactory::ProcessRoots() {}

void SemanticsIRFactory::ProcessFuntionNode(SemanticsIR::Block& block,
                                            ParseTree::Node decl_node) {}

}  // namespace Cocktail