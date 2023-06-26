#include "Cocktail/CppRefactor/FnInserter.h"
#include "Cocktail/CppRefactor/ForRange.h"
#include "Cocktail/CppRefactor/MatcherManager.h"
#include "Cocktail/CppRefactor/VarDecl.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"

using clang::tooling::RefactoringTool;

static void InitReplacements(RefactoringTool* tool) {
  clang::FileManager& files = tool->getFiles();
  Cocktail::Matcher::ReplacementMap& repl = tool->getReplacements();
  for (const std::string& path : tool->getSourcePaths()) {
    llvm::ErrorOr<const clang::FileEntry*> file = files.getFile(path);
    if (file.getError()) {
      llvm::report_fatal_error(llvm::Twine("Error accessing `") + path +
                               "`: " + file.getError().message() + "\n");
    }
    repl.insert({files.getCanonicalName(*file).str(), {}});
  }
}

auto main(int argc, const char** argv) -> int {
  llvm::cl::OptionCategory category("C++ refactoring options");
  auto parser =
      clang::tooling::CommonOptionsParser::create(argc, argv, category);
  RefactoringTool tool(parser->getCompilations(), parser->getSourcePathList());
  InitReplacements(&tool);

  // Set up AST matcher callbacks.
  Cocktail::MatcherManager matchers(&tool.getReplacements());
  matchers.Register(std::make_unique<Cocktail::FnInserterFactory>());
  matchers.Register(std::make_unique<Cocktail::ForRangeFactory>());
  matchers.Register(std::make_unique<Cocktail::VarDeclFactory>());

  return tool.runAndSave(
      clang::tooling::newFrontendActionFactory(matchers.GetFinder()).get());
}