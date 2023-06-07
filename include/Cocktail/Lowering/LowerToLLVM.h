#ifndef COCKTAIL_LOWERING_LOWER_TO_LLVM_H
#define COCKTAIL_LOWERING_LOWER_TO_LLVM_H

#include "Cocktail/Semantics/SemanticsIR.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace Cocktail {

auto LowerToLLVM(llvm::LLVMContext& llvm_context, llvm::StringRef module_name,
                 const SemanticsIR& semantics_ir)
    -> std::unique_ptr<llvm::Module>;

}  // namespace Cocktail

#endif  // COCKTAIL_LOWERING_LOWER_TO_LLVM_H