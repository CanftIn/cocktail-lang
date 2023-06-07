#ifndef COCKTAIL_LEXER_LEX_HELPER_H
#define COCKTAIL_LEXER_LEX_HELPER_H

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"

namespace Cocktail {

// Should guard calls to getAsInteger due to performance issues with large
// integers. Emits an error if the text cannot be lexed.
auto CanLexInteger(DiagnosticEmitter<const char*>& emitter,
                   llvm::StringRef text) -> bool;

}  // namespace Cocktail

#endif  // COCKTAIL_LEXER_LEX_HELPER_H