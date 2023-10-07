#include "Cocktail/Lower/Lower.h"

#include "Cocktail/Lower/FileContext.h"

namespace Cocktail::Lower {

auto LowerToLLVM(llvm::LLVMContext& llvm_context, llvm::StringRef module_name,
                 const SemIR::File& semantics_ir,
                 llvm::raw_ostream* vlog_stream)
    -> std::unique_ptr<llvm::Module> {
  FileContext context(llvm_context, module_name, semantics_ir, vlog_stream);
  return context.Run();
}

}  // namespace Cocktail::Lower