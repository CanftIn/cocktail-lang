#ifndef COCKTAIL_CHECK_CHECK_H
#define COCKTAIL_CHECK_CHECK_H

#include "Cocktail/Common/Ostream.h"
#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Cocktail/Lex/TokenizedBuffer.h"
#include "Cocktail/Parse/Tree.h"
#include "Cocktail/SemIR/File.h"

namespace Cocktail::Check {

// Constructs builtins. A single instance should be reused with CheckParseTree
// calls associated with a given compilation.
inline auto MakeBuiltins() -> SemIR::File { return SemIR::File(); }

// Produces and checks the IR for the provided Parse::Tree.
extern auto CheckParseTree(const SemIR::File& builtin_ir,
                           const Lex::TokenizedBuffer& tokens,
                           const Parse::Tree& parse_tree,
                           DiagnosticConsumer& consumer,
                           llvm::raw_ostream* vlog_stream) -> SemIR::File;

}  // namespace Cocktail::Check

#endif  // COCKTAIL_CHECK_CHECK_H