#include "Cocktail/Lowering/LowerToLLVM.h"

namespace Cocktail {

auto LowerToLLVM(llvm::LLVMContext& llvm_context, llvm::StringRef module_name,
                 const SemanticsIR& semantics_ir)
    -> std::unique_ptr<llvm::Module> {
  auto module = std::make_unique<llvm::Module>(module_name, llvm_context);
  return module;
}

}  // namespace Cocktail