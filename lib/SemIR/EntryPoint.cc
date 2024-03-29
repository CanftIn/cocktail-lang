#include "Cocktail/SemIR/EntryPoint.h"

#include "llvm/ADT/StringRef.h"

namespace Cocktail::SemIR {

static constexpr llvm::StringLiteral EntryPointFunction = "Run";

auto IsEntryPoint(const SemIR::File& file, SemIR::FunctionId function_id)
    -> bool {
  // TODO: Check if `file` is in the `Main` package.
  auto& function = file.GetFunction(function_id);
  // TODO: Check if `function` is in a namespace.
  return function.name_id.is_valid() &&
         file.GetString(function.name_id) == EntryPointFunction;
}

}  // namespace Cocktail::SemIR