#ifndef COCKTAIL_SEMIR_FORMATTER_H
#define COCKTAIL_SEMIR_FORMATTER_H

#include "Cocktail/Lex/TokenizedBuffer.h"
#include "Cocktail/Parse/Tree.h"
#include "Cocktail/SemIR/File.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail::SemIR {

auto FormatFile(const Lex::TokenizedBuffer& tokenized_buffer,
                const Parse::Tree& parse_tree, const File& semantics_ir,
                llvm::raw_ostream& out) -> void;

}  // namespace Cocktail::SemIR

#endif  // COCKTAIL_SEMIR_FORMATTER_H