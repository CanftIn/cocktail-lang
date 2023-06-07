#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory my_tool_category("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp common_help(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp more_help("\n More help text...\n");

StatementMatcher loop_matcher =
  forStmt(hasLoopInit(declStmt(hasSingleDecl(varDecl(
    hasInitializer(integerLiteral(equals(0)))))))).bind("forLoop");

class LoopPrinter : public MatchFinder::MatchCallback {
public :
   void run(const MatchFinder::MatchResult &result) override {
    if (const auto *fs = result.Nodes.getNodeAs<clang::ForStmt>("forLoop"))
      fs->dump();
  }
};

auto main(int argc, const char **argv) -> int {
  auto expected_parser = CommonOptionsParser::create(argc, argv, my_tool_category);
  if (!expected_parser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << expected_parser.takeError();
    return 1;
  }
  CommonOptionsParser& options_parser = expected_parser.get();
  ClangTool tool(options_parser.getCompilations(),
                 options_parser.getSourcePathList());

  LoopPrinter printer;
  MatchFinder finder;
  finder.addMatcher(loop_matcher, &printer);

  return tool.run(newFrontendActionFactory(&finder).get());
}
