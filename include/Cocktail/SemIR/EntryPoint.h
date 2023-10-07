#ifndef COCKTAIL_SEMIR_ENTRY_POINT_H
#define COCKTAIL_SEMIR_ENTRY_POINT_H

#include "Cocktail/SemIR/File.h"

namespace Cocktail::SemIR {

// Returns whether the specified function is the entry point function for the
// program, `Main.Run`.
auto IsEntryPoint(const SemIR::File& file, SemIR::FunctionId function_id)
    -> bool;

}  // namespace Cocktail::SemIR

#endif  // COCKTAIL_SEMIR_ENTRY_POINT_H