#include "Cocktail/CppRefactor/Matcher.h"

#include "clang/Basic/SourceManager.h"

namespace Cocktail {

void Matcher::AddReplacement(clang::CharSourceRange range,
                             llvm::StringRef replacement_text) {
  if (!range.isValid()) {
    return;
  }
  const auto& source_manager = GetSourceManager();
  if (source_manager.getDecomposedLoc(range.getBegin()).first !=
      source_manager.getDecomposedLoc(range.getEnd()).first) {
    return;
  }
  if (source_manager.getFileID(range.getBegin()) !=
      source_manager.getFileID(range.getEnd())) {
    return;
  }

  auto rep = clang::tooling::Replacement(
      source_manager, source_manager.getExpansionRange(range),
      replacement_text);
  auto entry = replacements->find(std::string(rep.getFilePath()));
  if (entry == replacements->end()) {
    return;
  }

  llvm::Error err = entry->second.add(rep);
  if (err) {
    llvm::errs() << "Error with replacement `" << rep.toString()
                 << "`: " << llvm::toString(std::move(err)) << "\n";
  }
}

}  // namespace Cocktail