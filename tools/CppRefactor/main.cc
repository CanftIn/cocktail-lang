#include "Cocktail/CppRefactor/FnInserter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"

namespace cam = ::clang::ast_matchers;
namespace ct = ::clang::tooling;

static void InitReplacements(ct::RefactoringTool* tool) {
  auto& files = tool->getFiles();
  auto& repl = tool->getReplacements();
  for (const auto& path : tool->getSourcePaths()) {
    auto file = files.getFile(path);
    if (file.getError()) {
      llvm::report_fatal_error(llvm::Twine("Error accessing `" + path + "`: " +
                                           file.getError().message() + "\n"));
    }
    repl.insert({files.getCanonicalName(*file).str(), {}});
  }
}

auto main(int argc, const char** argv) -> int {
  llvm::cl::OptionCategory category("C++ refactoring options");
  auto parser = ct::CommonOptionsParser::create(argc, argv, category);
  ct::RefactoringTool tool(parser->getCompilations(),
                           parser->getSourcePathList());
  InitReplacements(&tool);

  // Set up AST matcher callbacks.
  cam::MatchFinder finder;
  Cocktail::FnInserter fn_inserter(tool.getReplacements(), &finder);

  return tool.runAndSave(
      clang::tooling::newFrontendActionFactory(&finder).get());
}