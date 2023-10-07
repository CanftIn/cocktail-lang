#ifndef COCKTAIL_LOWER_LOWER_H
#define COCKTAIL_LOWER_LOWER_H

#include "Cocktail/SemIR/File.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace Cocktail::Lower {

// Lowers SemIR to LLVM IR.
auto LowerToLLVM(llvm::LLVMContext& llvm_context, llvm::StringRef module_name,
                 const SemIR::File& semantics_ir,
                 llvm::raw_ostream* vlog_stream)
    -> std::unique_ptr<llvm::Module>;

}  // namespace Cocktail::Lower

#endif  // COCKTAIL_LOWER_LOWER_H