#include "Cocktail/CppRefactor/FnInserter.h"

#include "clang/ASTMatchers/ASTMatchers.h"

namespace cam = ::clang::ast_matchers;

namespace Cocktail {

FnInserter::FnInserter(std::map<std::string, Replacements>& in_replacements,
                       cam::MatchFinder* finder)
    : Matcher(in_replacements) {
  finder->addMatcher(cam::functionDecl(cam::hasTrailingReturn()).bind(Label),
                     this);
}

void FnInserter::run(const cam::MatchFinder::MatchResult& result) {
  const auto* decl = result.Nodes.getNodeAs<clang::FunctionDecl>(Label);
  if (!decl) {
    llvm::report_fatal_error(
        llvm::Twine("getNodeAs failed for " + std::string(Label)));
  }
  auto begin = decl->getBeginLoc();
  // Replace the first token in the range, `auto`.
  auto range = clang::CharSourceRange::getTokenRange(begin, begin);
  AddReplacement(*(result.SourceManager), range, "fn");
}

}  // namespace Cocktail